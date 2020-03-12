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
#include "rtmp_g711.h"
#include "rtmp_util.h"
#include <string.h>

int g711_write_header(struct rtmpc *rtmpc)
{
    return 0;
}

int write_g711_frame(struct rtmpc *rtmpc, unsigned char *frame,int len,unsigned int timestamp)
{
    struct rtmp_private_buf *buf = rtmpc->priv_buf;
    put_byte(buf, FLV_TAG_TYPE_AUDIO);
    put_be24(buf, len + 1);
    put_be24(buf, timestamp);
    put_byte(buf, (timestamp >> 24)&0x7f);
    put_be24(buf, 0);//streamId, always 0

    int flags = FLV_SAMPLERATE_SPECIAL | FLV_SAMPLESSIZE_16BIT | FLV_MONO;
    if (rtmpc->audio->codec_id == AUDIO_ENCODE_G711_A) {
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
int g711_write_packet(struct rtmpc *rtmpc, struct audio_packet *pkt)
{
    static const int g711_frame_size = 80; //10 ms
    unsigned int timestamp = pkt->pts;
    int remained_len = pkt->size;
    unsigned char *data = pkt->data;
    if (g711_data_size) {
        memcpy(&g711_buffer[g711_data_size],data,g711_frame_size -g711_data_size);
        write_g711_frame(rtmpc, g711_buffer, g711_frame_size, timestamp);
        remained_len -= g711_frame_size -g711_data_size;
        data += g711_frame_size -g711_data_size;
        g711_data_size = 0;
    }
    while (remained_len >= g711_frame_size) {
        write_g711_frame(rtmpc, data, g711_frame_size, timestamp);
        remained_len -= g711_frame_size;
        data += g711_frame_size;
    }
    if (remained_len) {
        memcpy(g711_buffer,data,remained_len);
        g711_data_size = remained_len;
    }

    if (flush_data(rtmpc, 1)) {
        return -1;
    }
    return 0;
}
