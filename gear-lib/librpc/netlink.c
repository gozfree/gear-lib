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
#include "netlink_driver.h"
#include <gear-lib/libmacro.h>
#include <gear-lib/libgevent.h>
#include <gear-lib/libthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <linux/netlink.h>
#include <semaphore.h>
#include <errno.h>

/*
 * netlink RPC build flow:
 *
 *            client (userspace)       proxy (kernel)                            server (userspace)
 * step.1                              netlink_kernel_create (PORT)
 * step.2 bind(PORT), send "/RPC_CLIENT.$pid", wait msg
 * step.3                              recv msg, send "/RPC_CLIENT.$pid"
 * step.4 recv msg
 *
 *
 *
 *
 *
 */

#define MAX_NAME    256


struct nl_ctx {
    int fd;
    sem_t sem;
    char rd_name[MAX_NAME];
    rpc_role role;
    int connected;
    struct gevent_base *evbase;
};

static char *_nl_recv_buf = NULL;
static rpc_recv_cb *_nl_recv_cb = NULL;
static struct gevent *ev_on_conn_resp;
static struct gevent *ev_on_recv;

static int nl_recv(struct rpc *rpc, void *buf, size_t len);

static int nlmsg_seq = 0;
static const char *nl_dir[] = {
        "SERVER_TO_SERVER",
        "SERVER_TO_CLIENT",
        "CLIENT_TO_SERVER",
        "CLIENT_TO_CLIENT"};

#define nl_debug(ctx, nlhdr) \
    do { \
        printf("============================\n"); \
        printf("netlink status connected[%d]\n", ctx->connected); \
        printf("nl_msg direction: %s\n", nl_dir[nlhdr->nlmsg_type]); \
        printf("nl_msg sequence number: %d\n", nlhdr->nlmsg_seq); \
        printf("============================\n"); \
    } while (0);


static void *nl_init(struct rpc *rpc, const char *host, uint16_t port, enum rpc_role role)
{
    int ret;
    struct sockaddr_nl saddr;
    char name[256] = {0};
    int fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USERSOCK);
    if (fd == -1) {
        printf("socket failed: %d:%s\n", errno, strerror(errno));
        return NULL;
    }
    memset(&saddr, 0, sizeof(saddr));
    saddr.nl_family = AF_NETLINK;
    saddr.nl_pid = getpid();
    if (role == RPC_SERVER) {
        saddr.nl_groups = NETLINK_RPC_GROUP_SERVER;
    } else {
        saddr.nl_groups = NETLINK_RPC_GROUP_CLIENT;
    }
    saddr.nl_pad = 0;
    ret = bind(fd, (struct sockaddr *)&saddr, sizeof(saddr));
    if (ret < 0) {
        printf("bind failed: %d:%s\n", errno, strerror(errno));
        if (errno == EPERM) {
            printf("using sudo to root\n");
        }
        return NULL;
    }
    struct nl_ctx *ctx = CALLOC(1, struct nl_ctx);
    if (!ctx) {
        printf("malloc failed!\n");
        return NULL;
    }
    ctx->fd = fd;
    ctx->role = role;
    ctx->connected = 0;
    strncpy(ctx->rd_name, name, sizeof(ctx->rd_name));
    if (-1 == sem_init(&ctx->sem, 0, 0)) {
        printf("sem_init failed %d:%s\n", errno, strerror(errno));
        free(ctx);
        return NULL;
    }
    ctx->evbase = gevent_base_create();
    if (!ctx->evbase) {
        printf("gevent_base_create failed!\n");
        free(ctx);
        return NULL;
    }
    _nl_recv_buf = (char *)calloc(1, MAX_RPC_MESSAGE_SIZE);
    if (!_nl_recv_buf) {
        printf("malloc failed!\n");
        free(ctx);
        return NULL;
    }
    return ctx;
}

