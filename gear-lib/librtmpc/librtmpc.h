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
#ifndef LIBRTMPC_H
#define LIBRTMPC_H

#include <gear-lib/libqueue.h>
#include <gear-lib/libthread.h>
#include <gear-lib/libmedia-io.h>
#include <stdio.h>
#include <stdint.h>
#if defined (__linux__) || defined (__CYGWIN__)
#include <stdbool.h>
#include <sys/uio.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

struct rtmp_video_params {
    int codec_id;
    int width;
    int height;
    double framerate;
    int bitrate;
    uint8_t *extra_data;
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

struct rtmpc {
    void *base;
    struct rtmp_video_params *video;
    struct rtmp_audio_params *audio;
    struct rtmp_private_buf *priv_buf;
    struct queue *q;
    struct iovec tmp_buf;
    struct thread *thread;
    bool is_run;
    bool is_start;
    bool is_keyframe_got;
    bool sent_headers;
    uint64_t start_dts_offset;
    uint32_t prev_msec;
    uint32_t prev_timestamp;
};

struct rtmpc *rtmp_create(const char *push_url);
int rtmp_stream_add(struct rtmpc *rtmpc, struct media_packet *pkt);
int rtmp_stream_start(struct rtmpc *rtmpc);
void rtmp_stream_stop(struct rtmpc *rtmpc);
int rtmp_send_packet(struct rtmpc *rtmpc, struct media_packet *pkt);
void rtmp_destroy(struct rtmpc *rtmpc);




#ifdef __cplusplus
}
#endif
#endif
