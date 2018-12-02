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
#ifndef RTMP_H264_H
#define RTMP_H264_H

#include "librtmp.h"
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

int h264_send_data(struct rtmp *rtmp, uint8_t *data, int len, uint32_t timestamp);
int h264_add(struct rtmp *rtmp, struct iovec *data);
int h264_write_header(struct rtmp *rtmp);
int h264_write_packet(struct rtmp *rtmp, struct rtmp_packet *pkt);

#ifdef __cplusplus
}
#endif
#endif
