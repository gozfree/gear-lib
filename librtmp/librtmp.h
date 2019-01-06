/******************************************************************************
 * Copyright (C) 2014-2018 Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
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
