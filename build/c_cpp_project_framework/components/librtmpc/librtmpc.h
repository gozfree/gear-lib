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
#ifndef LIBRTMPC_H
#define LIBRTMPC_H

#include <libposix.h>
#include <libqueue.h>
#include <libthread.h>
#include <libmedia-io.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "flv_mux.h"

#define LIBRTMPC_VERSION "0.1.0"

#ifdef __cplusplus
extern "C" {
#endif

struct rtmp_url {
    char *addr;
    char *key;
    char *username;
    char *password;

};

struct rtmpc {
    void *base;
    struct flv_muxer *flv;
    struct queue *q;
    struct thread *thread;
    bool is_run;
    bool is_start;
    bool sent_headers;
};

GEAR_API struct rtmpc *rtmpc_create(const char *push_url);
GEAR_API struct rtmpc *rtmpc_create2(struct rtmp_url *url);
GEAR_API int rtmpc_stream_add(struct rtmpc *rtmpc, struct media_packet *pkt);
GEAR_API int rtmpc_stream_start(struct rtmpc *rtmpc);
GEAR_API void rtmpc_stream_stop(struct rtmpc *rtmpc);
GEAR_API int rtmpc_send_packet(struct rtmpc *rtmpc, struct media_packet *pkt);
GEAR_API void rtmpc_destroy(struct rtmpc *rtmpc);


#ifdef __cplusplus
}
#endif
#endif
