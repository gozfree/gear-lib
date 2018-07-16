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

#include "rtsp_cmd.h"
#include "request_handle.h"
#include "media_source.h"
#include "librtsp_server.h"
#include "librtp.h"
#include "sdp.h"
#include "uri_parse.h"
#include <liblog.h>
#include <libdict.h>
#include <libmacro.h>
#include <libfile.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define RTSP_RESPONSE_LEN_MAX	(8192)

#define RTSP_SERVER_IP      ((const char *)"127.0.0.1")

static char *strcat_url(char *dest, char *prefix, char *suffix)
{
    dest[0] = '\0';
    if (prefix[0] != '\0') {
        strcat(dest, prefix);
        strcat(dest, "/");
    }
    return strcat(dest, suffix);
}

static char const* get_date_string()
{
    static char buf[128];
    time_t cur_time = time(NULL);
    strftime(buf, sizeof(buf), "%a, %b %d %Y %H:%M:%S GMT", gmtime(&cur_time));
    return buf;
}

int handle_rtsp_response(struct rtsp_request *req, int code, const char *msg)
{
    char buf[RTSP_RESPONSE_LEN_MAX];
    int len = snprintf(buf, sizeof(buf),
                    "RTSP/1.0 %d %s\r\n"
                    "CSeq: %s\r\n"
                    "Date: %s\r\n"
                    "%s",
                    code, rtsp_reason_phrase(code),
                    req->cseq,
                    get_date_string(),
                    msg?msg:"\r\n");
    logd("rtsp response len = %d buf:\n%s\n", len, buf);
    return skt_send(req->fd, buf, len);
}


static int handle_not_found(struct rtsp_request *req, char *buf, size_t size)
{
    snprintf(buf, size, RESP_NOT_FOUND_FMT,
             req->cseq, get_date_string());
    return 0;
}

static int handle_session_not_found(struct rtsp_request *req, char *buf, size_t size)
{
    snprintf(buf, size, RESP_SESSION_NOT_FOUND_FMT,
             req->cseq, get_date_string());
    return 0;
}

static int on_teardown(struct rtsp_request *req, char *url)
{
    //transport_session_pool_destroy(NULL);
    //int len = skt_send(req->fd, resp, strlen(resp));
    return 0;
}
static int on_get_parameter(struct rtsp_request *req, char *url)
{
    //int len = skt_send(req->fd, resp, strlen(resp));
    return 0;
}



static uint32_t conv_to_rtp_timestamp(struct timeval tv)
{
    uint32_t ts_freq = 1;//audio sample rate
    uint32_t ts_inc = (ts_freq*tv.tv_sec);
    ts_inc += (uint32_t)(ts_freq*(tv.tv_usec/1000000.0) + 0.5);
    return ts_inc;
}



static uint32_t get_timestamp()
{
    struct timeval now;
    gettimeofday(&now, NULL);

    return conv_to_rtp_timestamp(now);
}


static char *get_rtp_info(struct rtsp_request *req, char *buf, size_t size)
{
    char const* rtp_info_fmt =
    "RTP-Info: " // "RTP-Info:", plus any preceding rtpInfo items
    "url=%s/%s"
    ";seq=%d"
    ";rtptime=%u";
    struct rtsp_server_ctx *rc = req->rtsp_server_ctx;
    char url_total[2*RTSP_PARAM_STRING_MAX];
    // enough space for url_prefix/url_suffix'\0'
    strcat_url(url_total, req->url_prefix, req->url_suffix);
    struct media_source *ms = media_source_lookup(rc->media_source_pool, url_total);
    if (!ms) {
        loge("media_source_lookup find nothting\n");
        return NULL;
    }
    snprintf(buf, size, rtp_info_fmt,
             req->url_origin,
             "track1",
             req->cseq,
             get_timestamp());
    return buf;
}

static int on_play(struct rtsp_request *req, char *url)
{
    char buf[RTSP_RESPONSE_LEN_MAX];
    char rtp_info[1024] = {0};

    int len = snprintf(buf, sizeof(buf), RESP_PLAY_FMT,
               req->cseq,
               get_date_string(),
               (uint32_t)strtol(req->session.id, NULL, 16),
               get_rtp_info(req, rtp_info, sizeof(rtp_info)));
    char rtp_url[128] = {0};
    memset(rtp_url, 0, sizeof(rtp_url));

#if 0
    struct rtsp_server_ctx *rc = req->rtsp_server_ctx;
    snprintf(rtp_url, sizeof(rtp_url), "rtp://127.0.0.1:%d", rc->server_rtp_port);
    struct protocol_ctx *pc = protocol_new(rtp_url);
    logi("protocol_new success!\n");
    if (-1 == protocol_open(pc)) {
        logi("protocol_open rtp failed!\n");
        return -1;
    }
    logi("protocol_open rtp success!\n");
    req->rtsp_server_ctx->rtp_ctx = pc;
    logi("rtp_ctx = %p\n", req->rtsp_server_ctx->rtp_ctx);
#endif

    logd("rtsp response len = %d buf:\n%s\n", len, buf);
    return skt_send(req->fd, buf, len);
}

static int on_options(struct rtsp_request *req, char *url)
{
    return handle_rtsp_response(req, 200, ALLOWED_COMMAND);
}

