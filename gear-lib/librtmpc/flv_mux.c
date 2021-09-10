/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#include <stdlib.h>
#include <libserializer.h>
#include <libposix.h>
#include "flv_mux.h"
//#define __STDC_FORMAT_MACROS
//#include <inttypes.h>

enum {
    FLV_CODECID_H263    = 2,
    FLV_CODECID_SCREEN  = 3,
    FLV_CODECID_VP6     = 4,
    FLV_CODECID_VP6A    = 5,
    FLV_CODECID_SCREEN2 = 6,
    FLV_CODECID_H264    = 7,
};

enum flv_tag {
    FLV_TAG_TYPE_AUDIO = 0x08,
    FLV_TAG_TYPE_VIDEO = 0x09,
    FLV_TAG_TYPE_META  = 0x12,
};

enum flv_header {
    FLV_HEADER_FLAG_HASVIDEO = 1,
    FLV_HEADER_FLAG_HASAUDIO = 4,
};

enum amf_data_type {
    AMF_DATA_TYPE_NUMBER      = 0x00,
    AMF_DATA_TYPE_BOOL        = 0x01,
    AMF_DATA_TYPE_STRING      = 0x02,
    AMF_DATA_TYPE_OBJECT      = 0x03,
    AMF_DATA_TYPE_NULL        = 0x05,
    AMF_DATA_TYPE_UNDEFINED   = 0x06,
    AMF_DATA_TYPE_REFERENCE   = 0x07,
    AMF_DATA_TYPE_MIXEDARRAY  = 0x08,
    AMF_DATA_TYPE_OBJECT_END  = 0x09,
    AMF_DATA_TYPE_ARRAY       = 0x0a,
    AMF_DATA_TYPE_DATE        = 0x0b,
    AMF_DATA_TYPE_LONG_STRING = 0x0c,
    AMF_DATA_TYPE_UNSUPPORTED = 0x0d,
};

#define FLV_AUDIO_SAMPLESSIZE_OFFSET 1
#define FLV_AUDIO_SAMPLERATE_OFFSET  2
#define FLV_AUDIO_CODECID_OFFSET     4
#define FLV_VIDEO_FRAMETYPE_OFFSET   4

enum {
    FLV_MONO   = 0,
    FLV_STEREO = 1,
};

enum {
    FLV_SAMPLESSIZE_8BIT  = 0,
    FLV_SAMPLESSIZE_16BIT = 1 << FLV_AUDIO_SAMPLESSIZE_OFFSET,
};

enum {
    FLV_SAMPLERATE_SPECIAL = 0, /**< signifies 5512Hz and 8000Hz in the case of NELLYMOSER */
    FLV_SAMPLERATE_11025HZ = 1 << FLV_AUDIO_SAMPLERATE_OFFSET,
    FLV_SAMPLERATE_22050HZ = 2 << FLV_AUDIO_SAMPLERATE_OFFSET,
    FLV_SAMPLERATE_44100HZ = 3 << FLV_AUDIO_SAMPLERATE_OFFSET,
};

enum {
    FLV_CODECID_PCM                  = 0,
    FLV_CODECID_ADPCM                = 1 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_MP3                  = 2 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_PCM_LE               = 3 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_NELLYMOSER_16KHZ_MONO = 4 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_NELLYMOSER_8KHZ_MONO = 5 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_NELLYMOSER           = 6 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_G711_ALAW  = 7 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_G711_MULAW  = 8 << FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_AAC                  = 10<< FLV_AUDIO_CODECID_OFFSET,
    FLV_CODECID_SPEEX                = 11<< FLV_AUDIO_CODECID_OFFSET,
};

enum {
    FLV_FRAME_KEY        = 1 << FLV_VIDEO_FRAMETYPE_OFFSET,
    FLV_FRAME_INTER      = 2 << FLV_VIDEO_FRAMETYPE_OFFSET,
    FLV_FRAME_DISP_INTER = 3 << FLV_VIDEO_FRAMETYPE_OFFSET,
};

