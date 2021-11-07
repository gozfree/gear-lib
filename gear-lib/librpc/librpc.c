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
#include "librpc.h"
#include <libgevent.h>
#include <libhash.h>
#include <libthread.h>
#include <libtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define ENABLE_DEBUG 0

extern struct rpc_ops socket_ops;

struct rpc_backend {
    int id;
    struct rpc_ops *ops;
};

typedef enum rpc_backend_type {
    RPC_SOCKET,
    RPC_UART,
} rpc_backend_type;

static struct rpc_backend rpc_backend_list[] = {
    {RPC_SOCKET,   &socket_ops},
};

#define RPC_BACKEND RPC_SOCKET
#define MAX_UUID_LEN                (21)
#define MAX_MSG_ID_STRLEN           (11)

struct wq_arg {
    msg_handler_t handler;
    struct rpc_session session;
    void *ibuf;
    size_t ilen;
    void *obuf;
    size_t olen;
    struct rpcs *rpcs;
};

static struct hash *_msg_map_registered = NULL;

static void dump_buffer(void *buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (!(i%16))
           printf("\n%p: ", (char *)buf+i);
        printf("%02x ", (*((char *)buf + i)) & 0xff);
    }
    printf("\n");
}

static void dump_packet(struct rpc_packet *pkt)
{
    printf("packet header (%zu):", sizeof(struct rpc_header));
    dump_buffer(&pkt->header, (int)sizeof(struct rpc_header));
    printf("packet payload (%d):", pkt->header.payload_len);
    dump_buffer(pkt->payload, pkt->header.payload_len);
}

void print_packet(struct rpc_packet *pkt)
{
    char ts[64];
    printf("================\n");
    printf("rpc_packet[%p]:\n", pkt);
    printf("header.uuid_dst    = 0x%08x\n", pkt->header.uuid_dst);
    printf("header.uuid_src    = 0x%08x\n", pkt->header.uuid_src);
    printf("header.msg_id      = 0x%08x\n", pkt->header.msg_id);
    printf("header.timestamp   = %" PRIu64 "(%s)\n", pkt->header.timestamp,
            time_str_format_by_msec(pkt->header.timestamp, ts, sizeof(ts)));
    printf("header.payload_len = %d\n", pkt->header.payload_len);
    printf("header.checksum    = 0x%08x\n", pkt->header.checksum);
    dump_packet(pkt);
    printf("================\n");
}

void print_session(struct rpc_session *ss)
{
    char ts[64];
    printf("================\n");
    printf("rpc_session[%p]:\n", ss);
    printf("session.base.fd     = %d\n", ss->base.fd);
    printf("session.uuid_dst    = 0x%08x\n", ss->uuid_dst);
    printf("session.uuid_src    = 0x%08x\n", ss->uuid_src);
    printf("session.msg_id      = 0x%08x\n", ss->msg_id);
    printf("session.timestamp   = %" PRIu64 "(%s)\n", ss->timestamp,
            time_str_format_by_msec(ss->timestamp, ts, sizeof(ts)));
    printf("session.checksum    = 0x%" PRIx64 "\n", ss->cseq);
    printf("================\n");
}

static void *event_thread(struct thread *t, void *arg)
{
    struct rpc_base *base = (struct rpc_base *)arg;
    gevent_base_loop(base->evbase);
    return NULL;
}

static int rpc_base_init(struct rpc_base *base)
{
    base->ops = rpc_backend_list[RPC_BACKEND].ops;
    base->evbase = gevent_base_create();
    if (!base->evbase) {
        printf("gevent_base_create failed!\n");
        return -1;
    }
    da_init(base->ev_list);
    base->dispatch_thread = thread_create(event_thread, base);
    if (!base->dispatch_thread) {
        printf("thread_create failed!\n");
        return -1;
    }
    return 0;
}

static void rpc_base_deinit(struct rpc_base *base)
{
    gevent_base_loop_break(base->evbase);
    gevent_base_destroy(base->evbase);
    thread_destroy(base->dispatch_thread);
    base->ops = NULL;
}

