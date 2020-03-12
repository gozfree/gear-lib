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
#ifndef RTMP_H264_H
#define RTMP_H264_H

#include "librtmpc.h"
#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SYSTEM_MEM_SMALL
#define MAX_NALS_LEN    (320* 1024)
#define MAX_DATA_LEN    (64* 1024)
#else
#define MAX_NALS_LEN    (4*1024*1024)
#define MAX_DATA_LEN    (320* 1024)
#endif


struct rtmp_h264_info {
    int width;
    int height;
    int time_base_num;
    int time_base_den;
};

int h264_add(struct rtmpc *rtmpc, struct video_packet *pkt);
int h264_write_header(struct rtmpc *rtmpc);
int h264_write_packet(struct rtmpc *rtmpc, struct video_packet *pkt);
int h264_send_packet(struct rtmpc *rtmpc, struct video_packet *pkt);

#ifdef __cplusplus
}
#endif
#endif