uint64_t dbl2int(double value)
{
    union tmp{double f; unsigned long long i;} u;
    u.f = value;
    return u.i;
}

#define AMF_END_OF_OBJECT         0x09

struct flv_muxer *flv_mux_create(flv_mux_output_cb *cb, void *cb_ctx)
{
    struct flv_muxer *flv = calloc(1, sizeof(struct flv_muxer));
    if (!flv) {
        printf("malloc flv_muxer failed!\n");
        return NULL;
    }
    flv->output_cb = cb;
    flv->output_cb_ctx = cb_ctx;
    serializer_array_init(&flv->s);
    flv->is_keyframe_got = false;
    flv->is_header = true;
    return flv;
}

void flv_mux_destroy(struct flv_muxer *flv)
{
    if (!flv) {
        return;
    }
    serializer_array_deinit(&flv->s);
    if (flv->audio)
        free(flv->audio);
    if (flv->video)
        free(flv->video);
    free(flv);
}

int flv_mux_add_media(struct flv_muxer *flv, struct media_packet *mp)
{
    switch (mp->type) {
    case MEDIA_TYPE_AUDIO:
        if (flv->audio) {
            printf("flv_mux already add audio encoder!\n");
            return -1;
        }
        flv->audio = calloc(1, sizeof(struct audio_encoder));
        memcpy(flv->audio, &mp->audio->encoder, sizeof(struct audio_encoder));
        break;
    case MEDIA_TYPE_VIDEO:
        if (flv->video) {
            printf("flv_mux already add video encoder!\n");
            return -1;
        }
        flv->video = calloc(1, sizeof(struct video_encoder));
        memcpy(flv->video, &mp->video->encoder, sizeof(struct video_encoder));
        break;
    default:
        printf("unsupport type!\n");
        break;
    }
    return 0;
}

#define MILLISECOND_DEN 1000

static int32_t get_ms_time_v(struct video_packet *packet, int64_t val)
{
    return (int32_t)(val * MILLISECOND_DEN / packet->encoder.timebase.den);
}

static int32_t get_ms_time_a(struct audio_packet *packet, int64_t val)
{
    return (int32_t)(val * MILLISECOND_DEN / packet->encoder.timebase.den);
}

static int write_video(struct serializer *s, struct video_packet *vp, int32_t dts_offset, bool is_hdr)
{
    uint8_t *data;
    size_t size;
    int64_t offset = vp->pts - vp->dts;
    int32_t time_ms = get_ms_time_v(vp, vp->dts) - dts_offset;

    s_w8(s, FLV_TAG_TYPE_VIDEO);

    s_wb24(s, vp->size + 5);
    s_wb24(s, time_ms);
    s_w8(s, (time_ms >> 24) & 0x7f);
    s_wb24(s, 0);
    s_w8(s, (vp->key_frame?FLV_FRAME_KEY:FLV_FRAME_INTER) | FLV_CODECID_H264);
    s_w8(s, is_hdr ? 0 : 1);
    s_wb24(s, get_ms_time_v(vp, offset));

    s_write(s, vp->data, vp->size);
    s_wb32(s, s_getpos(s) - 1);

    serializer_array_get_data(s, &data, &size);

    return 0;
}

static int write_audio(struct serializer *s, struct audio_packet *ap, int32_t dts_offset, bool is_hdr)
{
    int32_t time_ms = get_ms_time_a(ap, ap->dts) - dts_offset;

    s_w8(s, FLV_TAG_TYPE_AUDIO);
    s_wb24(s, ap->size + 2);
    s_wb24(s, time_ms);
    s_w8(s, (time_ms >> 24) & 0x7F);
    s_wb24(s, 0);
    s_w8(s, FLV_CODECID_AAC|FLV_SAMPLERATE_44100HZ|FLV_SAMPLESSIZE_16BIT|FLV_STEREO);
    s_w8(s, is_hdr ? 0 : 1);
    s_write(s, ap->data, ap->size);
    s_wb32(s, s_getpos(s) - 1);

    return 0;
}

