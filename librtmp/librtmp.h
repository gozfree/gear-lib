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
#ifndef LIBRTMP_H
#define LIBRTMP_H

#include <libqueue.h>
#include <libthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

enum rtmp_data_type {
    RTMP_DATA_H264,
    RTMP_DATA_AAC,
    RTMP_DATA_G711_A,
    RTMP_DATA_G711_U,
};

struct rtmp_video_params {
    int codec_id;
    int width;
    int height;
    double framerate;
    int bitrate;
    uint8_t extra[128];
    int extra_data_len;
};

struct rtmp_audio_params {
    int codec_id;
    int bitrate;
    int sample_rate;
    int sample_size;
    int channels;
    uint8_t *extra_data;
    int extra_data_len;
};

struct rtmp_private_buf {
    uint8_t *data;
    int d_cur;
    int d_max;
    uint64_t d_total;
};

struct rtmp {
    void *base;
    struct rtmp_video_params *video;
    struct rtmp_audio_params *audio;
    struct rtmp_private_buf *buf;
    struct queue *q;
    struct iovec tmp_buf;
    struct thread *thread;
    bool is_run;
    bool is_keyframe_got;
    uint32_t prev_msec;
    uint32_t prev_timestamp;
};

struct rtmp_packet {
    int type;
    uint8_t *data;
    int len;
    uint32_t timestamp;
    int key_frame;
    struct rtmp *parent;
};

struct rtmp *rtmp_create(const char *push_url);
int rtmp_stream_add(struct rtmp *rtmp, enum rtmp_data_type type, struct iovec *data);
int rtmp_stream_start(struct rtmp *rtmp);
void rtmp_stream_stop(struct rtmp *rtmp);
int rtmp_stream_pause(struct rtmp *rtmp);
int rtmp_send_data(struct rtmp *rtmp, enum rtmp_data_type type, uint8_t *data, int len, uint32_t timestamp);
void rtmp_destroy(struct rtmp *rtmp);

#ifdef __cplusplus
}
#endif
#endif
