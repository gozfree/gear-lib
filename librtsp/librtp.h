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
#ifndef LIBRTP_H
#define LIBRTP_H

#include <stdint.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

/* RTP Header
 * |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|X|   CC  |M|      PT     |         sequence number       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                           timestamp                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |            synchronization source (SSRC) identifier           |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 */
typedef struct rtp_header {
    uint8_t v:2;        /* protocol version */
    uint8_t p:1;        /* padding flag */
    uint8_t x:1;        /* header extension flag */
    uint8_t cc:4;       /* CSRC count */
    uint8_t m:1;        /* marker bit */
    uint8_t pt:7;       /* payload type */
    uint16_t seq:16;    /* sequence number */
    uint32_t timestamp; /* timestamp */
    uint32_t ssrc;      /* synchronization source */
} rtp_header_t;

#define RTP_VERSION 2 // RTP version field must equal 2 (p66)

#define RTP_V(v)    ((v >> 30) & 0x03)   /* protocol version */
#define RTP_P(v)    ((v >> 29) & 0x01)   /* padding flag */
#define RTP_X(v)    ((v >> 28) & 0x01)   /* header extension flag */
#define RTP_CC(v)   ((v >> 24) & 0x0F)   /* CSRC count */
#define RTP_M(v)    ((v >> 23) & 0x01)   /* marker bit */
#define RTP_PT(v)   ((v >> 16) & 0x7F)   /* payload type */
#define RTP_SEQ(v)  ((v >> 00) & 0xFFFF) /* sequence number */


/* RTCP Sender Report
 * |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|    RC   |   PT=SR=200   |             length            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                         SSRC of sender                        |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |              NTP timestamp, most significant word             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |             NTP timestamp, least significant word             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                         RTP timestamp                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                     sender's packet count                     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                      sender's octet count                     |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 */
typedef struct rtcp_header {
    uint8_t  v:2;       /* version */
    uint8_t  p:1;       /* padding */
    uint8_t  rc:5;      /* reception report count */
    uint8_t  pt:8;      /* packet type */
    uint16_t length:16; /* pkt len in words, w/o this word */
} rtcp_header_t;

#define RTCP_V(v)   ((v >> 30) & 0x03) // rtcp version
#define RTCP_P(v)   ((v >> 29) & 0x01) // rtcp padding
#define RTCP_RC(v)  ((v >> 24) & 0x1F) // rtcp reception report count
#define RTCP_PT(v)  ((v >> 16) & 0xFF) // rtcp packet type
#define RTCP_LEN(v) (v & 0xFFFF) // rtcp packet length

typedef struct rtcp_sr { //sender report
    uint32_t ssrc;
    uint32_t ntpmsw; // ntp timestamp MSW(in second)
    uint32_t ntplsw; // ntp timestamp LSW(in picosecond)
    uint32_t rtpts;  // rtp timestamp
    uint32_t spc;    // sender packet count
    uint32_t soc;    // sender octet count
} rtcp_sr_t;

typedef struct rtcp_rr { //receiver report
    uint32_t ssrc;
} rtcp_rr_t;

typedef struct rtcp_rb { // report block
    uint32_t ssrc;
    uint32_t fraction:8; // fraction lost
    uint32_t cumulative:24; // cumulative number of packets lost
    uint32_t exthsn; // extended highest sequence number received
    uint32_t jitter; // interarrival jitter
    uint32_t lsr; // last SR
    uint32_t dlsr; // delay since last SR
} rtcp_rb_t;

typedef struct rtcp_sdes_item { // source description RTCP packet
    uint8_t pt; // chunk type
    uint8_t len;
    uint8_t *data;
} rtcp_sdes_item_t;

enum {
    RTCP_SR   = 200,
    RTCP_RR   = 201,
    RTCP_SDES = 202,
    RTCP_BYE  = 203,
    RTCP_APP  = 204,
};

enum {
    RTCP_SDES_END     = 0,
    RTCP_SDES_CNAME   = 1,
    RTCP_SDES_NAME    = 2,
    RTCP_SDES_EMAIL   = 3,
    RTCP_SDES_PHONE   = 4,
    RTCP_SDES_LOC     = 5,
    RTCP_SDES_TOOL    = 6,
    RTCP_SDES_NOTE    = 7,
    RTCP_SDES_PRIVATE = 8,
};

struct rtp_packet
{
    struct rtp_header header;
    uint32_t csrc[16];
    const void* extension; // extension(valid only if rtp.x = 1)
    uint16_t extlen; // extension length in bytes
    uint16_t reserved; // extension reserved
    const void* payload; // payload
    int payloadlen; // payload length in bytes
};


enum rtp_mode {
    RTP_TCP,
    RTP_UDP,
    RAW_UDP,
};

struct rtp_socket {
    enum rtp_mode mode;
    uint16_t rtp_port;
    uint16_t rtcp_port;
    char ip[INET_ADDRSTRLEN];
    int rtp_fd;
    int rtcp_fd;
    int tcp_fd;
};

struct rtp_socket *rtp_socket_create(enum rtp_mode mode, int tcp_fd, const char* src_ip);
void rtp_socket_destroy(struct rtp_socket *s);

ssize_t rtp_sendto(struct rtp_socket *s, const char *ip, uint16_t port, const void *buf, size_t len);
ssize_t rtp_recvfrom(struct rtp_socket *s, uint32_t *ip, uint16_t *port, void *buf, size_t len);

ssize_t rtcp_sendto(struct rtp_socket *s, const char *ip, uint16_t port, const void *buf, size_t len);
ssize_t rtp_recvfrom(struct rtp_socket *s, uint32_t *ip, uint16_t *port, void *buf, size_t len);

#ifdef __cplusplus
}
#endif
#endif
