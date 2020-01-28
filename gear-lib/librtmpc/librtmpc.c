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
#include "librtmpc.h"
#include "rtmp_util.h"
#include "rtmp_h264.h"
#include "rtmp_aac.h"
#include "rtmp_g711.h"
#include "rtmp.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define RTMP_PKT_SIZE   1408

#define AMF_END_OF_OBJECT         0x09


void rtmp_destroy(struct rtmpc *rtmpc)
{
    if (!rtmpc) {
        return;
    }
    RTMP_Close(rtmpc->base);
    RTMP_Free(rtmpc->base);
    rtmpc->base = NULL;
    free(rtmpc->video);
    free(rtmpc->audio);
    queue_destroy(rtmpc->q);
    free(rtmpc->tmp_buf.iov_base);
    free(rtmpc);
}

static void *item_alloc_hook(void *data, size_t len, void *arg)
{
    struct media_packet *pkt = (struct media_packet *)arg;
    if (!pkt) {
        printf("calloc packet failed!\n");
        return NULL;
    }
    int alloc_size = (len + 15)/16*16;
    switch (pkt->type) {
    case MEDIA_PACKET_AUDIO:
        pkt->audio->data = calloc(1, alloc_size);
        memcpy(pkt->audio->data, data, len);
        pkt->audio->size = len;
        break;
    case MEDIA_PACKET_VIDEO:
        pkt->video->data = calloc(1, alloc_size);
        memcpy(pkt->video->data, data, len);
        pkt->video->size = len;
        break;
    default:
        printf("item alloc unsupport type %d\n", pkt->type);
        break;
    }

    return pkt;
}

static void item_free_hook(void *data)
{
    struct media_packet *pkt = (struct media_packet *)data;
    media_packet_destroy(pkt);
}

struct rtmpc *rtmp_create(const char *url)
{
    struct rtmpc *rtmpc = (struct rtmpc *)calloc(1, sizeof(struct rtmpc));
    if (!rtmpc) {
        printf("malloc rtmpc failed!\n");
        goto failed;
    }
    RTMP *base = RTMP_Alloc();
    if (!base) {
        printf("RTMP_Alloc failed!\n");
        goto failed;
    }
    RTMP_Init(base);
    RTMP_LogSetLevel(RTMP_LOGINFO);

    if (!RTMP_SetupURL(base, (char *)url)) {
        printf("RTMP_SetupURL failed!\n");
        goto failed;
    }

    RTMP_EnableWrite(base);

    RTMP_AddStream(base, NULL);

    if (!RTMP_Connect(base, NULL)) {
        printf("RTMP_Connect failed!\n");
        goto failed;
    }
    if (!RTMP_ConnectStream(base, 0)){
        printf("RTMP_ConnectStream failed!\n");
        goto failed;
    }

    rtmpc->priv_buf = calloc(1, sizeof(struct rtmp_private_buf));
    if (!rtmpc->priv_buf) {
        printf("malloc private buffer failed!\n");
        goto failed;
    }
    rtmpc->priv_buf->data = calloc(1, MAX_DATA_LEN);
    if (!rtmpc->priv_buf->data) {
        printf("malloc private buffer failed!\n");
        goto failed;
    }
    rtmpc->priv_buf->d_max = MAX_DATA_LEN;
    rtmpc->q = queue_create();
    if (!rtmpc->q) {
        printf("queue_create failed!\n");
        goto failed;
    }
    queue_set_hook(rtmpc->q, item_alloc_hook, item_free_hook);
    rtmpc->tmp_buf.iov_len = MAX_NALS_LEN;
    rtmpc->tmp_buf.iov_base = calloc(1, MAX_NALS_LEN);
    if (!rtmpc->tmp_buf.iov_base) {
        printf("malloc tmp buf failed!\n");
        goto failed;
    }
    rtmpc->base = base;
    rtmpc->is_run = false;
    rtmpc->is_start = false;
    rtmpc->is_keyframe_got = false;
    rtmpc->prev_msec = 0;
    rtmpc->prev_timestamp = 0;
    return rtmpc;

failed:
    if (rtmpc) {
        if (rtmpc->tmp_buf.iov_base) {
            free(rtmpc->tmp_buf.iov_base);
        }
        if (rtmpc->q) {
            queue_destroy(rtmpc->q);
        }
        if (rtmpc->priv_buf) {
            if (rtmpc->priv_buf->data) {
                free(rtmpc->priv_buf->data);
            }
            free(rtmpc->priv_buf);
        }
        free(rtmpc);
    }
    return NULL;
}

int rtmp_stream_add(struct rtmpc *rtmpc, struct media_packet *pkt)
{
    int ret = 0;
    switch (pkt->type) {
    case MEDIA_PACKET_VIDEO:
        ret = h264_add(rtmpc, pkt->video);
        break;
    case MEDIA_PACKET_AUDIO:
        ret = aac_add(rtmpc, pkt->audio);
        break;
    default:
        break;
    }
    return ret;
}