static int nl_send(struct rpc *rpc, const void *buf, size_t len)
{
    struct nl_ctx *ctx = (struct nl_ctx *)rpc->ctx;
    struct sockaddr_nl daddr;
    struct msghdr msg;
    struct nlmsghdr *nlhdr = NULL;
    struct iovec iov;
    int ret;

    memset(&daddr, 0, sizeof(daddr));
    daddr.nl_family = AF_NETLINK;
    daddr.nl_pid = 0;
    if (ctx->role == RPC_SERVER) {
        daddr.nl_groups = NETLINK_RPC_GROUP_CLIENT;
    } else if (ctx->role == RPC_CLIENT) {
        daddr.nl_groups = NETLINK_RPC_GROUP_SERVER;
    } else {
    }
    daddr.nl_pad = 0;

    nlhdr = (struct nlmsghdr *)calloc(1, NLMSG_LENGTH(len+1));
    nlhdr->nlmsg_pid = getpid();
    nlhdr->nlmsg_len = NLMSG_LENGTH(len+1);
    nlhdr->nlmsg_flags = 0;
    nlhdr->nlmsg_seq = nlmsg_seq++;
    if (ctx->role == RPC_SERVER) {
        daddr.nl_groups = NETLINK_RPC_GROUP_CLIENT;
        if (ctx->connected) {
            nlhdr->nlmsg_type = SERVER_TO_CLIENT;
            printf("SERVER_TO_CLIENT\n");
        } else {
            nlhdr->nlmsg_type = SERVER_TO_SERVER;
            printf("SERVER_TO_SERVER\n");
        }
    } else if (ctx->role == RPC_CLIENT) {
        daddr.nl_groups = NETLINK_RPC_GROUP_SERVER;
        if (ctx->connected) {
            nlhdr->nlmsg_type = CLIENT_TO_SERVER;
            printf("CLIENT_TO_SERVER\n");
        } else {
            nlhdr->nlmsg_type = CLIENT_TO_CLIENT;
            printf("CLIENT_TO_CLIENT\n");
        }
    } else {
    }
    memcpy(NLMSG_DATA(nlhdr), buf, len);

    memset(&msg, 0, sizeof(struct msghdr));
    iov.iov_base = (void *)nlhdr;
    iov.iov_len = nlhdr->nlmsg_len;
    msg.msg_name = (void *)&daddr;
    msg.msg_namelen = sizeof(daddr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    nl_debug(ctx, nlhdr);
    printf("nlmsg_pid=%d, nlmsg_len=%d\n", nlhdr->nlmsg_pid, nlhdr->nlmsg_len);
    ret = sendmsg(ctx->fd, &msg, 0);
    if (ret == -1) {
        printf("sendmsg failed: %d:%s\n", errno, strerror(errno));
    }
    free(nlhdr);
    return ret;
}

static int nl_recv(struct rpc *rpc, void *buf, size_t len)
{
    struct nl_ctx *ctx = (struct nl_ctx *)rpc->ctx;
    struct sockaddr_nl sa;
    struct nlmsghdr *nlhdr = NULL;
    struct msghdr msg;
    struct iovec iov;
    int ret;
    char nlhdr_buf[MAX_RPC_MESSAGE_SIZE];

    memset(nlhdr_buf, 0, sizeof(nlhdr_buf));
    nlhdr = (struct nlmsghdr *)nlhdr_buf;
    iov.iov_base = (void *)nlhdr;
    iov.iov_len = MAX_RPC_MESSAGE_SIZE;

    memset(&sa, 0, sizeof(sa));
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (void *)&(sa);
    msg.msg_namelen = sizeof(sa);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    ret = recvmsg(ctx->fd, &msg, 0);
    if (ret == -1) {
        printf("recvmsg failed %d:%s\n", errno, strerror(errno));
        return ret;
    }
    len = nlhdr->nlmsg_len - NLMSG_HDRLEN;
    memcpy(buf, NLMSG_DATA(nlhdr), len);
    nl_debug(ctx, nlhdr);
    if (ctx->role == RPC_SERVER) {
        if (nlhdr->nlmsg_type == SERVER_TO_CLIENT ||
            nlhdr->nlmsg_type == CLIENT_TO_CLIENT) {
            printf("ingore msg\n");
            return 0;
        }
    }
    if (ctx->role == RPC_CLIENT) {
        if (nlhdr->nlmsg_type == CLIENT_TO_SERVER ||
            nlhdr->nlmsg_type == SERVER_TO_SERVER) {
            printf("ingore msg\n");
            return 0;
        }
    }
    return ret;
}

static void on_error(int fd, void *arg)
{
    printf("error: %d\n", errno);
}

static void on_recv(int fd, void *arg)
{
    struct rpc *rpc = (struct rpc *)arg;
    int len;
    len = nl_recv(rpc, _nl_recv_buf, MAX_RPC_MESSAGE_SIZE);
    if (len == -1) {
        printf("nl_recv failed!\n");
        return;
    } else if (len == 0) {
        printf("nl_recv null msg!\n");
        return;
    }
    if (_nl_recv_cb) {
        _nl_recv_cb(rpc, _nl_recv_buf, len);
    }
}

static void on_conn_resp(int fd, void *arg)
{
    struct rpc *rpc = (struct rpc *)arg;
    struct nl_ctx *ctx = (struct nl_ctx *)rpc->ctx;
    int len;
    len = nl_recv(rpc, _nl_recv_buf, MAX_RPC_MESSAGE_SIZE);
    if (len == -1) {
        printf("nl_recv failed!\n");
        return;
    } else if (len == 0) {
        printf("nl_recv null msg!\n");
        return;
    }
    printf("recv len=%d, buf=%s\n", len, _nl_recv_buf);
    if (_nl_recv_cb) {
        _nl_recv_cb(rpc, _nl_recv_buf, len);
    }
    if (strncmp(_nl_recv_buf, ctx->rd_name, len)) {
        printf("connect response check failed!\n");
    }
    gevent_del(ctx->evbase, ev_on_conn_resp);
    ev_on_recv = gevent_create(ctx->fd, on_recv, NULL, on_error, rpc);
    if (-1 == gevent_add(ctx->evbase, ev_on_recv)) {
        printf("gevent_add failed!\n");
        return;
    }
    sem_post(&ctx->sem);
}

static void *connecting_thread(void *arg)
{
//wait connect response
    struct rpc *rpc = (struct rpc *)arg;
    struct nl_ctx *ctx = (struct nl_ctx *)rpc->ctx;
    ev_on_conn_resp = gevent_create(ctx->fd, on_conn_resp, NULL, on_error, rpc);
    if (-1 == gevent_add(ctx->evbase, ev_on_conn_resp)) {
        printf("gevent_add failed!\n");
        return NULL;
    }
    gevent_base_loop(ctx->evbase);

    return NULL;
}

static int nl_accept(struct rpc *rpc)
{
    pthread_t tid;
    struct nl_ctx *ctx = (struct nl_ctx *)rpc->ctx;
    printf("nl_send len=%zu, buf=%s\n", strlen(ctx->rd_name), ctx->rd_name);
    if (-1 == nl_send(rpc, ctx->rd_name, strlen(ctx->rd_name))) {
        printf("nl_send failed!\n");
        return -1;
    }
    pthread_create(&tid, NULL, connecting_thread, rpc);
    struct timeval now;
    struct timespec abs_time;
    uint32_t timeout = 5000;//msec
    gettimeofday(&now, NULL);
    now.tv_usec += (timeout % 1000) * 1000;
    now.tv_sec += timeout / 1000;
    if (now.tv_usec >= 1000000) {
        now.tv_usec -= 1000000;
        now.tv_sec++;
    }
    /* Convert to timespec */
    abs_time.tv_sec = now.tv_sec;
    abs_time.tv_nsec = now.tv_usec * 1000;
    printf("nl_accept\n");
    if (-1 == sem_timedwait(&ctx->sem, &abs_time)) {
        printf("accept failed %d: %s\n", errno, strerror(errno));
        return -1;
    }
    ctx->connected = 1;
    return 0;
}

static int nl_connect(struct rpc *rpc, const char *name)
{
    pthread_t tid;
    struct nl_ctx *ctx = (struct nl_ctx *)rpc->ctx;
    printf("nl_send len=%zu, buf=%s\n", strlen(ctx->rd_name), ctx->rd_name);
    nl_send(rpc, ctx->rd_name, strlen(ctx->rd_name));
    pthread_create(&tid, NULL, connecting_thread, rpc);
    struct timeval now;
    struct timespec abs_time;
    uint32_t timeout = 5000;//msec
    gettimeofday(&now, NULL);
    /* Add our timeout to current time */
    now.tv_usec += (timeout % 1000) * 1000;
    now.tv_sec += timeout / 1000;
    /* Wrap the second if needed */
    if ( now.tv_usec >= 1000000 ) {
        now.tv_usec -= 1000000;
        now.tv_sec ++;
    }
    /* Convert to timespec */
    abs_time.tv_sec = now.tv_sec;
    abs_time.tv_nsec = now.tv_usec * 1000;
    if (-1 == sem_timedwait(&ctx->sem, &abs_time)) {
        printf("connect %s failed %d: %s\n", name, errno, strerror(errno));
        return -1;
    }
    ctx->connected = 1;
    return 0;
}


static int nl_set_recv_cb(struct rpc *rpc, rpc_recv_cb *cb)
{
    _nl_recv_cb = cb;
    return 0;
}

static void nl_deinit(struct rpc *rpc)
{
    if (!rpc) {
        return;
    }
    struct nl_ctx *ctx = (struct nl_ctx *)rpc->ctx;
    close(ctx->fd);
    sem_destroy(&ctx->sem);
    if (_nl_recv_buf) {
        free(_nl_recv_buf);
    }
    free(ctx);
}

struct rpc_ops netlink_ops = {
    .init             = nl_init,
    .deinit           = nl_deinit,
    .accept           = nl_accept,
    .connect          = nl_connect,
    .register_recv_cb = nl_set_recv_cb,
    .send             = nl_send,
    .recv             = nl_recv,
};