size_t pack_msg(struct rpc_packet *pkt, uint32_t uuid_dst, uint32_t uuid_src,
                uint32_t msg_id, const void *in_arg, size_t in_len)
{
    struct rpc_header *hdr;
    size_t pkt_len;
    struct time_info ti;

    hdr = &(pkt->header);
    hdr->uuid_dst = uuid_dst;
    hdr->uuid_src = uuid_src;
    hdr->msg_id = msg_id;
    time_now_info(&ti);
    hdr->timestamp = ti.utc_msec;

    if (in_arg) {
        hdr->payload_len = in_len;
        pkt->payload = (void *)in_arg;
    } else {
        hdr->payload_len = 0;
        pkt->payload = NULL;
    }
    pkt_len = sizeof(struct rpc_packet) + hdr->payload_len;
    return pkt_len;
}

static size_t unpack_msg(struct rpc_packet *pkt, uint32_t *msg_id,
                void *out_arg, size_t *out_len)
{
    size_t pkt_len;
    struct rpc_header *hdr;
    if (!pkt || !out_arg || !out_len) {
        printf("invalid paraments!\n");
        return -1;
    }
    hdr = &(pkt->header);
    *msg_id = hdr->msg_id;
    *out_len = hdr->payload_len;
    memcpy(out_arg, pkt->payload, *out_len);
    free(pkt->payload);
    pkt_len = sizeof(struct rpc_packet) + hdr->payload_len;
    return pkt_len;
}

int rpc_send(struct rpc_base *base, struct rpc_packet *pkt)
{
    int ret, head_size;
    void *buf = NULL;
    int len;

    head_size = sizeof(rpc_header_t);

#if ENABLE_DEBUG
    printf("rpc_send >>>>\n");
    struct rpc_session *session = container_of(base, struct rpc_session, base);
    print_session(session);
    print_packet(pkt);
#endif
    len = head_size + pkt->header.payload_len;
    buf = calloc(1, len);
    if (!buf) {
        printf("%s:%d alloc buf failed!\n", __func__, __LINE__);
        return -1;
    }
    memcpy(buf, (void *)&pkt->header, head_size);
    memcpy(buf+head_size, pkt->payload, pkt->header.payload_len);

    ret = base->ops->send(base, buf, len);
    if (ret < 0) {
        printf("%s:%d send failed!\n", __func__, __LINE__);
        ret = -1;
    } if (ret != len) {
        printf("%s:%d send len %d not matched %d failed!\n", __func__, __LINE__, ret, len);
        ret = -1;
    }
    free(buf);
    return ret;
}

static int rpc_recv(struct rpc_base *base, struct rpc_packet *pkt)
{
    int ret;
    int head_size = sizeof(rpc_header_t);

    ret = base->ops->recv(base, (void *)&pkt->header, head_size);
    if (ret == 0) {
        printf("peer connect closed\n");
        return 0;
    } else if (ret < 0) {
        printf("recv failed: %d\n", errno);
        return -1;
    } else if (ret != head_size) {
        printf("recv failed, head_size = %d, ret = %d\n", head_size, ret);
        return -1;
    }
    if (pkt->header.payload_len == 0)
        return ret;

    pkt->payload = calloc(1, pkt->header.payload_len);

    ret = base->ops->recv(base, pkt->payload, pkt->header.payload_len);
    if (ret == 0) {
        printf("peer connect closed\n");
        return 0;
    }
#if ENABLE_DEBUG
    printf("rpc_recv <<<<\n");
    struct rpc_session *session = container_of(base, struct rpc_session, base);
    print_session(session);
    print_packet(pkt);
#endif
    return ret;
}

static msg_handler_t *find_msg_handler(uint32_t msg_id)
{
    char msg_id_str[MAX_MSG_ID_STRLEN];
    msg_handler_t *handler;
    snprintf(msg_id_str, sizeof(msg_id_str), "0x%08x", msg_id);
    msg_id_str[10] = '\0';
    handler = (msg_handler_t *)hash_get(_msg_map_registered, msg_id_str);
    return handler;
}

