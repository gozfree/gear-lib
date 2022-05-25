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
#include "request_handle.h"
#include "media_source.h"
#include "transport_session.h"
#include "librtsp_server.h"
#include "rtp.h"
#include "sdp.h"
#include "uri_parse.h"
#include <liblog.h>
#include <libdict.h>
#include <libfile.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define RTSP_RESPONSE_LEN_MAX   (8192)

#define ALLOWED_COMMAND                         \
        "OPTIONS,"                              \
        "DESCRIBE,"                             \
        "SETUP,"                                \
        "TEARDOWN,"                             \
        "PLAY,"                                 \
        "GET_PARAMETER,"                        \
        "SET_PARAMETER\r\n\r\n"

#define RESP_DESCRIBE_FMT                       \
        "Content-Base: %s\r\n"                  \
        "Content-Type: application/sdp\r\n"     \
        "Content-Length: %u\r\n\r\n"            \
        "%s"

#define RESP_SETUP_FMT                          \
        "Transport: %s\r\n"                     \
        "Session: %08X\r\n\r\n"

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
    logi("rtsp response[%d]:\n==== C <<<< S ====\n%s\n==== C <<<< S ====\n",
          len, buf);
    return sock_send(req->fd, buf, len);
}

static int on_teardown(struct rtsp_request *req, char *url)
{
    struct rtsp_server *rc;
    struct transport_session *ts;
    //int len = sock_send(req->fd, resp, strlen(resp));
    if (-1 == parse_range(&req->range, (char *)req->raw->iov_base, req->raw->iov_len)) {
        loge("parse_range failed!\n");
        return -1;
    }
    rc = req->rtsp_server;
    ts = transport_session_lookup(rc->transport_session_pool, req->session.id);
    if (!ts) {
        loge("transport_session is NULL\n");
        return handle_rtsp_response(req, 454, NULL);
    }
    transport_session_stop(ts);
    return handle_rtsp_response(req, 200, NULL);
}

static int on_get_parameter(struct rtsp_request *req, char *url)
{
    //int len = sock_send(req->fd, resp, strlen(resp));
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

static int on_options(struct rtsp_request *req, char *url)
{
    return handle_rtsp_response(req, 200, ALLOWED_COMMAND);
}

static int on_describe(struct rtsp_request *req, char *url)
{
    char buf[RTSP_RESPONSE_LEN_MAX];
    struct media_source *ms = rtsp_media_source_lookup(url);
    if (!ms) {
        loge("media_source %s not found\n", url);
        return handle_rtsp_response(req, 404, NULL);
    }
    ms->sdp_generate(ms);
    snprintf(buf, sizeof(buf)+sizeof(ms->sdp), RESP_DESCRIBE_FMT,
                 req->url_origin,
                 (uint32_t)strlen(ms->sdp),
                 ms->sdp);//XXX: sdp line can't using "\r\n" as ending!!!
    return handle_rtsp_response(req, 200, buf);
}

static int on_setup(struct rtsp_request *req, char *url)
{
    char buf[RTSP_RESPONSE_LEN_MAX];
    char transport[128];
    struct transport_session *ts;
    struct rtsp_server *rc = req->rtsp_server;

    if (-1 == parse_transport(&req->transport, (char *)req->raw->iov_base, req->raw->iov_len)) {
        return handle_rtsp_response(req, 461, NULL);
    }
    if (-1 == parse_session(&req->session, (char *)req->raw->iov_base, req->raw->iov_len)) {
        loge("parse_session failed!\n");
    }
    if (0 == strlen(req->transport.source)) {
        //set local ipaddr to source
        struct sock_addr addr;
        sock_getaddr_by_fd(req->fd, &addr);
        strncpy(req->transport.source, addr.ip_str, sizeof(req->transport.source));
    }
    if (0 == strlen(req->transport.destination)) {
        sock_addr_ntop(req->transport.destination, req->client.ip);
    }

    ts = transport_session_lookup(rc->transport_session_pool, req->session.id);
    if (!ts) {
        ts = transport_session_create(rc->transport_session_pool, &req->transport);
        if (!ts) {
            loge("transport_session_create failed\n");
            return handle_rtsp_response(req, 500, NULL);
        }
    }

    switch (req->transport.mode) {
    case RTP_TCP:
        snprintf(transport, sizeof(transport), "RTP/AVP/TCP;unicast;interleaved=%d-%d", req->transport.interleaved1, req->transport.interleaved2);
        break;
    case RTP_UDP:
        if (req->transport.multicast) {
            return handle_rtsp_response(req, 461, NULL);
        } else {
            snprintf(transport, sizeof(transport), 
                     "RTP/AVP;unicast;client_port=%hu-%hu;server_port=%hu-%hu%s%s", 
                     req->transport.rtp.u.client_port1, req->transport.rtp.u.client_port2,
                     ts->rtp->sock->rtp_src_port, ts->rtp->sock->rtcp_src_port,
                     req->transport.destination[0] ? ";destination=" : "",
                     req->transport.destination[0] ? req->transport.destination : "");
        }
        break;
    case RAW_UDP:
        return handle_rtsp_response(req, 461, NULL);
        break;
    default:
        break;
    }
    snprintf(buf, sizeof(buf), RESP_SETUP_FMT, transport, ts->session_id);
    return handle_rtsp_response(req, 200, buf);
}

static int on_play(struct rtsp_request *req, char *url)
{
    int n = 0;
    char buf[RTSP_RESPONSE_LEN_MAX];
    struct rtsp_server *rc;
    struct transport_session *ts;
    struct media_source *ms;

    if (-1 == parse_range(&req->range, (char *)req->raw->iov_base, req->raw->iov_len)) {
        loge("parse_range failed!\n");
        return -1;
    }
    rc = req->rtsp_server;
    ts = transport_session_lookup(rc->transport_session_pool, req->session.id);
    if (!ts) {
        handle_rtsp_response(req, 454, NULL);
        return -1;
    }
    ms = rtsp_media_source_lookup(url);

    if (req->range.to > 0) {
        n += snprintf(buf+n, sizeof(buf)-n, "Range: npt=%.3f-%.3f\r\n", (float)(req->range.from / 1000.0f), (float)(req->range.to / 1000.0f));
    } else {
        n += snprintf(buf+n, sizeof(buf)-n, "Range: npt=%.3f-\r\n", (float)(req->range.from / 1000.0f));
    }

    n += snprintf(buf+n, sizeof(buf)-n, "Session: %s\r\n", req->session.id);
    n += snprintf(buf+n, sizeof(buf)-n, "RTP-Info: url=%s;seq=%s;rtptime=%u\r\n\r\n", req->url_origin, req->cseq, get_timestamp());//XXX

    handle_rtsp_response(req, 200, buf);
    transport_session_start(ts, ms);
    return 0;
}

static int on_pause(struct rtsp_request *req, char *url)
{
    return handle_rtsp_response(req, 461, NULL);
}

int handle_rtsp_request(struct rtsp_request *req)
{
    char url[2*RTSP_PARAM_STRING_MAX];
    strcat_url(url, req->url_prefix, req->url_suffix);

    logi("rtsp request[%d]:\n==== C >>>> S ====\n%s\n==== C >>>> S ====\n",
          req->raw->iov_len, req->raw->iov_base);
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
            return on_setup(req, url);//ok
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
            return on_play(req, url);//ok
        else if (strcasecmp(req->cmd, "PAUSE") == 0)
            return on_pause(req, url);
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
