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
#include "librtmp.h"
#include "rtmp_util.h"
#include "rtmp_h264.h"
#include "rtmp_aac.h"
#include "rtmp_g711.h"
#include "rtmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define RTMP_PKT_SIZE   1408

#define AMF_END_OF_OBJECT         0x09


void rtmp_destroy(struct rtmp *rtmp)
{
    if (!rtmp) {
        return;
    }
    RTMP_Close(rtmp->base);
    RTMP_Free(rtmp->base);
    rtmp->base = NULL;
    free(rtmp->video);
    free(rtmp->audio);
    queue_destroy(rtmp->q);
    free(rtmp->tmp_buf.iov_base);
    free(rtmp);
}

static void *item_alloc_hook(void *data, size_t len)
{
    struct rtmp_packet *pkt = (struct rtmp_packet *)calloc(1, sizeof(struct rtmp_packet));
    if (!pkt) {
        printf("calloc packet failed!\n");
        return NULL;
    }
    int alloc_size = (len + 15)/16*16;
    pkt->data = (uint8_t *)calloc(1,alloc_size);
    if (!pkt->data) {
        printf("calloc packet data failed!\n");
        free(pkt);
        return NULL;
    }
    memcpy(pkt->data, data, len);
    pkt->len = len;
    return pkt;
}

static void item_free_hook(void *data)
{
    struct rtmp_packet *pkt = (struct rtmp_packet *)data;
    if (pkt) {
        if (pkt->data) {
            free(pkt->data);
            pkt->data = NULL;
        }
        free(pkt);
        pkt = NULL;
    }
}

struct rtmp *rtmp_create(const char *url)
{
    int protocol = RTMP_PROTOCOL_UNDEFINED;
    AVal hostname = { 0, 0 };
    unsigned int port = 1935;
    AVal playpath = { 0, 0 };
    AVal app = { 0, 0 };
    AVal tcUrl = { 0, 0 };
    AVal swfUrl = { 0, 0 };
    AVal auth = { 0, 0 };
    AVal swfHash = { 0, 0 };
    AVal flashVer = { 0, 0 };
    AVal subscribepath = { 0, 0 };
    AVal usherToken = { 0, 0 };
    AVal sockshost = { 0, 0 };
    AVal pageUrl = { 0, 0 };
    unsigned int  swfSize = 0;
    unsigned int dSeek = 0;
    unsigned int dStopOffset = 0;
    struct rtmp *rtmp = (struct rtmp *)calloc(1, sizeof(struct rtmp));
    if (!rtmp) {
        printf("malloc rtmp failed!\n");
        goto failed;
    }
    RTMP *base = RTMP_Alloc();
    if (!base) {
        printf("RTMP_Alloc failed!\n");
        goto failed;
    }
    RTMP_Init(base);
    if (!RTMP_ParseURL(url, &protocol, &hostname, &port, &playpath, &app)) {
        printf("Couldn't parse the url %s", url);
        goto failed;
    }
    if (tcUrl.av_len == 0) {
        char str[512] = { 0 };
        tcUrl.av_len = snprintf(str, 511, "%s://%.*s:%d/%.*s",RTMPProtocolStringsLower[protocol], hostname.av_len,hostname.av_val, port, app.av_len, app.av_val);
        tcUrl.av_val = strdup(str);
    }
    RTMP_SetupStream(base, protocol, &hostname, port, &sockshost, &playpath,
                      &tcUrl, &swfUrl, &pageUrl, &app, &auth, &swfHash, swfSize,
                      &flashVer, &subscribepath,&usherToken, dSeek, dStopOffset, true, 30);
    uint32_t bufferTime = 10 * 3600 * 1000;
    RTMP_SetBufferMS(base, bufferTime);
    RTMP_EnableWrite(base);
    if (!RTMP_Connect(base, NULL)) {
        printf("RTMP_Connect failed!\n");
        goto failed;
    }
    if (!RTMP_ConnectStream(base, dSeek)){
        printf("RTMP_ConnectStream failed!\n");
        goto failed;
    }

    RTMPPacket pack;
    RTMPPacket_Alloc(&pack, 4);
    pack.m_packetType = RTMP_PACKET_TYPE_CHUNK_SIZE;
    pack.m_nChannel = 0x02;
    pack.m_headerType = RTMP_PACKET_SIZE_LARGE;
    pack.m_nTimeStamp = 0;
    pack.m_nInfoField2 = 0;
    pack.m_nBodySize = 4;
    int nVal = RTMP_PKT_SIZE;
    pack.m_body[3] = nVal & 0xff;
    pack.m_body[2] = nVal >> 8;
    pack.m_body[1] = nVal >> 16;
    pack.m_body[0] = nVal >> 24;
    base->m_outChunkSize = nVal;
    if (FALSE == RTMP_SendPacket(base, &pack, 1)) {
        printf("RTMP_SendPacket failed!\n");
        goto failed;
    }
    RTMPPacket_Free(&pack);