static int register_msg_proc(msg_handler_t *handler)
{
    uint32_t msg_id;
    char msg_id_str[MAX_MSG_ID_STRLEN];
    char *msg_proc;

    if (!handler) {
        printf("Cannot register null msg proc\n");
        return -1;
    }
    msg_id = handler->msg_id;
    snprintf(msg_id_str, sizeof(msg_id_str), "0x%08x", msg_id);
    msg_id_str[10] = '\0';
    if (!_msg_map_registered) {
            _msg_map_registered = hash_create(10240);
            if (!_msg_map_registered) {
                    printf("create message map table failed!\n");
                    return -1;
            }
    }
    msg_proc = hash_get(_msg_map_registered, msg_id_str);
    if (msg_proc) {//force update
        if (0 != hash_del(_msg_map_registered, msg_id_str)) {
            printf("hash_del failed!\n");
        }
    }
    if (0 != hash_set(_msg_map_registered, msg_id_str, (char *)handler)) {
        printf("hash_add failed!\n");
    }
    return 0;
}

int register_msg_map(msg_handler_t *map, int num_entry)
{
    int i;
    int ret;

    if (map == NULL) {
        printf("register_msg_map: null map \n");
        return -1;
    }

    if (num_entry <= 0) {
        printf("register_msg_map:invalid num_entry %d \n", num_entry);
        return -1;
    }

    for (i = 0; i < num_entry; i++) {
        ret = register_msg_proc(&map[i]);
        if (ret < 0) {
            printf("register_msg_map:register failed at %d \n", i);
            return -1;
        }
        printf("register_msg_map <msg_id, func> : <0x%d: %p>\n", map[i].msg_id, map[i].cb);
    }
    return 0;
}

/******************************************************************************
 * client API
 ******************************************************************************/
int rpc_call(struct rpc *rpc, uint32_t msg_id,
             const void *in_arg, size_t in_len, void *out_arg, size_t out_len)
{
    int ret;
    struct rpc_session *ss = &rpc->session;
    struct rpc_packet send_pkt, recv_pkt;
    if (!rpc) {
        printf("invalid parament!\n");
        return -1;
    }
    size_t pkt_len = pack_msg(&send_pkt, ss->uuid_dst, ss->uuid_src, msg_id, in_arg, in_len);
    if (pkt_len == 0) {
        printf("pack_msg failed!\n");
        return -1;
    }
    ret = rpc_send(&ss->base, &send_pkt);
    if (-1 == ret) {
        printf("rpc_send failed\n");
        return -1;
    }
    if (IS_RPC_MSG_NEED_RETURN(msg_id)) {
        thread_lock(ss->base.dispatch_thread);
        rpc->state = rpc_send_syn;
        if (thread_wait(ss->base.dispatch_thread, 2000) == -1) {
            printf("%s wait response failed %d:%s\n", __func__, errno, strerror(errno));
            return -1;
        }
        thread_unlock(ss->base.dispatch_thread);
        rpc->state = rpc_connected;
        memset(&recv_pkt, 0, sizeof(recv_pkt));
        if (-1 == rpc_recv(&ss->base, &recv_pkt)) {
            printf("rpc_recv failed!\n");
            return -1;
        }
        unpack_msg(&recv_pkt, &msg_id, out_arg, &out_len);
    }
    return 0;
}

