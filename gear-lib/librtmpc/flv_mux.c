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
#include "flv_mux.h"
#include "libserializer.h"

static void put_amf_string(struct serializer *s, const char *str)
{
    size_t len = strlen(str);
    s_wb16(s, len);
    s_write(s, str, len);
}

static void put_amf_double(struct serializer *s, double d)
{
    s_w8(s, AMF_DATA_TYPE_NUMBER);
    s_wb64(s, dbl2int(d));
}

static int write_video(struct video_packet *pkt, bool is_header)
{
    int64_t offset = pkt->pts - pkt->dts;
    int32_t time_ms = get_ms_time2(pkt, pkt->dts) - rtmpc->start_dts_offset;

    s_w8(s, FLV_TAG_TYPE_VIDEO);

    s_wb24(s, pkt->size + 5);
    s_wb24(s, time_ms);
    s_w8(s, (time_ms >> 24) & 0x7f);
    s_wb24(s, 0);

    s_w8(s, FLV_CODECID_H264|(pkt->key_frame?FLV_FRAME_KEY:FLV_FRAME_INTER));
    s_w8(s, is_header? 0 : 1);
    s_wb24(s, get_ms_time2(pkt, offset));
    s_write(s, pkt->data, pkt->size);

    s_wb32(s, serializer_get_pos(s) - 1);
    return 0;
}

static int write_audio(struct rtmpc *rtmpc, struct audio_packet *pkt, bool is_header)
{
	int32_t time_ms = get_ms_time(packet, packet->dts) - dts_offset;
    s_w8(buf, FLV_TAG_TYPE_AUDIO);
    s_wb24(buf, pkt->size + 2);
    s_wb24(buf, time_ms);
    s_w8(buf, (time_ms >> 24)&0x7f);
    s_wb24(buf, 0);

    s_w8(buf, FLV_CODECID_AAC|FLV_SAMPLERATE_44100HZ|FLV_SAMPLESSIZE_16BIT|FLV_STEREO);
    s_w8(buf, is_header?0:1);
    s_write(buf, pkt->data, pkt->size);

    s_wb32(s, serializer_get_pos(s) - 1);

    return 0;
}

static bool build_flv_meta_data(uint8_t **output, size_t *size)
{
	struct serializer s;
	struct array_output_data data;

	array_output_serializer_init(&s, &data);
    /* write meta_tag */
    int metadata_size_pos;
    s_w8(s, FLV_TAG_TYPE_META); // tag type META
    metadata_size_pos = serializer_get_pos(s);
    s_wb24(s, 0); // size of data part (sum of all parts below)
    s_wb24(s, 0); // time stamp
    w_wb32(s, 0); // reserved

    /* now data of data_size size */
    /* first event name as a string */
    s_w8(s, AMF_DATA_TYPE_STRING);
    put_amf_string(s, "onMetaData"); // 12 bytes

    /* mixed array (hash) with size and string/type/data tuples */
    s_w8(s, AMF_DATA_TYPE_MIXEDARRAY);
    w_wb32(s, 5*video_exist + 5*audio_exist + 2); // +2 for duration and file size

    put_amf_string(s, "duration");
    put_amf_double(s, 0.0);

    if (video_exist) {
        put_amf_string(s, "width");
        put_amf_double(s, r->video->width);

        put_amf_string(s, "height");
        put_amf_double(s, r->video->height);

        put_amf_string(s, "videodatarate");
        put_amf_double(s, r->video->bitrate/1024.0);

        put_amf_string(s, "framerate");
        put_amf_double(s, r->video->framerate);

        put_amf_string(s, "videocodecid");
        put_amf_double(s, FLV_CODECID_H264);
    }

    if (audio_exist) {
        put_amf_string(s, "audiodatarate");
        put_amf_double(s, r->audio->bitrate /1024.0);

        put_amf_string(s, "audiosamplerate");
        put_amf_double(s, r->audio->sample_rate);

        put_amf_string(s, "audiosamplesize");
        put_amf_double(s, r->audio->sample_size);

        put_amf_string(s, "stereo");
        s_w8(s, AMF_DATA_TYPE_BOOL);
        s_w8(s, (r->audio->channels == 2));

        put_amf_string(s, "audiocodecid");
        unsigned int codec_id = 0xffffffff;

        switch (r->audio->codec_id) {
        case AUDIO_ENCODE_AAC:
            codec_id = 10;
            break;
        case AUDIO_ENCODE_G711_A:
            codec_id = 7;
            break;
        case AUDIO_ENCODE_G711_U:
            codec_id = 8;
            break;
        default:
            break;
        }
        if (codec_id != 0xffffffff){
            put_amf_double(s, codec_id);
        }
    }
    put_amf_string(s, "filesize");
    put_amf_double(s, 0); // delayed write

    put_amf_string(s, "");
    s_w8(s, AMF_END_OF_OBJECT);


}

static int flv_write_header(struct flv *flv)
{
    struct serializer *s = r->s;
    int audio_exist = !!r->audio;
    int video_exist = !!r->video;

	uint8_t *meta_data = NULL;
	size_t meta_data_size;

    /*==============================*/
    s_write(s, "FLV", 3); // Signature
    s_w8(s, 1);    // Version
    s_w8(s, !!flv->audio * FLV_HEADER_FLAG_HASAUDIO +
            !!flv->video * FLV_HEADER_FLAG_HASVIDEO);  //Video/Audio
    s_wb32(s, 9);    // DataOffset
    s_wb32(s, 0);    // PreviousTagSize0

    /*==============================*/
    build_flv_meta_data(flv, &meta_data, &meta_data_size);
    /* write total size of tag */
    int data_size= serializer_get_pos(s) - metadata_size_pos - 10;
    update_amf_be24(s, data_size, metadata_size_pos);
    w_wb32(s, data_size + 11);

    if (video_exist) {
        write_video(r, NULL, true);
    }
    if (audio_exist) {
        switch (r->audio->codec_id) {
        case AUDIO_ENCODE_AAC:
            write_audio(r, NULL, true);
            break;
        case AUDIO_ENCODE_G711_A:
        case AUDIO_ENCODE_G711_U:
        default:
            break;
        }
    }
    if (flush_data_force(r, 1) < 0){
        printf("flush_data_force FAILED\n");
        return -1;
    }
    return 0;
}

static int flv_write_packet(struct media_packet *pkt, uint64_t dts_base, uint8_t **output, size_t *size)
{
	struct darray_data data;
	struct serializer s;

    serializer_init(&s, &data);

    switch (pkt->type) {
    case MEDIA_TYPE_VIDEO:
        return write_video(pkt->video, false);
        break;
    case MEDIA_TYPE_AUDIO:
        return write_audio(rtmpc, pkt->audio, false);
        break;
    case MEDIA_TYPE_SUBTITLE:
    default:
        break;
    }

	*output = data.bytes.array;
	*size = data.bytes.num;
    return 0;

}