    rtmp->buf = calloc(1, sizeof(struct rtmp_private_buf));
    if (!rtmp->buf) {
        printf("malloc private buffer failed!\n");
        goto failed;
    }
    rtmp->buf->data = calloc(1, MAX_DATA_LEN);
    if (!rtmp->buf->data) {
        printf("malloc private buffer failed!\n");
        goto failed;
    }
    rtmp->buf->d_max = MAX_DATA_LEN;
    rtmp->q = queue_create();
    if (!rtmp->q) {
        printf("queue_create failed!\n");
        goto failed;
    }
    queue_set_hook(rtmp->q, item_alloc_hook, item_free_hook, NULL, NULL);
    rtmp->tmp_buf.iov_len = MAX_NALS_LEN;
    rtmp->tmp_buf.iov_base = calloc(1, MAX_NALS_LEN);
    if (!rtmp->tmp_buf.iov_base) {
        printf("malloc tmp buf failed!\n");
        goto failed;
    }
    rtmp->base = base;
    rtmp->is_run = false;
    rtmp->is_keyframe_got = false;
    rtmp->prev_msec = 0;
    rtmp->prev_timestamp = 0;
    return rtmp;

failed:
    if (rtmp) {
        if (rtmp->tmp_buf.iov_base) {
            free(rtmp->tmp_buf.iov_base);
        }
        if (rtmp->q) {
            queue_destroy(rtmp->q);
        }
        if (rtmp->buf->data) {
            free(rtmp->buf->data);
        }
        free(rtmp);
        RTMPPacket_Free(&pack);
    }
    return NULL;
}

int rtmp_stream_add(struct rtmp *rtmp, enum rtmp_data_type type, struct iovec *data)
{
    int ret = 0;
    switch (type) {
    case RTMP_DATA_H264:
        ret = h264_add(rtmp, data);
        break;
    case RTMP_DATA_AAC:
        ret = aac_add(rtmp, data);
        break;
    default:
        break;
    }
    return ret;
}

int write_header(struct rtmp *rtmp)
{
    int audio_exist = !!rtmp->audio;
    int video_exist = !!rtmp->video;
    struct rtmp_private_buf *buf = rtmp->buf;

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
        put_amf_double(buf, rtmp->video->width);

        put_amf_string(buf, "height");
        put_amf_double(buf, rtmp->video->height);

        put_amf_string(buf, "videodatarate");
        put_amf_double(buf, rtmp->video->bitrate/1024.0);

        put_amf_string(buf, "framerate");
        put_amf_double(buf, 0/*video->framerate*/);//TODO

        put_amf_string(buf, "videocodecid");
        put_amf_double(buf, FLV_CODECID_H264);
    }

    if (audio_exist) {
        put_amf_string(buf, "audiodatarate");
        put_amf_double(buf, rtmp->audio->bitrate /1024.0);

        put_amf_string(buf, "audiosamplerate");
        put_amf_double(buf, rtmp->audio->sample_rate);

        put_amf_string(buf, "audiosamplesize");
        put_amf_double(buf, rtmp->audio->sample_size);

        put_amf_string(buf, "stereo");
        put_byte(buf, AMF_DATA_TYPE_BOOL);
        put_byte(buf, !!(rtmp->audio->channels == 2));

        put_amf_string(buf, "audiocodecid");
        unsigned int codec_id = 0xffffffff;

        switch (rtmp->audio->codec_id) {
        case RTMP_DATA_AAC:
            codec_id = 10;
            break;
        case RTMP_DATA_G711_A:
            codec_id = 7;
            break;
        case RTMP_DATA_G711_U:
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
        h264_write_header(rtmp);
    }
    if (audio_exist) {
        switch (rtmp->audio->codec_id) {
        case RTMP_DATA_AAC:
            aac_write_header(rtmp);
            break;
        case RTMP_DATA_G711_A:
        case RTMP_DATA_G711_U:
            g711_write_header(rtmp);
            break;
        default:
            break;
        }
    }
    if (flush_data_force(rtmp, 1) < 0){
        printf("flush_data_force FAILED\n");
        return -1;
    }
    return 0;
}

int write_packet(struct rtmp *rtmp, struct rtmp_packet *pkt)
{
    switch (pkt->type) {
    case RTMP_DATA_H264:
        return h264_write_packet(rtmp, pkt);
        break;
    case RTMP_DATA_AAC:
        return aac_write_packet(rtmp, pkt);
        break;
    case RTMP_DATA_G711_A:
    case RTMP_DATA_G711_U:
        return g711_write_packet(rtmp, pkt);
        break;
    }
    return 0;
}

int rtmp_send_data(struct rtmp *rtmp, enum rtmp_data_type type, uint8_t *data, int len, uint32_t timestamp)
{
    int ret = 0;
    switch (type) {
    case RTMP_DATA_H264:
        ret = h264_send_data(rtmp, data, len, timestamp);
        break;
    case RTMP_DATA_AAC:
        ret = aac_send_data(rtmp, data, len, timestamp);
        break;
    default:
        ret = -1;
        break;
    }
    return ret;
}

static void *rtmp_stream_thread(struct thread *t, void *arg)
{
    struct rtmp *rtmp = (struct rtmp *)arg;
    if (0 != write_header(rtmp)) {
        printf("write_header failed!\n");
        return NULL;
    }
    queue_flush(rtmp->q);
    rtmp->is_run = true;
    while (rtmp->is_run) {
        struct item *it = queue_pop(rtmp->q);
        if (!it) {
            usleep(200000);
            continue;
        }
        struct rtmp_packet *pkt = (struct rtmp_packet *)it->opaque.iov_base;
        if (0 != write_packet(rtmp, pkt)) {
            printf("write_packet failed!\n");
            rtmp->is_run = false;
        }
        item_free(rtmp->q, it);
    }
    return NULL;
}

void rtmp_stream_stop(struct rtmp *rtmp)
{
    if (rtmp) {
        thread_destroy(rtmp->thread);
        rtmp->thread = NULL;
    }
}

int rtmp_stream_start(struct rtmp *rtmp)
{
    if (!rtmp) {
        return -1;
    }
    rtmp->thread = thread_create(rtmp_stream_thread, rtmp);
    if (!rtmp->thread) {
        printf("thread_create failed!\n");
        return -1;
    }
    return 0;
}
