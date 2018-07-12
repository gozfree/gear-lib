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
#ifndef RTSP_PARSER_H
#define RTSP_PARSER_H

#include <libskt.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RTSP_PARAM_STRING_MAX   (200)

typedef enum stream_mode {
    RTP_TCP,
    RTP_UDP,
    RAW_UDP,
} stream_mode_t;

typedef struct transport_header {
    int mode;
    int multicast; // 0-unicast/1-multicast, default multicast
    char destination[65];
    char source[65];
	int layer; // rtsp setup response only
    int append; // use with RECORD mode only, rtsp setup response only
	unsigned short interleaved1, interleaved2; // rtsp setup response only
    union {
        struct multicast_t {
            int ttl; // multicast only
            unsigned short port1, port2; // multicast only
        } m;

        struct unicast_t {
            unsigned short client_port1, client_port2; // unicast RTP/RTCP port pair, RTP only
            unsigned short server_port1, server_port2; // unicast RTP/RTCP port pair, RTP only
            int ssrc; // RTP only(synchronization source (SSRC) identifier) 4-bytes
        } u;
    } rtp;
} transport_header_t;

struct session_header
{
	char id[RTSP_PARAM_STRING_MAX]; // session id
	int timeout;	   // millisecond
};

typedef struct rtsp_request {
    int fd;
    struct skt_addr client;
    struct iovec *raw;
    uint32_t content_len;
    char cmd[RTSP_PARAM_STRING_MAX];
    char url_origin[RTSP_PARAM_STRING_MAX];
    char url_prefix[RTSP_PARAM_STRING_MAX];
    char url_suffix[RTSP_PARAM_STRING_MAX];
    char cseq[RTSP_PARAM_STRING_MAX];
    struct transport_header transport;//maybe multi
    struct session_header session;
    struct rtsp_server_ctx *rtsp_server_ctx;
} rtsp_request_t;

int parse_rtsp_request(struct rtsp_request *req);
int parse_transport(struct transport_header *t, char *buf, int len);
int parse_session(struct session_header *s, char *buf, int len);
const char* rtsp_reason_phrase(int code);

#ifdef __cplusplus
}
#endif
#endif
