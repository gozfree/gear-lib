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
#include "media_session.h"
#include "librtsp_server.h"
#include "sdp.h"
#include <liblog.h>
#include <libdict.h>
#include <libmacro.h>
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
                    "%s"
                    "\r\n",
                    code, rtsp_reason_phrase(code), req->cseq, get_date_string(), msg?msg:"");
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
static void client_session_destroy(struct client_session *cs)
{

}

static int on_teardown(struct rtsp_request *req, char *url)
{
    client_session_destroy(NULL);
    //int len = skt_send(req->fd, resp, strlen(resp));
    return 0;
}
static int on_get_parameter(struct rtsp_request *req, char *url)
{
    //int len = skt_send(req->fd, resp, strlen(resp));
    return 0;
}
static char *get_rtsp_url(struct rtsp_request *req, struct media_session *ms, char *buf, size_t size)
{
    const char *host = RTSP_SERVER_IP;
    int port = req->rtsp_server_ctx->host.port;
    char url_total[2*RTSP_PARAM_STRING_MAX];
    memset(url_total, 0, sizeof(url_total));
    if (port == 554) {
        snprintf(buf, size, "rtsp://%s/%s", host, strcat_url(url_total, req->url_prefix, ms->name));
    } else {
        snprintf(buf, size, "rtsp://%s:%hu/%s", host, port, strcat_url(url_total, req->url_prefix, ms->name));
    }
    logd("RTSP URL: %s\n", buf);
    return buf;
}

static int on_describe(struct rtsp_request *req, char *url)
{
    char sdp[RTSP_RESPONSE_LEN_MAX];
    char buf[RTSP_RESPONSE_LEN_MAX];
    struct rtsp_server_ctx *rc = req->rtsp_server_ctx;
    char rtspurl[1024]   = {0};
    struct media_session *ms = media_session_lookup(rc->media_session_pool, url);
    if (!ms) {
        logd("media_session %s not found\n", url);
        return handle_rtsp_response(req, 404, NULL);
    }
    if (-1 == get_sdp(ms, sdp, sizeof(sdp))) {
        loge("get_sdp failed!\n");
        return handle_rtsp_response(req, 404, NULL);
    }
    //XXX: sdp line can't using "\r\n" as ending!!!
    int len = snprintf(buf, sizeof(buf), RESP_DESCRIBE_FMT,
                 req->cseq,
                 get_date_string(),
                 get_rtsp_url(req, ms, rtspurl, sizeof(rtspurl)),
                 (uint32_t)strlen(sdp),
                 sdp);
    //int ret = handle_rtsp_response(req, 200, sdp);
    loge("buf = %s\n", buf);
    int ret = skt_send(req->fd, buf, len);
    return ret;
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
    char rtspurl[1024] = {0};
    char const* rtp_info_fmt =
    "RTP-Info: " // "RTP-Info:", plus any preceding rtpInfo items
    "url=%s/%s"
    ";seq=%d"
    ";rtptime=%u";
    struct rtsp_server_ctx *rc = req->rtsp_server_ctx;
    char url_total[2*RTSP_PARAM_STRING_MAX];
    // enough space for url_prefix/url_suffix'\0'
    strcat_url(url_total, req->url_prefix, req->url_suffix);
    struct media_session *ms = media_session_lookup(rc->media_session_pool, url_total);
    if (!ms) {
        loge("media_session_lookup find nothting\n");
        return NULL;
    }
    snprintf(buf, size, rtp_info_fmt,
             get_rtsp_url(req, ms, rtspurl, sizeof(rtspurl)),
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
               (uint32_t)strtol(req->session_id, NULL, 16),
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

uint32_t get_random_number()
{
  struct timeval now = {0};
  gettimeofday(&now, NULL);
  srand(now.tv_usec);
  return (rand() % ((uint32_t)-1));
}

static struct client_session *client_session_create(struct rtsp_request *req)
{
    char session_id[9];
    struct rtsp_server_ctx *rc = req->rtsp_server_ctx;
    struct client_session *cs = CALLOC(1, struct client_session);
    struct client_session *tmp;
    do {
        cs->session_id = get_random_number();
        snprintf(session_id, sizeof(session_id), "%08x", cs->session_id);
        tmp = (struct client_session *)dict_get(rc->client_session, session_id, NULL);
    } while (tmp != NULL);
    dict_add(rc->client_session, session_id, (char *)cs);

    return cs;
}
static struct client_session *client_session_lookup(struct rtsp_server_ctx *rc, struct rtsp_request *req)
{
    struct client_session *cs = (struct client_session *)dict_get(rc->client_session, req->session_id, NULL);
    return cs;
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

    parse_transport(&req->hdr, (char *)req->iobuf->iov_base, req->iobuf->iov_len);
    struct client_session *cs = client_session_lookup(rc, req);
    if (!cs) {
        loge("client_session_lookup find nothting\n");
        cs = client_session_create(req);
        if (!cs) {
            loge("client_session_create failed\n");
            handle_session_not_found(req, buf, sizeof(buf));
            return -1;
        }
    }
    struct media_session *ms = media_session_lookup(rc->media_session_pool, url);
    if (!ms) {
        loge("media_session_lookup find nothting\n");
        handle_not_found(req, buf, sizeof(buf));
        return -1;
    }
    //TODO: handle subsession
    if (req->hdr.client_dst_addr != NULL) {
        dst_ip = req->hdr.client_dst_addr;
    }
    switch (req->hdr.mode) {
    case RTP_TCP:
        mode_string = "RTP/AVP/TCP";
        break;
    case RTP_UDP:
        mode_string = "RTP/AVP";
        break;
    case RAW_UDP:
        mode_string = req->hdr.mode_str;
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
                 req->hdr.client_rtp_port,
                 req->hdr.client_rtcp_port,
                 port_rtp, port_rtcp,
                 cs->session_id);
    logd("rtsp response len = %d buf:\n%s\n", len, buf);
    return skt_send(req->fd, buf, len);
}

static int on_options(struct rtsp_request *req, char *url)
{
    return handle_rtsp_response(req, 200, ALLOWED_COMMAND);
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
            return on_describe(req, url);
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
