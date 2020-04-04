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
#include <gear-lib/libserializer.h>
#include <gear-lib/libmacro.h>
#include "flv_mux.h"
#include "rtmp_util.h"

#define AMF_END_OF_OBJECT         0x09

int flv_mux_init(struct flv_muxer *flv, flv_mux_output_cb *cb, void *cb_ctx)
{
    flv->output_cb = cb;
    flv->output_cb_ctx = cb_ctx;
    serializer_array_init(&flv->s);
    return 0;
}

void flv_mux_deinit(struct flv_muxer *flv)
{
    serializer_array_deinit(&flv->s);
    if (flv->audio)
        free(flv->audio);
    if (flv->video)
        free(flv->video);
    memset(flv, 0, sizeof(struct flv_muxer));
}

int flv_mux_add_media(struct flv_muxer *flv, struct media_packet *mp)
{
    switch (mp->type) {
    case MEDIA_TYPE_AUDIO:
        flv->audio = calloc(1, sizeof(struct audio_encoder));
        memcpy(flv->audio, &mp->audio->encoder, sizeof(struct audio_encoder));
        break;
    case MEDIA_TYPE_VIDEO:
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
        printf("flv->video->width = %d\n", flv->video->width);
        s_amf_string(s, "height");
        s_amf_double(s, flv->video->height);
        printf("flv->video->height = %d\n", flv->video->height);
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
        unsigned int codec_id = 0xffffffff;

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
	s_wb32(s, (uint32_t)s_getpos(s) - start_pos - 1);

    if (meta)
        free(meta);
    return 0;
}

int flv_write_header(struct flv_muxer *flv, struct media_packet *pkt)
{
    uint8_t *data;
    size_t size;
    int strm_idx;
    struct serializer *s = &flv->s;
    int has_audio = !!flv->audio;
    int has_video = !!flv->video;

    write_header(s, has_audio, has_video);

    write_meta(s, flv);

    if (has_video) {
        write_video(s, NULL, 0, true);
    }
    if (has_audio) {
        write_audio(s, NULL, 0, true);
    }

    serializer_array_get_data(s, &data, &size);
    if (flv->output_cb) {
        strm_idx = 0;
        flv->output_cb(flv->output_cb_ctx, data, size, strm_idx);
    }
    return 0;
}

int flv_write_packet(struct flv_muxer *flv, struct media_packet *pkt)
{
    uint8_t *data;
    size_t size;
    int strm_idx;
    struct serializer *s = &flv->s;

    switch (pkt->type) {
    case MEDIA_TYPE_VIDEO:
        strm_idx = 0;
        write_video(s, pkt->video, pkt->video->encoder.start_dts_offset, false);
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
        flv->output_cb(flv->output_cb_ctx, data, size, strm_idx);
    }
    serializer_array_reset(s);
    return 0;
}
