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

int rtmp_flush(struct rtmp *rtmp)
{
    if (RTMP_IsConnected(rtmp->base)) {

        int ret = RTMP_Write(rtmp->base, (const char *)rtmp->buf->data, rtmp->buf->d_cur);
        if (ret == -1) {
            printf("RTMP_Write() failed\n");
            return -1;
        }
        return 0;
    }
    printf("RTMP_IsConnected() == false, data discarded\n");
    return -1;
}

int flush_data_force(struct rtmp *rtmp, int sent)
{
    if (!rtmp->buf->d_cur) {
        return 0;
    }
    if (sent) {
        if (rtmp_flush(rtmp) < 0) {
            return -1;
        }
    }
    rtmp->buf->d_total += rtmp->buf->d_cur;
    rtmp->buf->d_cur = 0;
    return 0;
}

int flush_data(struct rtmp *rtmp, int sent)
{
    if (!rtmp->buf->d_cur) {
        printf("flush_data empty\n");
        return 0;
    }
    if (rtmp->buf->d_cur > 1024) {
        return flush_data_force(rtmp, sent);
    }
    return 0;
}
