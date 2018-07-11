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

#define RTSP_PARAM_STRING_MAX	(200)

typedef enum stream_mode {
    RTP_TCP,
    RTP_UDP,
    RAW_UDP,
} stream_mode_t;

typedef struct transport_header {
    int mode;
    char *mode_str;
    char *client_dst_addr;
    uint16_t client_rtp_port;
    uint16_t client_rtcp_port;
    uint8_t  rtp_channel_id;
    uint8_t  rtcp_channel_id;
    uint8_t  client_dst_ttl;
    uint8_t  reserved;
} rtsp_transport_header_t;

typedef struct rtsp_request {
    int fd;
    struct skt_addr client;
    struct iovec *iobuf;
    char cmd[RTSP_PARAM_STRING_MAX];
    char url_prefix[RTSP_PARAM_STRING_MAX];
    char url_suffix[RTSP_PARAM_STRING_MAX];
    char cseq[RTSP_PARAM_STRING_MAX];
    char session_id[RTSP_PARAM_STRING_MAX];
    struct transport_header hdr;
    struct rtsp_server_ctx *rtsp_server_ctx;
} rtsp_request_t;

int parse_rtsp_request(struct rtsp_request *req);
int parse_transport(struct transport_header *hdr, char *buf, int len);
const char* rtsp_reason_phrase(int code);

#ifdef __cplusplus
}
#endif
#endif