static int on_connect_to_server(struct rpc *rpc)
{
    struct rpc_packet pkt;
    msg_handler_t *msg_handler;
    struct rpc_session *ss = &rpc->session;

    if (rpc->state == rpc_inited) {
        memset(&pkt, 0, sizeof(pkt));
        if (-1 == rpc_recv(&ss->base, &pkt)) {
            printf("rpc_recv failed!\n");
            return -1;
        }
        memcpy(&ss->uuid_src, pkt.payload, sizeof(uint32_t));
        free(pkt.payload);
        thread_lock(ss->base.dispatch_thread);
        thread_signal(ss->base.dispatch_thread);
        thread_unlock(ss->base.dispatch_thread);
        rpc->state = rpc_connected;
#if ENABLE_DEBUG
        printf("rpc state: rpc_inited -> rpc_connecting\n");
#endif
    } else if (rpc->state == rpc_connecting) {
        thread_lock(ss->base.dispatch_thread);
        thread_signal(ss->base.dispatch_thread);
        thread_unlock(ss->base.dispatch_thread);
        rpc->state = rpc_connected;
#if ENABLE_DEBUG
        printf("rpc state: rpc_connecting -> rpc_connected\n");
#endif
    } else if (rpc->state == rpc_connected) {
        memset(&pkt, 0, sizeof(pkt));
        if (-1 == rpc_recv(&ss->base, &pkt)) {
            printf("rpc_recv failed!\n");
            return -1;
        }
        msg_handler = find_msg_handler(pkt.header.msg_id);
        if (msg_handler) {
            msg_handler->cb(ss, pkt.payload, pkt.header.payload_len, NULL, NULL);
        }
#if ENABLE_DEBUG
        printf("rpc state: rpc_connected -> rpc_connected\n");
#endif
    } else if (rpc->state == rpc_send_syn) {
        thread_lock(ss->base.dispatch_thread);
        thread_signal(ss->base.dispatch_thread);
        thread_unlock(ss->base.dispatch_thread);
        printf("rpc state: ... -> rpc_send_syn\n");
    } else if (rpc->state == rpc_send_ack) {
    } else {
        printf("rpc state is invalid!\n");
    }
    return 0;
}

struct rpc *rpc_client_create(const char *host, uint16_t port)
{
    struct rpc *rpc = calloc(1, sizeof(struct rpc));
    if (!rpc) {
        printf("malloc failed!\n");
        return NULL;
    }
    if (rpc_base_init(&rpc->session.base) < 0) {
        printf("rpc_base_init failed!\n");
        return NULL;
    }
    rpc->on_connect_server = on_connect_to_server;
    rpc->state = rpc_inited;
    if (rpc->session.base.ops->init_client(&rpc->session.base, host, port) < 0) {
        printf("init_client failed!\n");
        goto failed;
    }

    printf("rpc_client_create success, uuid = 0x%08X\n", rpc->session.uuid_src);
    return rpc;

failed:
    free(rpc);
    return NULL;
}

int rpc_client_dispatch(struct rpc *rpc)
{
    return gevent_base_loop(rpc->session.base.evbase);
}

void rpc_client_destroy(struct rpc *rpc)
{
    if (!rpc) {
        return;
    }
    rpc->session.base.ops->deinit(&rpc->session.base);
    rpc_base_deinit(&rpc->session.base);
    free(rpc);
}

GEAR_API int rpc_client_set_dest(struct rpc *rpc, uint32_t uuid_dst)
{
    rpc->session.uuid_dst = uuid_dst;
    return 0;
}

void rpc_client_dump_info(struct rpc *rpc)
{
    print_session(&rpc->session);
}

/******************************************************************************
 * server API
 ******************************************************************************/
static struct rpc_session *rpc_session_create(struct rpcs *s, int fd, uint32_t uuid)
{
    struct rpc_session *session;
    struct time_info ti;
    struct rpc_packet pkt;
    int ret;

    session = hash_get32(s->hash_session, uuid);
    if (session) {
        printf("rpc session %d already exist!\n", uuid);
        return session;
    }
    session = (struct rpc_session *)calloc(1, sizeof(struct rpc_session));
    if (!session) {
        printf("malloc rpc_session failed!\n");
        return NULL;
    }
    memcpy(&session->base, &s->base, sizeof(struct rpc_base));
    session->base.fd = fd;
    session->uuid_src = uuid;
    session->cseq = 0;
    hash_set32(s->hash_session, uuid, session);

    memset(&pkt, 0, sizeof(pkt));
    pkt.header.uuid_src = uuid;
    pkt.header.uuid_dst = uuid;
    pkt.header.msg_id = 0;
    pkt.header.payload_len = sizeof(uuid);
    pkt.payload = &uuid;
    time_now_info(&ti);
    pkt.header.timestamp = ti.utc_msec;

    ret = rpc_send(&session->base, &pkt);
    if (ret == -1) {
        printf("%s: rpc_send failed\n", __func__);
    }
    return session;
}

