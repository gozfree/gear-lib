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
#include "rtmp_util.h"
#include "rtmp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int append_data(struct rtmp_private_buf *buf, void *data, int size)
{
    int ns = buf->d_cur + size;
    if (ns > buf->d_max) {
        int dn = 16;
        while (ns > dn) {
#ifdef SYSTEM_MEM_SMALL
            dn += 1*1024;
#else
            dn <<= 1;
#endif
        }
#ifdef SYSTEM_MEM_SMALL
        if (dn > 512*1024) {
            printf("append_data:Failed to realloc 512KB buffer\n");
            return -1;
        }
#endif
        void *new_data = calloc(1, dn);
        if (!new_data) {
            printf("append_data:Failed to realloc\n");
            return -1;
        }
        memcpy(new_data, buf->data, buf->d_cur);
        free(buf->data);
        buf->data = new_data;
        buf->d_max = dn;
    }
    memcpy(buf->data + buf->d_cur, data, size);
    buf->d_cur = ns;
    return 0;
}

int put_byte(struct rtmp_private_buf *buf, uint8_t b)
{
    return append_data(buf, &b, 1);
}

int put_be32(struct rtmp_private_buf *buf, uint32_t val)
{
    int ret = 0;
    ret += put_byte(buf, val >> 24);
    ret += put_byte(buf, val >> 16);
    ret += put_byte(buf, val >> 8);
    ret += put_byte(buf, val);
    return ret;
}

int put_be64(struct rtmp_private_buf *buf, uint64_t val)
{
    int ret = 0;
    ret += put_be32(buf, val >> 32);
    ret += put_be32(buf, val);
    return ret;
}

int put_be16(struct rtmp_private_buf *buf, uint16_t val)
{
    int ret = 0;
    ret += put_byte(buf, val >> 8);
    ret += put_byte(buf, val);
    return ret;
}

int put_be24(struct rtmp_private_buf *buf, uint32_t val)
{
    int ret = 0;
    ret += put_be16(buf, val >> 8);
    ret += put_byte(buf, val);
    return ret;
}

int put_tag(struct rtmp_private_buf *buf, const char *tag)
{
    int ret = 0;
    while(*tag) {
        ret += put_byte(buf, *tag++);
    }
    return ret;
}

uint64_t dbl2int(double value)
{
    union tmp{double f; unsigned long long i;} u;
    u.f = value;
    return u.i;
}

int put_amf_string(struct rtmp_private_buf *buf, const char *str)
{
    int ret = 0;
    int len = strlen(str);
    ret += put_be16(buf, len);
    ret += append_data(buf, (void *)str, len);
    return ret;
}

int put_amf_double(struct rtmp_private_buf *buf, double d)
{
    int ret = 0;
    ret += put_byte(buf, 0/*AMF_DATA_TYPE_NUMBER*/);
    ret += put_be64(buf, dbl2int(d));
    return ret;
}

int update_amf_byte(struct rtmp_private_buf *buf, unsigned int value, unsigned int pos)
{
    *(buf->data + pos) = value;
    return 0;
}

int update_amf_be24(struct rtmp_private_buf *buf, uint32_t value, int pos)
{
    *(buf->data + pos + 0) = value >> 16;
    *(buf->data + pos + 1) = value >> 8;
    *(buf->data + pos + 2) = value >> 0;
    return 0;
}

int tell(struct rtmp_private_buf *buf)
{
    return buf->d_cur;
}

int rtmp_flush(struct rtmpc *rtmpc)
{
    if (RTMP_IsConnected(rtmpc->base)) {

        int ret = RTMP_Write(rtmpc->base, (const char *)rtmpc->priv_buf->data, rtmpc->priv_buf->d_cur, 0);
        if (ret == -1) {
            printf("RTMP_Write() failed\n");
            return -1;
        }
        return 0;
    }
    printf("RTMP_IsConnected() == false, data discarded\n");
    return -1;
}

int flush_data_force(struct rtmpc *rtmpc, int sent)
{
    if (!rtmpc->priv_buf->d_cur) {
        return 0;
    }
    if (sent) {
        if (rtmp_flush(rtmpc) < 0) {
            return -1;
        }
    }
    rtmpc->priv_buf->d_total += rtmpc->priv_buf->d_cur;
    rtmpc->priv_buf->d_cur = 0;
    return 0;
}

int flush_data(struct rtmpc *rtmpc, int sent)
{
    if (!rtmpc->priv_buf->d_cur) {
        printf("flush_data empty\n");
        return 0;
    }
    if (rtmpc->priv_buf->d_cur > 1024) {
        return flush_data_force(rtmpc, sent);
    }
    return 0;
}