int rtmp_write_header(struct rtmpc *rtmpc)
{
    int audio_exist = !!rtmpc->audio;
    int video_exist = !!rtmpc->video;
    struct rtmp_private_buf *buf = rtmpc->priv_buf;

    put_tag(buf, "FLV"); // Signature
    put_byte(buf, 1);    // Version
    int flag = audio_exist * FLV_HEADER_FLAG_HASAUDIO + video_exist * FLV_HEADER_FLAG_HASVIDEO;
    put_byte(buf, flag);  //Video/Audio
    put_be32(buf, 9);    // DataOffset
    put_be32(buf, 0);    // PreviousTagSize0

    /* write meta_tag */
    int metadata_size_pos;
    put_byte(buf, FLV_TAG_TYPE_META); // tag type META
    metadata_size_pos = tell(buf);
    put_be24(buf, 0); // size of data part (sum of all parts below)
    put_be24(buf, 0); // time stamp
    put_be32(buf, 0); // reserved

    /* now data of data_size size */
    /* first event name as a string */
    put_byte(buf, AMF_DATA_TYPE_STRING);
    put_amf_string(buf, "onMetaData"); // 12 bytes

    /* mixed array (hash) with size and string/type/data tuples */
    put_byte(buf, AMF_DATA_TYPE_MIXEDARRAY);
    put_be32(buf, 5*video_exist + 5*audio_exist + 2); // +2 for duration and file size

    put_amf_string(buf, "duration");
    put_amf_double(buf, 0/*s->duration / AV_TIME_BASE*/); // fill in the guessed duration, it'll be corrected later if incorrect

    if (video_exist) {
        put_amf_string(buf, "width");
        put_amf_double(buf, rtmpc->video->width);

        put_amf_string(buf, "height");
        put_amf_double(buf, rtmpc->video->height);

        put_amf_string(buf, "videodatarate");
        put_amf_double(buf, rtmpc->video->bitrate/1024.0);

        put_amf_string(buf, "framerate");
        put_amf_double(buf, 0/*video->framerate*/);//TODO

        put_amf_string(buf, "videocodecid");
        put_amf_double(buf, FLV_CODECID_H264);
    }

    if (audio_exist) {
        put_amf_string(buf, "audiodatarate");
        put_amf_double(buf, rtmpc->audio->bitrate /1024.0);

        put_amf_string(buf, "audiosamplerate");
        put_amf_double(buf, rtmpc->audio->sample_rate);

        put_amf_string(buf, "audiosamplesize");
        put_amf_double(buf, rtmpc->audio->sample_size);

        put_amf_string(buf, "stereo");
        put_byte(buf, AMF_DATA_TYPE_BOOL);
        put_byte(buf, !!(rtmpc->audio->channels == 2));

        put_amf_string(buf, "audiocodecid");
        unsigned int codec_id = 0xffffffff;

        switch (rtmpc->audio->codec_id) {
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
            put_amf_double(buf, codec_id);
        }
    }
    put_amf_string(buf, "filesize");
    put_amf_double(buf, 0); // delayed write

    put_amf_string(buf, "");
    put_byte(buf, AMF_END_OF_OBJECT);

    /* write total size of tag */
    int data_size= tell(buf) - metadata_size_pos - 10;
    update_amf_be24(buf, data_size, metadata_size_pos);
    put_be32(buf, data_size + 11);

    if (video_exist) {
        h264_write_header(rtmpc);
    }
    if (audio_exist) {
        switch (rtmpc->audio->codec_id) {
        case AUDIO_ENCODE_AAC:
            aac_write_header(rtmpc);
            break;
        case AUDIO_ENCODE_G711_A:
        case AUDIO_ENCODE_G711_U:
            g711_write_header(rtmpc);
            break;
        default:
            break;
        }
    }
    if (flush_data_force(rtmpc, 1) < 0){
        printf("flush_data_force FAILED\n");
        return -1;
    }
    return 0;
}

static int write_packet(struct rtmpc *rtmpc, struct media_packet *pkt)
{
    switch (pkt->type) {
    case MEDIA_PACKET_VIDEO:
        return h264_write_packet(rtmpc, pkt->video);
        break;
    case MEDIA_PACKET_AUDIO:
        return aac_write_packet(rtmpc, pkt->audio);
        break;
#if 0
    case RTMP_DATA_G711_A:
    case RTMP_DATA_G711_U:
        return g711_write_packet(rtmpc, pkt->audio);
        break;
#endif
    }
    return 0;
}

int rtmp_send_packet(struct rtmpc *rtmpc, struct media_packet *pkt)
{
    int ret = 0;
    switch (pkt->type) {
    case MEDIA_PACKET_VIDEO:
        ret = h264_send_packet(rtmpc, pkt->video);
        break;
    case MEDIA_PACKET_AUDIO:
        ret = aac_send_packet(rtmpc, pkt->audio);
        break;
    default:
        ret = -1;
        break;
    }
    return ret;
}

static void *rtmp_stream_thread(struct thread *t, void *arg)
{
    struct rtmpc *rtmpc = (struct rtmpc *)arg;
    queue_flush(rtmpc->q);
    rtmpc->is_run = true;
    while (rtmpc->is_run) {
        struct item *it = queue_pop(rtmpc->q);
        if (!it) {
            usleep(200000);
            continue;
        }
        if (!rtmpc->sent_headers) {
            rtmp_write_header(rtmpc);
            rtmpc->sent_headers = true;
        }
        struct media_packet *pkt = (struct media_packet *)it->opaque.iov_base;
        if (0 != write_packet(rtmpc, pkt)) {
            printf("write_packet failed!\n");
            rtmpc->is_run = false;
        }
        item_free(rtmpc->q, it);
    }
    return NULL;
}

void rtmp_stream_stop(struct rtmpc *rtmpc)
{
    if (rtmpc) {
        thread_destroy(rtmpc->thread);
        rtmpc->thread = NULL;
        rtmpc->is_start = false;
    }
}

int rtmp_stream_start(struct rtmpc *rtmpc)
{
    if (!rtmpc) {
        return -1;
    }
    if (rtmpc->is_start) {
        printf("rtmpc stream already start!\n");
        return -1;
    }
    rtmpc->thread = thread_create(rtmp_stream_thread, rtmpc);
    if (!rtmpc->thread) {
        rtmpc->is_start = false;
        printf("thread_create failed!\n");
        return -1;
    }
    rtmpc->is_start = true;
    return 0;
}
