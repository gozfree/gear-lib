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
#include "libgevent.h"
#include "libposix4win.h"


struct iocp_ctx {
    HANDLE port;
    ULONG_PTR key;
    int thread_nums;
    pthread_t *tids;
    bool is_run;

    long ms;
};



#define NOTIFICATION_KEY ((ULONG_PTR)-1)

static void iocp_cb(OVERLAPPED *o, ULONG_PTR completion_key, DWORD nBytes, int ok)
{
#if 0
    struct event_overlapped *eo =
	    EVUTIL_UPCAST(o, struct event_overlapped, overlapped);
	eo->cb(eo, completion_key, nBytes, ok);
#endif
}

static void *iocp_thread(void *arg)
{
    int status;

    ULONG_PTR key = 0;
    DWORD bytes = 0;
    OVERLAPPED *overlapped = NULL;
    struct iocp_ctx *c = (struct iocp_ctx *)arg;
    while (c->is_run) {
        status = GetQueuedCompletionStatus(c->port, &bytes, &key, &overlapped, c->ms);
        if (key != NOTIFICATION_KEY && overlapped)
            iocp_cb(overlapped, key, bytes, status);
        else if (!overlapped)
            break;
    }
    return NULL;
}

static void *iocp_init(void)
{
    int i;
    int ncpus = 0;
    struct iocp_ctx *c = (struct iocp_ctx *)calloc(1, sizeof(struct iocp_ctx));
    if (!c) {
        printf("malloc iocp_ctx failed!\n");
        return NULL;
    }

    ncpus = get_nprocs();
    c->thread_nums = ncpus * 2;
    c->tids = (pthread_t *)calloc(c->thread_nums, sizeof(pthread_t));
    if (!c->tids) {
        printf("malloc tids failed!\n");
        return NULL;
    }

    c->port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    c->is_run = true;
    c->ms = INFINITE;
    for (i = 0; i < c->thread_nums; i++) {
        pthread_create(&c->tids[i], NULL, iocp_thread, c);
    }
    return c;
}

static void iocp_deinit(void *ctx)
{
    struct iocp_ctx *c = (struct iocp_ctx *)ctx;
    CloseHandle(c->port);
    free(c->tids);
    free(c);
}

static int iocp_add(struct gevent_base *eb, struct gevent *e)
{
    struct iocp_ctx *c = (struct iocp_ctx *)eb->ctx;
    if (INVALID_HANDLE_VALUE == CreateIoCompletionPort((HANDLE)e->evfd, c->port, c->key, c->thread_nums)) {
        printf("CreateIoCompletionPort failed!\n");
        return -1;
    }

    return 0;
}

static int iocp_del(struct gevent_base *eb, struct gevent *e)
{
    return 0;
}

static int iocp_dispatch(struct gevent_base *eb, struct timeval *tv)
{
    return 0;
}

struct gevent_ops iocpops = {
    iocp_init,
    iocp_deinit,
    iocp_add,
    iocp_del,
    iocp_dispatch,
};