static void rpc_session_destroy(struct rpcs *s, uint32_t uuid)
{
    struct rpc_session *session;
    session = hash_get32(s->hash_session, uuid);
    if (!session) {
        printf("rpc session %d does not exist!\n", uuid);
        return;
    }
    free(session);
    hash_del32(s->hash_session, uuid);
    printf("rpc_session_destroy: uuid:0x%08x\n", uuid);
}

static void process_wq(void *arg)
{
    struct wq_arg *wq = (struct wq_arg *)arg;
    struct rpc_session *session = &wq->session;
    struct rpc_packet pkt;
    if (&wq->handler) {
        wq->handler.cb(session, wq->ibuf, wq->ilen, &wq->obuf, &wq->olen);
        if (IS_RPC_MSG_NEED_RETURN(wq->handler.msg_id)) {
            pack_msg(&pkt, 0, session->uuid_src, wq->handler.msg_id, wq->obuf, wq->olen);
            rpc_send(&session->base, &pkt);
        }
    }
}

struct rpcs *rpc_server_get_handle(struct rpc_session *ss)
{
    struct wq_arg *wq_arg = container_of(ss, struct wq_arg, session);
    return wq_arg->rpcs;
}

static int process_msg(struct rpcs *s, struct rpc_session *session, struct rpc_packet *pkt)
{
    int ret = 0;
    msg_handler_t *msg_handler;
    struct rpc_header *h = &pkt->header;

    msg_handler = find_msg_handler(h->msg_id);
    if (msg_handler) {
        struct wq_arg *arg = calloc(1, sizeof(struct wq_arg));
        struct rpc_session *ss = &arg->session;
        arg->rpcs = s;
        memcpy(&arg->handler, msg_handler, sizeof(msg_handler_t));
        memcpy(&arg->session, session, sizeof(struct rpc_session));
        ss->uuid_dst = pkt->header.uuid_dst;
        ss->timestamp = pkt->header.timestamp;
        ss->msg_id = pkt->header.msg_id;
        arg->ibuf = memdup(pkt->payload, h->payload_len);
        arg->ilen = h->payload_len;

        workq_pool_task_push(s->wq_pool, process_wq, arg);
    } else {
        printf("no callback for this MSG ID(%d) in process_msg\n", h->msg_id);
    }
    return ret;
}

static int on_message_from_client(struct rpcs *s, struct rpc_session *session)
{
    int ret;
    struct rpc_packet pkt;

    ret = rpc_recv(&session->base, &pkt);
    if (ret == 0) {
        printf("del connect: uuid:0x%08x\n", session->uuid_src);
        rpc_session_destroy(s, session->uuid_src);
    } else if (ret == -1) {
        printf("rpc_recv failed\n");
    } else {
        ret = process_msg(s, session, &pkt);
    }
    return ret;
}

struct rpcs *rpc_server_create(const char *host, uint16_t port)
{
    struct rpcs *s = calloc(1, sizeof(struct rpcs));
    if (!s) {
        printf("malloc rpcs failed!\n");
        return NULL;
    }
    if (rpc_base_init(&s->base) < 0) {
        printf("rpc_base_init failed!\n");
        goto failed;
    }

    s->hash_session = hash_create(1024);
    s->hash_fd2session = hash_create(10240);
    s->wq_pool = workq_pool_create();
    s->on_create_session = rpc_session_create;
    s->on_message = on_message_from_client;
    if (s->base.ops->init_server(&s->base, host, port) < 0) {
        printf("init_server failed!\n");
        goto failed;
    }
    printf("rpc_server_create success\n");
    return s;

failed:
    if (s) {
        free(s);
    }
    return NULL;
}

int rpc_server_dispatch(struct rpcs *s)
{
    return gevent_base_loop(s->base.evbase);
}

void rpc_server_destroy(struct rpcs *s)
{
    if (!s) {
        return;
    }
    s->base.ops->deinit(&s->base);
    workq_pool_destroy(s->wq_pool);
    rpc_base_deinit(&s->base);
    free(s);
}