static void s_amf_string(struct serializer *s, const char *str)
{
    size_t len = strlen(str);
    s_wb16(s, len);
    s_write(s, str, len);
}

static void s_amf_double(struct serializer *s, double d)
{
    s_w8(s, AMF_DATA_TYPE_NUMBER);
    s_wb64(s, dbl2int(d));
}

static int build_meta_data(struct flv_muxer *flv, uint8_t **output, size_t *size)
{
    struct serializer *s, ss;
    uint8_t *data;
    unsigned int codec_id = 0xffffffff;

    s = &ss;
    serializer_array_init(&ss);

    s_w8(s, AMF_DATA_TYPE_STRING);
    s_amf_string(s, "onMetaData"); // 12 bytes

    /* mixed array (hash) with size and string/type/data tuples */
    s_w8(s, AMF_DATA_TYPE_MIXEDARRAY);
    s_wb32(s, 5*!!flv->video + 5*!!flv->audio + 2); // +2 for duration and file size

    s_amf_string(s, "duration");
    s_amf_double(s, 0.0);

    if (flv->video) {
        s_amf_string(s, "width");
        s_amf_double(s, flv->video->width);

        s_amf_string(s, "height");
        s_amf_double(s, flv->video->height);
        s_amf_string(s, "videodatarate");
        s_amf_double(s, flv->video->bitrate/1024.0);
        s_amf_string(s, "framerate");
        s_amf_double(s, flv->video->framerate.num/flv->video->framerate.den);

        s_amf_string(s, "videocodecid");
        s_amf_double(s, FLV_CODECID_H264);
    }

    if (flv->audio) {
        s_amf_string(s, "audiodatarate");
        s_amf_double(s, flv->audio->bitrate /1024.0);
        s_amf_string(s, "audiosamplerate");
        s_amf_double(s, flv->audio->sample_rate);
        s_amf_string(s, "audiosamplesize");
        s_amf_double(s, flv->audio->sample_size);
        s_amf_string(s, "stereo");
        s_w8(s, AMF_DATA_TYPE_BOOL);
        s_w8(s, (flv->audio->channels == 2));

        s_amf_string(s, "audiocodecid");

        switch (flv->audio->format) {
        case AUDIO_CODEC_AAC:
            codec_id = 10;
            break;
        case AUDIO_CODEC_G711_A:
            codec_id = 7;
            break;
        case AUDIO_CODEC_G711_U:
            codec_id = 8;
            break;
        default:
            break;
        }
        if (codec_id != 0xffffffff){
            s_amf_double(s, codec_id);
        }
    }
    s_amf_string(s, "filesize");
    s_amf_double(s, 0); // delayed write

    s_amf_string(s, "");
    s_w8(s, AMF_END_OF_OBJECT);
    serializer_array_get_data(s, &data, size);
    *output = memdup(data, *size);
    serializer_array_deinit(s);
    return 0;
}

static int write_header(struct serializer *s, bool has_audio, bool has_video)
{
    s_write(s, "FLV", 3); /* Signature */
    s_w8(s, 1);           /* Version */
    s_w8(s, has_audio * FLV_HEADER_FLAG_HASAUDIO + has_video * FLV_HEADER_FLAG_HASVIDEO);
                          /* Video/Audio */
    s_wb32(s, 9);         /* DataOffset */
    s_wb32(s, 0);         /* PreviousTagSize0 */

    return 0;
}

