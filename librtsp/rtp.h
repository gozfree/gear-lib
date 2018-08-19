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
 * |               contributing source (CSRC) identifiers          |
 * |                               ....                            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
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

#define RTP_VERSION      2
#define RTP_FIXED_HEADER 12
struct rtp_packet
{
    struct rtp_header header;
    uint32_t csrc[16];
    const void* extension; // extension(valid only if rtp.x = 1)
    uint16_t extlen; // extension length in bytes
    uint16_t reserved; // extension reserved
    const void* payload; // payload
    int payloadlen; // payload length in bytes
    int size; //XXX
};

enum rtp_mode {
    RTP_TCP,
    RTP_UDP,
    RAW_UDP,
};

struct rtp_socket {
    enum rtp_mode mode;
    uint16_t rtp_src_port;
    uint16_t rtcp_src_port;
    uint16_t rtp_dst_port;
    uint16_t rtcp_dst_port;
    char ip[INET_ADDRSTRLEN];
    int rtp_fd;
    int rtcp_fd;
    int tcp_fd;
};

int rtp_ssrc(void);

struct rtp_packet *rtp_packet_create(int size, uint8_t pt, uint16_t seq, uint32_t ssrc);
void rtp_packet_destroy(struct rtp_packet *pkt);
int rtp_packet_serialize_header(const struct rtp_packet *pkt, void* data, int bytes);
int rtp_packet_serialize(const struct rtp_packet *pkt, void* data, int bytes);
int rtp_packet_deserialize(struct rtp_packet *pkt, const void* data, int bytes);
int rtp_packet_pack(struct rtp_socket *sock, struct rtp_packet *pkt, const void* data, int bytes, uint32_t timestamp);
int rtp_packet_get_info(struct rtp_packet *pkt, uint16_t* seq, uint32_t* timestamp);
void rtp_packet_setsize(int bytes);
int rtp_packet_getsize();

struct rtp_socket *rtp_socket_create(enum rtp_mode mode, int tcp_fd, const char* src_ip);
void rtp_socket_destroy(struct rtp_socket *s);

ssize_t rtp_sendto(struct rtp_socket *s, const char *ip, uint16_t port, const void *buf, size_t len);
ssize_t rtp_recvfrom(struct rtp_socket *s, uint32_t *ip, uint16_t *port, void *buf, size_t len);

ssize_t rtcp_sendto(struct rtp_socket *s, const char *ip, uint16_t port, const void *buf, size_t len);
ssize_t rtp_recvfrom(struct rtp_socket *s, uint32_t *ip, uint16_t *port, void *buf, size_t len);


enum rtp_role { 
    RTP_SENDER   = 1, /// send RTP packet
    RTP_RECEIVER = 2, /// receive RTP packet
};

enum media_type {
    MEDIA_TYPE_UNKNOWN = -1,
    MEDIA_TYPE_VIDEO,
    MEDIA_TYPE_AUDIO,
    MEDIA_TYPE_DATA,
    MEDIA_TYPE_SUBTITLE,
    MEDIA_TYPE_ATTACHMENT,
    MEDIA_TYPE_NB
};

struct rtp_payload_type {
    int pt;
    const char name[32];
    enum media_type media_type;
    int clock_rate;
    int audio_channels;
};

struct rtp_payload_t
{
    struct rtp_payload_type payload_type;
    void* (*alloc)(void* param, int bytes);
    void (*free)(void* param, void *packet);
    void (*packet)(void* param, const void *packet, int bytes, uint32_t timestamp, int flags);
};

struct rtp_payload_encode_t
{
    /// create RTP packer
    /// @param[in] size maximum RTP packet payload size(don't include RTP header)
    /// @param[in] payload RTP header PT filed (see more about rtp-profile.h)
    /// @param[in] seq RTP header sequence number filed
    /// @param[in] ssrc RTP header SSRC filed
    /// @param[in] handler user-defined callback
    /// @param[in] cbparam user-defined parameter
    /// @return RTP packer
    void* (*create)(int size, uint8_t payload, uint16_t seq, uint32_t ssrc, struct rtp_payload_t *handler, void* cbparam);
    /// destroy RTP Packer
    void(*destroy)(void* packer);

    void(*get_info)(void* packer, uint16_t* seq, uint32_t* timestamp);

    /// PS/H.264 Elementary Stream to RTP Packet
    /// @param[in] packer
    /// @param[in] data stream data
    /// @param[in] bytes stream length in bytes
    /// @param[in] time stream UTC time
    /// @return 0-ok, ENOMEM-alloc failed, <0-failed
    int(*input)(void* packer, const void* data, int bytes, uint32_t time);
};

struct rtp_payload_decode_t
{
    void* (*create)(struct rtp_payload_t *handler, void* param);
    void (*destroy)(void* packer);

    /// RTP packet to PS/H.264 Elementary Stream
    /// @param[in] decoder RTP packet unpackers
    /// @param[in] packet RTP packet
    /// @param[in] bytes RTP packet length in bytes
    /// @param[in] time stream UTC time
    /// @return 1-packet handled, 0-packet discard, <0-failed
    int (*input)(void* decoder, const void* packet, int bytes);
};


struct rtp_payload_delegate_t
{
    struct rtp_payload_encode_t* encoder;
    struct rtp_payload_decode_t* decoder;
    void* packer;
};

int rtp_payload_find(int payload, const char* encoding, struct rtp_payload_delegate_t* codec);

struct rtp_context
{
    uint32_t ssrc;
    // RTP/RTCP
    int avg_rtcp_size;
    int rtcp_bw;
    int rtcp_cycle; // for RTCP SDES
    int frequence;
    int init;
    int role;
    struct rtp_socket *sock;
    struct rtp_payload_t *payload;
};
struct rtp_context *rtp_create(int frequence, int boundwidth);

int rtp_payload_h264_encode(struct rtp_socket *sock, struct rtp_packet *pkt, const void* h264, int bytes, uint32_t timestamp);

int rtcp_parse(/*struct rtp_context *ctx, */char* data, size_t bytes);
#ifdef __cplusplus
}
#endif
#endif
