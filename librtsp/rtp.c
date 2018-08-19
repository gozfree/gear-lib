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
#include "rtp.h"
#include <liblog.h>
#include <libskt.h>
#include <libmacro.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>

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

struct rtp_packet *rtp_packet_create(int size, uint8_t pt, uint16_t seq, uint32_t ssrc)
{
    struct rtp_packet *pkt = CALLOC(1, struct rtp_packet);
    if (!pkt) return NULL;

    pkt->header.v = RTP_VERSION;
    pkt->header.pt = pt;
    pkt->header.seq = seq;
    pkt->header.ssrc = ssrc;
    pkt->size = size;
    return pkt;
}

void rtp_packet_destroy(struct rtp_packet *pkt)
{
    if (pkt)
        free(pkt);
}

int rtp_packet_get_info(struct rtp_packet *pkt, uint16_t* seq, uint32_t* timestamp)
{
    if (!pkt) {
        return -1;
    }
    *seq = pkt->header.seq;
    *timestamp = pkt->header.timestamp;
    return 0;
}

int rtp_packet_pack(struct rtp_socket *sock, struct rtp_packet *pkt, const void* data, int bytes, uint32_t timestamp)
{
    int n;
    uint8_t *rtp;
    const uint8_t *ptr;
    if (!pkt || pkt->header.timestamp == timestamp || pkt->payload == NULL) {
        return -1;
    }
    pkt->header.timestamp = timestamp;
    pkt->header.m = 0;

    for (ptr = (const uint8_t *)data; bytes > 0; ++pkt->header.seq) {
        pkt->payload = ptr;
        pkt->payloadlen = (bytes + RTP_FIXED_HEADER) <= pkt->size ? bytes : (pkt->size - RTP_FIXED_HEADER);
        ptr += pkt->payloadlen;
        bytes -= pkt->payloadlen;

        n = RTP_FIXED_HEADER + pkt->payloadlen;
        rtp = (uint8_t*)calloc(1, n);
        if (!rtp) return ENOMEM;

        n = rtp_packet_serialize(pkt, rtp, n);
        if (n != RTP_FIXED_HEADER + pkt->payloadlen) {
            free(rtp);
            return -1;
        }

        loge("send len=%d\n", n);
        rtp_sendto(sock, NULL, 0, rtp, n);//XXX
        free(rtp);
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
            s->rtp_src_port = i;
            s->rtcp_src_port = i+1;
            if (src_ip) {
                strcpy(s->ip, src_ip);
            }
            logi("bind %s rtp port %d %d\n", src_ip, s->rtp_src_port, s->rtcp_src_port);
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
    return skt_sendto(s->rtp_fd, ip, s->rtp_dst_port, buf, len);
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

struct rtp_context *rtp_create(int frequence, int boundwidth)
{
    struct rtp_context *ctx	= CALLOC(1, struct rtp_context);
    if (!ctx) return NULL;

    ctx->ssrc = rtp_ssrc();
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


// Default max packet size (1500, minus allowance for IP, UDP, UMTP headers)
// (Also, make it a multiple of 4 bytes, just in case that matters.)
//static int s_max_packet_size = 1456; // from Live555 MultiFrameRTPSink.cpp RTP_PAYLOAD_MAX_SIZE
//static size_t s_max_packet_size = 576; // UNIX Network Programming by W. Richard Stevens
static int s_max_packet_size = 1434; // from VLC

void rtp_packet_setsize(int bytes)
{
	s_max_packet_size = bytes < 564 ? 564 : bytes;
}

int rtp_packet_getsize()
{
	return s_max_packet_size;
}

static const struct rtp_payload_type rtp_payload_types[] = {
    {0,   "PCMU",   MEDIA_TYPE_AUDIO,   8000,  1},
    {3,   "GSM",    MEDIA_TYPE_AUDIO,   8000,  1},
    {4,   "G723",   MEDIA_TYPE_AUDIO,   8000,  1},
    {5,   "DVI4",   MEDIA_TYPE_AUDIO,   8000,  1},
    {6,   "DVI4",   MEDIA_TYPE_AUDIO,   16000, 1},
    {7,   "LPC",    MEDIA_TYPE_AUDIO,   8000,  1},
    {8,   "PCMA",   MEDIA_TYPE_AUDIO,   8000,  1},
    {9,   "G722",   MEDIA_TYPE_AUDIO,   8000,  1},
    {10,  "L16",    MEDIA_TYPE_AUDIO,   44100, 2},
    {11,  "L16",    MEDIA_TYPE_AUDIO,   44100, 1},
    {12,  "QCELP",  MEDIA_TYPE_AUDIO,   8000,  1},
    {13,  "CN",     MEDIA_TYPE_AUDIO,   8000,  1},
    {14,  "MPA",    MEDIA_TYPE_AUDIO,   90000, 0},
    {15,  "G728",   MEDIA_TYPE_AUDIO,   8000,  1},
    {16,  "DVI4",   MEDIA_TYPE_AUDIO,   11025, 1},
    {17,  "DVI4",   MEDIA_TYPE_AUDIO,   22050, 1},
    {18,  "G729",   MEDIA_TYPE_AUDIO,   8000,  1},
    {25,  "CelB",   MEDIA_TYPE_VIDEO,   90000, 0},
    {26,  "JPEG",   MEDIA_TYPE_VIDEO,   90000, 0},
    {28,  "nv",     MEDIA_TYPE_VIDEO,   90000, 0},
    {31,  "H261",   MEDIA_TYPE_VIDEO,   90000, 0},
    {32,  "MPV",    MEDIA_TYPE_VIDEO,   90000, 0},
    {32,  "MPV",    MEDIA_TYPE_VIDEO,   90000, 0},
    {33,  "MP2T",   MEDIA_TYPE_DATA,    90000, 0},
    {34,  "H263",   MEDIA_TYPE_VIDEO,   90000, 0},
    {96,  "H264",   MEDIA_TYPE_VIDEO,   90000, 0},
    {97,  "H265",   MEDIA_TYPE_VIDEO,   90000, 0},
    {-1,  "",       MEDIA_TYPE_UNKNOWN, -1,    0}
};

const struct rtp_payload_type* rtp_payload_type_find(int payload)
{
    int i;
    for (i = 0; rtp_payload_types[i].pt >= 0; i++) {
        if (i == payload) {
            return &rtp_payload_types[i];
        }
    }
    return NULL;
}

void* rtp_payload_encode_create(int payload, const char* name, uint16_t seq, uint32_t ssrc, struct rtp_payload_t *handler, void* cbparam)
{
	int size;
	struct rtp_payload_delegate_t* ctx;

	ctx = calloc(1, sizeof(*ctx));
	if (ctx)
	{
		size = rtp_packet_getsize();
		if (rtp_payload_find(payload, name, ctx) < 0
			|| NULL == (ctx->packer = ctx->encoder->create(size, (uint8_t)payload, seq, ssrc, handler, cbparam)))
		{
			free(ctx);
			return NULL;
		}
	}
	return ctx;
}

void rtp_payload_encode_destroy(void* encoder)
{
	struct rtp_payload_delegate_t* ctx;
	ctx = (struct rtp_payload_delegate_t*)encoder;
	ctx->encoder->destroy(ctx->packer);
	free(ctx);
}

int rtp_payload_find(int payload, const char* encoding, struct rtp_payload_delegate_t* codec)
{
#if 0
	assert(payload >= 0 && payload <= 127);
	if (payload >= 96 && encoding)
	{
		if (0 == strcasecmp(encoding, "H264"))
		{
			// H.264 video (MPEG-4 Part 10) (RFC 6184)
			codec->encoder = rtp_h264_encode();
			codec->decoder = rtp_h264_decode();
		}
		else if (0 == strcasecmp(encoding, "H265") || 0 == strcasecmp(encoding, "HEVC"))
		{
			// H.265 video (HEVC) (RFC 7798)
			codec->encoder = rtp_h265_encode();
			codec->decoder = rtp_h265_decode();
		}
		else if (0 == strcasecmp(encoding, "MP4V-ES"))
		{
			// RFC6416 RTP Payload Format for MPEG-4 Audio/Visual Streams
			// 5. RTP Packetization of MPEG-4 Visual Bitstreams (p8)
			// 7.1 Media Type Registration for MPEG-4 Audio/Visual Streams (p17)
			codec->encoder = rtp_mp4v_es_encode();
			codec->decoder = rtp_mp4v_es_decode();
		}
		else if (0 == strcasecmp(encoding, "MP4A-LATM"))
		{
			// RFC6416 RTP Payload Format for MPEG-4 Audio/Visual Streams
			// 6. RTP Packetization of MPEG-4 Audio Bitstreams (p15)
			// 7.3 Media Type Registration for MPEG-4 Audio (p21)
			codec->encoder = rtp_mp4a_latm_encode();
			codec->decoder = rtp_mp4a_latm_decode();
		}
		else if (0 == strcasecmp(encoding, "mpeg4-generic"))
		{
			/// RFC3640 RTP Payload Format for Transport of MPEG-4 Elementary Streams
			/// 4.1. MIME Type Registration (p27)
			codec->encoder = rtp_mpeg4_generic_encode();
			codec->decoder = rtp_mpeg4_generic_decode();
		}
		else if (0 == strcasecmp(encoding, "VP8"))
		{
			/// RFC7741 RTP Payload Format for VP8 Video
			/// 6.1. Media Type Definition (p21)
			codec->encoder = rtp_vp8_encode();
			codec->decoder = rtp_vp8_decode();
		}
		else if (0 == strcasecmp(encoding, "VP9"))
		{
			/// RTP Payload Format for VP9 Video draft-ietf-payload-vp9-03
			/// 6.1. Media Type Definition (p15)
			codec->encoder = rtp_vp9_encode();
			codec->decoder = rtp_vp9_decode();
		}
		else if (0 == strcasecmp(encoding, "MP2P") // MPEG-2 Program Streams video (RFC 2250)
			|| 0 == strcasecmp(encoding, "MP1S"))  // MPEG-1 Systems Streams video (RFC 2250)
		{
			codec->encoder = rtp_ts_encode();
			codec->decoder = rtp_ts_decode();
		}
		else if (0 == strcasecmp(encoding, "opus")	// RFC7587 RTP Payload Format for the Opus Speech and Audio Codec
			|| 0 == strcasecmp(encoding, "G726-16") // ITU-T G.726 audio 16 kbit/s (RFC 3551)
			|| 0 == strcasecmp(encoding, "G726-24")	// ITU-T G.726 audio 24 kbit/s (RFC 3551)
			|| 0 == strcasecmp(encoding, "G726-32") // ITU-T G.726 audio 32 kbit/s (RFC 3551)
			|| 0 == strcasecmp(encoding, "G726-40") // ITU-T G.726 audio 40 kbit/s (RFC 3551)
			|| 0 == strcasecmp(encoding, "G7221"))  // RFC5577 RTP Payload Format for ITU-T Recommendation G.722.1
		{
			codec->encoder = rtp_common_encode();
			codec->decoder = rtp_common_decode();
		}
		else
		{
			return -1;
		}
	}
	else
	{
#if defined(_DEBUG) || defined(DEBUG)
		const struct rtp_profile_t* profile;
		profile = rtp_profile_find(payload);
		assert(!profile || !encoding || !*encoding || 0 == strcasecmp(profile->name, encoding));
#endif

		switch (payload)
		{
		case RTP_PAYLOAD_PCMU: // ITU-T G.711 PCM u-Law audio 64 kbit/s (RFC 3551)
		case RTP_PAYLOAD_PCMA: // ITU-T G.711 PCM A-Law audio 64 kbit/s (RFC 3551)
		case RTP_PAYLOAD_G722: // ITU-T G.722 audio 64 kbit/s (RFC 3551)
		case RTP_PAYLOAD_G729: // ITU-T G.729 and G.729a audio 8 kbit/s (RFC 3551)
			codec->encoder = rtp_common_encode();
			codec->decoder = rtp_common_decode();
			break;

		case RTP_PAYLOAD_MPA: // MPEG-1 or MPEG-2 audio only (RFC 3551, RFC 2250)
		case RTP_PAYLOAD_MPV: // MPEG-1 and MPEG-2 video (RFC 2250)
			codec->encoder = rtp_mpeg1or2es_encode();
			codec->decoder = rtp_mpeg1or2es_decode();
			break;

		case RTP_PAYLOAD_MP2T: // MPEG-2 transport stream (RFC 2250)
			codec->encoder = rtp_ts_encode();
			codec->decoder = rtp_ts_decode();
			break;

		case RTP_PAYLOAD_JPEG:
		case RTP_PAYLOAD_H263:
			return -1; // TODO

		default:
			return -1; // not support
		}
	}
#endif

	return 0;
}

void rtcp_rr_unpack(rtcp_header_t *header, const uint8_t* ptr)
{
    uint32_t ssrc, i;
    rtcp_rb_t rb;

    if (header->length * 4 < sizeof(rtcp_rr_t) + header->rc * sizeof(rtcp_rb_t)) {
        loge("error occur\n");
        return;
    }
    ssrc = nbo_r32(ptr);
    logi("Received RTCP packet from %08X\n", ssrc);

    ptr += sizeof(rtcp_rr_t);
    for (i = 0; i < header->rc; i++, ptr += sizeof(rtcp_rb_t)) {
        rb.ssrc = nbo_r32(ptr);
        rb.fraction = ptr[4];
        rb.cumulative = (((uint32_t)ptr[5])<<16) | (((uint32_t)ptr[6])<<8)| ptr[7];
        rb.exthsn = nbo_r32(ptr+8);
        rb.jitter = nbo_r32(ptr+12);
        rb.lsr = nbo_r32(ptr+16);
        rb.dlsr = nbo_r32(ptr+20);
        logi("\n      Received RR of source: %08X"
             "\n              Fraction Lost: %hhu"
             "\n         Total Packets Lost: %d"
             "\nReceived Packets Seq Number: %u"
             "\n        Interarrival Jitter: %u"
             "\n                    Last SR: %u"
             "\n        Delay Since Last SR: %u\n",
             rb.ssrc, rb.fraction, rb.cumulative,
             rb.exthsn, rb.jitter, rb.lsr, rb.dlsr);
    }
}

void rtcp_sr_unpack(rtcp_header_t *header, const uint8_t* ptr)
{
    uint32_t i;
    rtcp_sr_t sr;
    rtcp_rb_t rb;

    if (header->length * 4 < sizeof(rtcp_sr_t) + header->rc * sizeof(rtcp_rb_t)) {
        loge("error occur\n");
        return;
    }
    sr.ssrc = nbo_r32(ptr);
    sr.ntpmsw = nbo_r32(ptr + 4);
    sr.ntplsw = nbo_r32(ptr + 8);
    sr.rtpts = nbo_r32(ptr + 12);
    sr.spc = nbo_r32(ptr + 16);
    sr.soc = nbo_r32(ptr + 20);

    logi("\nReceived SR sender info of SSRC %08X:"
         "\n           NTP  Timestamp MSW: %u"
         "\n           NTP  Timestamp LSW: %u"
         "\n                RTP Timestamp: %u"
         "\n        Sender's packet count: %u"
         "\n         Sender's octet count: %u",
         sr.ssrc, sr.ntpmsw, sr.ntplsw, sr.rtpts, sr.spc, sr.soc);
    ptr += sizeof(rtcp_sr_t);
    for (i = 0; i < header->rc; i++, ptr+=sizeof(rtcp_rb_t)) {
        rb.ssrc = nbo_r32(ptr);
        rb.fraction = ptr[4];
        rb.cumulative = (((uint32_t)ptr[5])<<16) | (((uint32_t)ptr[6])<<8)| ptr[7];
        rb.exthsn = nbo_r32(ptr+8);
        rb.jitter = nbo_r32(ptr+12);
        rb.lsr = nbo_r32(ptr+16);
        rb.dlsr = nbo_r32(ptr+20);
        logi("\n      Received SR of source: %08X"
             "\n              Fraction Lost: %hhu"
             "\n         Total Packets Lost: %d"
             "\nReceived Packets Seq Number: %u"
             "\n        Interarrival Jitter: %u"
             "\n                    Last SR: %u"
             "\n        Delay Since Last SR: %u\n",
             rb.ssrc, rb.fraction, rb.cumulative,
             rb.exthsn, rb.jitter, rb.lsr, rb.dlsr);
    }
}
int rtcp_parse(char* data, size_t bytes)
{
    rtcp_header_t header;
    uint32_t rtcphd = nbo_r32((uint8_t *)data);
    header.v = RTCP_V(rtcphd);
    header.p = RTCP_P(rtcphd);
    header.rc = RTCP_RC(rtcphd);
    header.pt = RTCP_PT(rtcphd);
    header.length = RTCP_LEN(rtcphd);

    if (header.length * 4 + 4 > bytes) {
        loge("xxx\n");
        return -1;
    }

    if (1 == header.p) {
        header.length -= *(data + header.length - 1) * 4;
        loge("xxx\n");
    }

    switch (header.pt) {
    case RTCP_SR:
        rtcp_sr_unpack(&header, (uint8_t *)data+4);
        break;
    case RTCP_RR:
        rtcp_rr_unpack(&header, (uint8_t *)data+4);
        break;
    case RTCP_SDES:
        loge("RTCP_SDES\n");
        //rtcp_sdes_unpack(ctx, &header, data+4);
        break;
    case RTCP_BYE:
        loge("RTCP_BYE\n");
        //rtcp_bye_unpack(ctx, &header, data+4);
        break;
    case RTCP_APP:
        loge("RTCP_APP\n");
        //rtcp_app_unpack(ctx, &header, data+4);
        break;
    default:
        loge("xxxx\n");
        break;
    }

    return 0;
}