static int on_describe(struct rtsp_request *req, char *url)
{
    //char sdp[RTSP_RESPONSE_LEN_MAX];
    char buf[RTSP_RESPONSE_LEN_MAX];
    struct rtsp_server_ctx *rc = req->rtsp_server_ctx;
    struct media_source *ms = media_source_lookup(rc->media_source_pool, url);
    if (!ms) {
        loge("media_source %s not found\n", url);

        char path1[256];
        struct uri_t r;
        if (0 == uri_parse(&r, req->url_origin, strlen(req->url_origin))) {
            url_decode(r.path, strlen(r.path), path1, sizeof(path1));
        } else {
            loge("xxxx\n");
        }
        loge("path = %s\n", path1);
        if (file_exist(url)) {
            ms = media_source_new(rc->media_source_pool, url, strlen(url));
            if (!ms) {
                return handle_rtsp_response(req, 404, NULL);
            }
        } else {
            return handle_rtsp_response(req, 404, NULL);
        }
    }
#if 0
    if (-1 == get_sdp(ms, sdp, sizeof(sdp))) {
        loge("get_sdp failed!\n");
        return handle_rtsp_response(req, 404, NULL);
    }
#endif
    snprintf(buf, sizeof(buf), RESP_DESCRIBE_FMT,
                 req->url_origin,
                 (uint32_t)strlen(ms->sdp),
                 ms->sdp);//XXX: sdp line can't using "\r\n" as ending!!!
    return handle_rtsp_response(req, 200, buf);
}

static int on_setup(struct rtsp_request *req, char *url)
{
    char buf[RTSP_RESPONSE_LEN_MAX];
    const char *mode_string = NULL;
    const char *dst_ip = RTSP_SERVER_IP;
    const char *client_ip = RTSP_SERVER_IP;
    struct rtsp_server_ctx *rc = req->rtsp_server_ctx;
    rc->server_rtp_port = 6970;
    rc->server_rtcp_port = rc->server_rtp_port + 1;
    int port_rtp = rc->server_rtp_port;
    int port_rtcp =  rc->server_rtcp_port;

    if (-1 == parse_transport(&req->transport, (char *)req->raw->iov_base, req->raw->iov_len)) {
        return handle_rtsp_response(req, 461, NULL);
    }
    if (-1 == parse_session(&req->session, (char *)req->raw->iov_base, req->raw->iov_len)) {
        loge("parse_session failed!\n");
    }
    loge("session.id = %s\n", req->session.id);

    struct transport_session *cs = transport_session_lookup(rc->transport_session_pool, req->session.id);
    if (!cs) {
        cs = transport_session_new(rc->transport_session_pool);
        if (!cs) {
            loge("transport_session_new failed\n");
            handle_session_not_found(req, buf, sizeof(buf));
            return -1;
        }
    }
    struct media_source *ms = media_source_lookup(rc->media_source_pool, url);
    if (!ms) {
        loge("media_source_lookup find nothting\n");
        handle_not_found(req, buf, sizeof(buf));
        return -1;
    }
    //TODO: handle subsession
    if (req->transport.destination != NULL) {
        dst_ip = req->transport.destination;
    }
    switch (req->transport.mode) {
    case RTP_TCP:
        mode_string = "RTP/AVP/TCP";
        break;
    case RTP_UDP:
        mode_string = "RTP/AVP";
        break;
    case RAW_UDP:
        mode_string = "RTP/AVP/UDP";
        break;
    default:
        break;
    }
    skt_udp_bind(NULL, port_rtp);
    skt_udp_bind(NULL, port_rtcp);

    int len = snprintf(buf, sizeof(buf), RESP_SETUP_FMT,
                 req->cseq,
                 get_date_string(),
                 mode_string,
                 dst_ip,
                 client_ip,
                 req->transport.rtp.u.client_port1,
                 req->transport.rtp.u.client_port2,
                 port_rtp, port_rtcp,
                 cs->session_id);
    logd("rtsp response len = %d buf:\n%s\n", len, buf);
    return skt_send(req->fd, buf, len);
}
int handle_rtsp_request(struct rtsp_request *req)
{
    char url[2*RTSP_PARAM_STRING_MAX];
    strcat_url(url, req->url_prefix, req->url_suffix);

    switch (req->cmd[0]) {
    case 'o':
    case 'O':
        if (strcasecmp(req->cmd, "OPTIONS") == 0)
            return on_options(req, url);//ok
        break;
    case 'd':
    case 'D':
        if (strcasecmp(req->cmd, "DESCRIBE") == 0)
            return on_describe(req, url);//ok
        break;
    case 's':
    case 'S':
        if (strcasecmp(req->cmd, "SETUP") == 0)
            return on_setup(req, url);
        else if (strcasecmp(req->cmd, "SET_PARAMETER") == 0)
            loge("SET_PARAMETER not support!\n");
        break;
    case 't':
    case 'T':
        if (strcasecmp(req->cmd, "TEARDOWN") == 0)
            return on_teardown(req, url);
        break;
    case 'p':
    case 'P':
        if (strcasecmp(req->cmd, "PLAY") == 0)
            return on_play(req, url);
        else if (strcasecmp(req->cmd, "PAUSE") == 0)
            loge("PAUSE not support!\n");
        break;
    case 'g':
    case 'G':
        if (strcasecmp(req->cmd, "GET_PARAMETER") == 0)
            return on_get_parameter(req, url);
        break;
    case 'r':
    case 'R':
        if (strcasecmp(req->cmd, "RECORD") == 0)
            loge("RECORD not support!\n");
        else if (strcasecmp(req->cmd, "REGISTER") == 0)
            loge("REGISTER not support!\n");
        break;
    default:
        loge("%s not support!\n", req->cmd);
        break;
    }
    return 0;
}
