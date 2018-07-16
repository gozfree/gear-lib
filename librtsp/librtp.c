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
#include "librtp.h"
#include <libskt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libmacro.h>

#define RTP_VERSION      2
#define RTP_FIXED_HEADER 12

#define RTP_V(v)    ((v >> 30) & 0x03)   /* protocol version */
#define RTP_P(v)    ((v >> 29) & 0x01)   /* padding flag */
#define RTP_X(v)    ((v >> 28) & 0x01)   /* header extension flag */
#define RTP_CC(v)   ((v >> 24) & 0x0F)   /* CSRC count */
#define RTP_M(v)    ((v >> 23) & 0x01)   /* marker bit */
#define RTP_PT(v)   ((v >> 16) & 0x7F)   /* payload type */
#define RTP_SEQ(v)  ((v >> 00) & 0xFFFF) /* sequence number */

#define RTCP_V(v)   ((v >> 30) & 0x03) // rtcp version
#define RTCP_P(v)   ((v >> 29) & 0x01) // rtcp padding
#define RTCP_RC(v)  ((v >> 24) & 0x1F) // rtcp reception report count
#define RTCP_PT(v)  ((v >> 16) & 0xFF) // rtcp packet type
#define RTCP_LEN(v) (v & 0xFFFF) // rtcp packet length


#define nbo_r16 rtp_read_uint16
#define nbo_r32 rtp_read_uint32
#define nbo_w16 rtp_write_uint16
#define nbo_w32 rtp_write_uint32

static uint16_t g_base_port = 20000;//must even data

static inline uint16_t rtp_read_uint16(const uint8_t* ptr)
{
    return (((uint16_t)ptr[0]) << 8) | ptr[1];
}

static inline uint32_t rtp_read_uint32(const uint8_t* ptr)
{
    return (((uint32_t)ptr[0]) << 24) | (((uint32_t)ptr[1]) << 16) | (((uint32_t)ptr[2]) << 8) | ptr[3];
}

static inline void rtp_write_uint16(uint8_t* ptr, uint16_t val)
{
    ptr[0] = (uint8_t)(val >> 8);
    ptr[1] = (uint8_t)val;
}

static inline void rtp_write_uint32(uint8_t* ptr, uint32_t val)
{
    ptr[0] = (uint8_t)(val >> 24);
    ptr[1] = (uint8_t)(val >> 16);
    ptr[2] = (uint8_t)(val >> 8);
    ptr[3] = (uint8_t)val;
}

static inline void nbo_write_rtp_header(uint8_t *ptr, const rtp_header_t *header)
{
    ptr[0] = (uint8_t)((header->v << 6) | (header->p << 5) | (header->x << 4) | header->cc);
    ptr[1] = (uint8_t)((header->m << 7) | header->pt);
    ptr[2] = (uint8_t)(header->seq >> 8);
    ptr[3] = (uint8_t)(header->seq & 0xFF);

    nbo_w32(ptr+4, header->timestamp);
    nbo_w32(ptr+8, header->ssrc);
}

int rtp_packet_serialize_header(const struct rtp_packet *pkt, void* data, int bytes)
{
    int hdrlen;
    uint32_t i;
    uint8_t* ptr;

    if (RTP_VERSION != pkt->header.v || 0 != (pkt->extlen % 4)) {
        printf("header version wrong!\n");
        return -1;
    }

    hdrlen = RTP_FIXED_HEADER + pkt->header.cc * 4 + (pkt->header.x ? 4 : 0);
    if (bytes < hdrlen + pkt->extlen)
        return -1;

    ptr = (uint8_t *)data;
    nbo_write_rtp_header(ptr, &pkt->header);
    ptr += RTP_FIXED_HEADER;

    for (i = 0; i < pkt->header.cc; i++, ptr += 4) {
        nbo_w32(ptr, pkt->csrc[i]);
    }

    if (1 == pkt->header.x) {
        if (0 != (pkt->extlen % 4)) {
            return -1;
        }
        nbo_w16(ptr, pkt->reserved);
        nbo_w16(ptr + 2, pkt->extlen / 4);
        memcpy(ptr + 4, pkt->extension, pkt->extlen);
        ptr += pkt->extlen + 4;
    }

    return hdrlen + pkt->extlen;
}

int rtp_packet_serialize(const struct rtp_packet *pkt, void* data, int bytes)
{
    int hdrlen;

    hdrlen = rtp_packet_serialize_header(pkt, data, bytes);
    if (hdrlen < RTP_FIXED_HEADER || hdrlen + pkt->payloadlen > bytes)
        return -1;

    memcpy(((uint8_t*)data) + hdrlen, pkt->payload, pkt->payloadlen);
    return hdrlen + pkt->payloadlen;
}

