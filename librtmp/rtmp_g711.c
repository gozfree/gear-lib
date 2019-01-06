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
#include "rtmp_g711.h"
#include "rtmp_util.h"
#include <string.h>

int g711_write_header(struct rtmp *rtmp)
{
    return 0;
}

int write_g711_frame(struct rtmp *rtmp, unsigned char *frame,int len,unsigned int timestamp)
{
    struct rtmp_private_buf *buf = rtmp->buf;
    put_byte(buf, FLV_TAG_TYPE_AUDIO);
    put_be24(buf, len + 1);
    put_be24(buf, timestamp);
    put_byte(buf, (timestamp >> 24)&0x7f);
    put_be24(buf, 0);//streamId, always 0

    int flags = FLV_SAMPLERATE_SPECIAL | FLV_SAMPLESSIZE_16BIT | FLV_MONO;
    if (rtmp->audio->codec_id == RTMP_DATA_G711_A) {
        flags |= FLV_CODECID_G711_ALAW;
    } else {
        flags |= FLV_CODECID_G711_MULAW;
    }
    put_byte(buf, flags);
    append_data(buf, frame, len);
    put_be32(buf, 11 + len  + 1);
    return 0;
}

static unsigned char g711_buffer[160];
static int g711_data_size;
int g711_write_packet(struct rtmp *rtmp, struct rtmp_packet *pkt)
{
    static const int g711_frame_size = 80; //10 ms
    unsigned int timestamp = pkt->timestamp;
    int remained_len = pkt->len;
    unsigned char *data = pkt->data;
    if (g711_data_size) {
        memcpy(&g711_buffer[g711_data_size],data,g711_frame_size -g711_data_size);
        write_g711_frame(rtmp, g711_buffer, g711_frame_size, timestamp);
        remained_len -= g711_frame_size -g711_data_size;
        data += g711_frame_size -g711_data_size;
        g711_data_size = 0;
    }
    while (remained_len >= g711_frame_size) {
        write_g711_frame(rtmp, data, g711_frame_size, timestamp);
        remained_len -= g711_frame_size;
        data += g711_frame_size;
    }
    if (remained_len) {
        memcpy(g711_buffer,data,remained_len);
        g711_data_size = remained_len;
    }

    if (flush_data(rtmp, 1)) {
        return -1;
    }
    return 0;
}