static int write_meta(struct serializer *s, struct flv_muxer *flv)
{
    uint8_t *meta = NULL;
    size_t meta_size;
    uint32_t start_pos;
    build_meta_data(flv, &meta, &meta_size);

    start_pos = s_getpos(s);

    s_w8(s, FLV_TAG_TYPE_META);
    s_wb24(s, meta_size);
    s_wb24(s, 0); /* time stamp */
    s_wb32(s, 0); /* reserved */
    s_write(s, meta, meta_size);
    s_wb32(s, (uint32_t)s_getpos(s) - start_pos);

    if (meta)
        free(meta);
    return 0;
}

static bool has_start_code(const uint8_t *data)
{
    if (data[0] != 0 || data[1] != 0)
        return false;

    return data[2] == 1 || (data[2] == 0 && data[3] == 1);
}

static const uint8_t *ff_avc_find_startcode_internal(const uint8_t *p,
                    const uint8_t *end)
{
    const uint8_t *a = p + 4 - ((intptr_t)p & 3);

    for (end -= 3; p < a && p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    for (end -= 3; p < end; p += 4) {
        uint32_t x = *(const uint32_t *)p;
        if ((x - 0x01010101) & (~x) & 0x80808080) {
            if (p[1] == 0) {
                if (p[0] == 0 && p[2] == 1)
                    return p;
                if (p[2] == 0 && p[3] == 1)
                    return p + 1;
            }

            if (p[3] == 0) {
                if (p[2] == 0 && p[4] == 1)
                    return p + 2;
                if (p[4] == 0 && p[5] == 1)
                    return p + 3;
            }
        }
    }

    for (end += 3; p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    return end + 3;
}

static const uint8_t *avc_find_startcode(const uint8_t *p, const uint8_t *end)
{
    const uint8_t *out = ff_avc_find_startcode_internal(p, end);
    if (p < out && out < end && !out[-1])
        out--;
    return out;
}

static void get_sps_pps(const uint8_t *data, size_t size, const uint8_t **sps,
                        size_t *sps_size, const uint8_t **pps, size_t *pps_size)
{
    const uint8_t *nal_start, *nal_end;
    const uint8_t *end = data + size;
    int type;

    nal_start = avc_find_startcode(data, end);
    while (true) {
        while (nal_start < end && !*(nal_start++))
            ;
        if (nal_start == end)
            break;
        nal_end = avc_find_startcode(nal_start, end);
        type = nal_start[0] & 0x1F;
        switch (type) {
        case H264_NAL_SPS:
            *sps = nal_start;
            *sps_size = nal_end - nal_start;
            break;
        case H264_NAL_PPS:
            *pps = nal_start;
            *pps_size = nal_end - nal_start;
            break;
        }

        nal_start = nal_end;
    }
}

static size_t parse_avc_header(const struct video_packet *src, struct video_packet *dst)
{
    uint8_t *header;
    struct serializer s;
    const uint8_t *sps = NULL, *pps = NULL;
    size_t sps_size = 0, pps_size = 0;

    const uint8_t *extra_data = src->encoder.extra_data;
    size_t extra_size = src->encoder.extra_size;
    serializer_array_init(&s);

    if (extra_size <= 6) {
        printf("%s:%d extra_size=%zu\n", __func__, __LINE__, extra_size);
        return 0;
    }

    if (!has_start_code(extra_data)) {
        dst->data = memdup(extra_data, extra_size);
        return extra_size;
    }

    get_sps_pps(extra_data, extra_size, &sps, &sps_size, &pps, &pps_size);
    if (!sps || !pps || sps_size < 4) {
        return 0;
    }

    s_w8(&s, 0x01);
    s_write(&s, sps + 1, 3);
    s_w8(&s, 0xff);
    s_w8(&s, 0xe1);

    s_wb16(&s, (uint16_t)sps_size);
    s_write(&s, sps, sps_size);
    s_w8(&s, 0x01);
    s_wb16(&s, (uint16_t)pps_size);
    s_write(&s, pps, pps_size);

    serializer_array_get_data(&s, &header, &extra_size);
    dst->data = memdup(header, extra_size);
    dst->size = extra_size;
    serializer_array_deinit(&s);
    memcpy(&dst->encoder, &src->encoder, sizeof(struct video_encoder));
    return extra_size;
}



static void serialize_avc_data(struct serializer *s, const uint8_t *data,
                size_t size, bool *is_keyframe)
{
    const uint8_t *nal_start, *nal_end;
    const uint8_t *end = data + size;
    int type;

    nal_start = avc_find_startcode(data, end);
    while (true) {
        while (nal_start < end && !*(nal_start++))
            ;
        if (nal_start == end)
            break;
        type = nal_start[0] & 0x1F;
        if (type == H264_NAL_IDR_SLICE || type == H264_NAL_SLICE) {
            if (is_keyframe)
                *is_keyframe = (type == H264_NAL_IDR_SLICE);
        }
        nal_end = avc_find_startcode(nal_start, end);
        s_wb32(s, (uint32_t)(nal_end - nal_start));
        s_write(s, nal_start, nal_end - nal_start);
        nal_start = nal_end;
    }

}

static void parse_avc_packet(const struct video_packet *src, struct video_packet *dst)
{
    uint8_t *data;
    size_t size;
    struct serializer s;

    serializer_array_init(&s);

    serialize_avc_data(&s, src->data, src->size, &dst->key_frame);
    serializer_array_get_data(&s, &data, &size);

    dst->data = memdup(data, size);
    dst->size = size;
    serializer_array_deinit(&s);
    memcpy(&dst->encoder, &src->encoder, sizeof(struct video_encoder));
}

int flv_write_packet(struct flv_muxer *flv, struct media_packet *pkt)
{
    uint8_t *data;
    size_t size;
    int strm_idx;
    struct serializer *s;
    struct video_packet *vpkt;

    if (!flv || !pkt) {
        printf("%s: invalid parameter!\n", __func__);
        return -1;
    }

    s = &flv->s;
    if (flv->is_header) {
        int has_audio = !!flv->audio;
        int has_video = !!flv->video;

        write_header(s, has_audio, has_video);
        write_meta(s, flv);

        if (has_video) {
            struct video_packet *vpkt = video_packet_create(MEDIA_MEM_DEEP, NULL, 0);
            parse_avc_header(pkt->video, vpkt);
            vpkt->key_frame = true;
            write_video(s, vpkt, 0, true);
            video_packet_destroy(vpkt);
        }
        if (has_audio) {
            write_audio(s, pkt->audio, 0, true);
        }
        flv->is_header = false;
    }

    switch (pkt->type) {
    case MEDIA_TYPE_VIDEO:
        strm_idx = 0;
        vpkt = video_packet_create(MEDIA_MEM_DEEP, NULL, 0);
        vpkt->key_frame = true;
        vpkt->dts = pkt->video->dts;
        vpkt->pts = pkt->video->pts;
        parse_avc_packet(pkt->video, vpkt);
        if (!flv->is_keyframe_got) {
            if (vpkt->key_frame) {
                flv->video->start_dts_offset = get_ms_time_v(pkt->video, pkt->video->dts);
                flv->is_keyframe_got = true;
            }
        }
        write_video(s, vpkt, flv->video->start_dts_offset, false);
        video_packet_destroy(vpkt);
        break;
    case MEDIA_TYPE_AUDIO:
        strm_idx = pkt->audio->track_idx;
        write_audio(s, pkt->audio, pkt->audio->encoder.start_dts_offset, false);
        break;
    case MEDIA_TYPE_SUBTITLE:
        //TODO
        //write_subtitle(s);
        break;
    default:
        break;
    }

    serializer_array_get_data(s, &data, &size);
    if (flv->output_cb) {
        strm_idx = 0;
        flv->output_cb(flv->output_cb_ctx, data, size, strm_idx);
    }
    serializer_array_reset(s);
    return 0;
}