int rtp_packet_deserialize(struct rtp_packet *pkt, const void* data, int bytes)
{
    uint32_t i, v;
    int hdrlen;
    const uint8_t *ptr;

    if (bytes < RTP_FIXED_HEADER)
        return -1;
    ptr = (const unsigned char *)data;
    memset(pkt, 0, sizeof(struct rtp_packet));

    v = nbo_r32(ptr);
    pkt->header.v = RTP_V(v);
    pkt->header.p = RTP_P(v);
    pkt->header.x = RTP_X(v);
    pkt->header.cc = RTP_CC(v);
    pkt->header.m = RTP_M(v);
    pkt->header.pt = RTP_PT(v);
    pkt->header.seq = RTP_SEQ(v);
    pkt->header.timestamp = nbo_r32(ptr + 4);
    pkt->header.ssrc = nbo_r32(ptr + 8);

    hdrlen = RTP_FIXED_HEADER + pkt->header.cc * 4;
    if (RTP_VERSION != pkt->header.v || bytes < hdrlen + (pkt->header.x ? 4 : 0) + (pkt->header.p ? 1 : 0))
        return -1;

    for (i = 0; i < pkt->header.cc; i++) {
        pkt->csrc[i] = nbo_r32(ptr + 12 + i * 4);
    }

    if (bytes < hdrlen) {
        return -1;
    }
    pkt->payload = (uint8_t*)ptr + hdrlen;
    pkt->payloadlen = bytes - hdrlen;

    if (1 == pkt->header.x) {
        const uint8_t *rtpext = ptr + hdrlen;
        if (pkt->payloadlen < 4) {
            return -1;
        }
        pkt->extension = rtpext + 4;
        pkt->reserved = nbo_r16(rtpext);
        pkt->extlen = nbo_r16(rtpext + 2) * 4;
        if (pkt->extlen + 4 > pkt->payloadlen) {
            return -1;
        } else {
            pkt->payload = rtpext + pkt->extlen + 4;
            pkt->payloadlen -= pkt->extlen + 4;
        }
    }

    if (1 == pkt->header.p) {
        uint8_t padding = ptr[bytes - 1];
        if (pkt->payloadlen < padding) {
            return -1;
        } else {
            pkt->payloadlen -= padding;
        }
    }

    return 0;
}

int rtp_ssrc(void)
{
    static unsigned int seed = 0;
    if (0 == seed) {
        seed = (unsigned int)time(NULL);
        srand(seed);
    }
    return rand();
}

struct rtp_socket *rtp_socket_create(enum rtp_mode mode, int tcp_fd, const char* src_ip)
{
    struct rtp_socket *s = CALLOC(1, struct rtp_socket);
    if (!s) {
        return NULL;
    }
    unsigned short i;
    switch (mode) {
    case RTP_TCP:
        s->tcp_fd = tcp_fd;
        break;
    case RTP_UDP:
        srand((unsigned int)time(NULL));
        do {
            i = rand() % 30000;
            i = i/2*2 + g_base_port;

            if (-1 == (s->rtp_fd = skt_udp_bind(src_ip, i))) {
                continue;
            }

            if (-1 == (s->rtcp_fd = skt_udp_bind(src_ip, i+1))) {
                skt_close(s->rtp_fd);
                continue;
            }
            s->rtp_port = i;
            s->rtcp_port = i+1;
            strcpy(s->ip, src_ip);
            printf("rtp_port = %d, rtcp_port = %d\n", s->rtp_port, s->rtcp_port);
            break;
        } while(1);
        break;
    case RAW_UDP:
    default:
        return NULL;
        break;
    }

    return s;
}

void rtp_socket_destroy(struct rtp_socket *s)
{
    if (s) {
        free(s);
    }
}

ssize_t rtp_sendto(struct rtp_socket *s, const char *ip, uint16_t port, const void *buf, size_t len)
{
    return skt_sendto(s->rtp_fd, ip, port, buf, len);
}

ssize_t rtp_recvfrom(struct rtp_socket *s, uint32_t *ip, uint16_t *port, void *buf, size_t len)
{
    return skt_recvfrom(s->rtp_fd, ip, port, buf, len);
}

ssize_t rtcp_sendto(struct rtp_socket *s, const char *ip, uint16_t port, const void *buf, size_t len)
{
    return skt_sendto(s->rtcp_fd, ip, port, buf, len);
}

ssize_t rtcp_recvfrom(struct rtp_socket *s, uint32_t *ip, uint16_t *port, void *buf, size_t len)
{
    return skt_recvfrom(s->rtp_fd, ip, port, buf, len);
}


#define RTCP_BANDWIDTH_FRACTION			0.05
#define RTCP_SENDER_BANDWIDTH_FRACTION	0.25

#define RTCP_REPORT_INTERVAL			5000 /* milliseconds RFC3550 p25 */
#define RTCP_REPORT_INTERVAL_MIN		2500 /* milliseconds RFC3550 p25 */

struct rtp_context * rtp_create(uint32_t ssrc, int frequence, int boundwidth)
{
    struct rtp_context *ctx	= CALLOC(1, struct rtp_context);
    if (!ctx) return NULL;

    ctx->rtcp_bw = (size_t)(boundwidth * RTCP_BANDWIDTH_FRACTION);
    ctx->avg_rtcp_size = 0;
    ctx->frequence = frequence;
    ctx->role = RTP_RECEIVER;
    ctx->init = 1;
    return ctx;
}

void rtp_destroy(struct rtp_context *rtp)
{
    free(rtp);
}

