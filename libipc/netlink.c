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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <linux/netlink.h>
#include <semaphore.h>
#include <libmacro.h>
#include <liblog.h>
#include <libgevent.h>
#include <libthread.h>
#include "libipc.h"
#include "netlink_driver.h"

/*
 * netlink IPC build flow:
 *
 *            client (userspace)       proxy (kernel)                            server (userspace)
 * step.1                              netlink_kernel_create (PORT)
 * step.2 bind(PORT), send "/IPC_CLIENT.$pid", wait msg
 * step.3                              recv msg, send "/IPC_CLIENT.$pid"
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
    ipc_role role;
    int connected;
    struct gevent_base *evbase;
};

static char *_nl_recv_buf = NULL;
static ipc_recv_cb *_nl_recv_cb = NULL;
static struct gevent *ev_on_conn_resp;
static struct gevent *ev_on_recv;

static int nl_recv(struct ipc *ipc, void *buf, size_t len);

static int nlmsg_seq = 0;
static const char *nl_dir[] = {
        "SERVER_TO_SERVER",
        "SERVER_TO_CLIENT",
        "CLIENT_TO_SERVER",
        "CLIENT_TO_CLIENT"};

#define nl_debug(ctx, nlhdr) \
    do { \
        logd("============================\n"); \
        logd("netlink status connected[%d]\n", ctx->connected); \
        logd("nl_msg direction: %s\n", nl_dir[nlhdr->nlmsg_type]); \
        logd("nl_msg sequence number: %d\n", nlhdr->nlmsg_seq); \
        logd("============================\n"); \
    } while (0);


static void *nl_init(const char *name, enum ipc_role role)
{
    int ret;
    struct sockaddr_nl saddr;
    int fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USERSOCK);
    if (fd == -1) {
        loge("socket failed: %d:%s\n", errno, strerror(errno));
        return NULL;
    }
    memset(&saddr, 0, sizeof(saddr));
    saddr.nl_family = AF_NETLINK;
    saddr.nl_pid = getpid();
    if (role == IPC_SERVER) {
        saddr.nl_groups = NETLINK_IPC_GROUP_SERVER;
    } else {
        saddr.nl_groups = NETLINK_IPC_GROUP_CLIENT;
    }
    saddr.nl_pad = 0;
    ret = bind(fd, (struct sockaddr *)&saddr, sizeof(saddr));
    if (ret < 0) {
        loge("bind failed: %d:%s\n", errno, strerror(errno));
        if (errno == EPERM) {
            loge("using sudo to root\n");
        }
        return NULL;
    }
    struct nl_ctx *ctx = CALLOC(1, struct nl_ctx);
    if (!ctx) {
        loge("malloc failed!\n");
        return NULL;
    }
    ctx->fd = fd;
    ctx->role = role;
    ctx->connected = 0;
    strncpy(ctx->rd_name, name, sizeof(ctx->rd_name));
    if (-1 == sem_init(&ctx->sem, 0, 0)) {
        loge("sem_init failed %d:%s\n", errno, strerror(errno));
        free(ctx);
        return NULL;
    }
    ctx->evbase = gevent_base_create();
    if (!ctx->evbase) {
        loge("gevent_base_create failed!\n");
        free(ctx);
        return NULL;
    }
    _nl_recv_buf = (char *)calloc(1, MAX_IPC_MESSAGE_SIZE);
    if (!_nl_recv_buf) {
        loge("malloc failed!\n");
        free(ctx);
        return NULL;
    }
    return ctx;
}

static int nl_send(struct ipc *ipc, const void *buf, size_t len)
{
    struct nl_ctx *ctx = (struct nl_ctx *)ipc->ctx;
    struct sockaddr_nl daddr;
    struct msghdr msg;
    struct nlmsghdr *nlhdr = NULL;
    struct iovec iov;
    int ret;

    memset(&daddr, 0, sizeof(daddr));
    daddr.nl_family = AF_NETLINK;
    daddr.nl_pid = 0;
    if (ctx->role == IPC_SERVER) {
        daddr.nl_groups = NETLINK_IPC_GROUP_CLIENT;
    } else if (ctx->role == IPC_CLIENT) {
        daddr.nl_groups = NETLINK_IPC_GROUP_SERVER;
    } else {
    }
    daddr.nl_pad = 0;

    nlhdr = (struct nlmsghdr *)calloc(1, NLMSG_LENGTH(len+1));
    nlhdr->nlmsg_pid = getpid();
    nlhdr->nlmsg_len = NLMSG_LENGTH(len+1);
    nlhdr->nlmsg_flags = 0;
    nlhdr->nlmsg_seq = nlmsg_seq++;
    if (ctx->role == IPC_SERVER) {
        daddr.nl_groups = NETLINK_IPC_GROUP_CLIENT;
        if (ctx->connected) {
            nlhdr->nlmsg_type = SERVER_TO_CLIENT;
            logd("SERVER_TO_CLIENT\n");
        } else {
            nlhdr->nlmsg_type = SERVER_TO_SERVER;
            logd("SERVER_TO_SERVER\n");
        }
    } else if (ctx->role == IPC_CLIENT) {
        daddr.nl_groups = NETLINK_IPC_GROUP_SERVER;
        if (ctx->connected) {
            nlhdr->nlmsg_type = CLIENT_TO_SERVER;
            logd("CLIENT_TO_SERVER\n");
        } else {
            nlhdr->nlmsg_type = CLIENT_TO_CLIENT;
            logd("CLIENT_TO_CLIENT\n");
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
    logd("nlmsg_pid=%d, nlmsg_len=%d\n", nlhdr->nlmsg_pid, nlhdr->nlmsg_len);
    ret = sendmsg(ctx->fd, &msg, 0);
    if (ret == -1) {
        loge("sendmsg failed: %d:%s\n", errno, strerror(errno));
    }
    free(nlhdr);
    return ret;
}

static int nl_recv(struct ipc *ipc, void *buf, size_t len)
{
    struct nl_ctx *ctx = (struct nl_ctx *)ipc->ctx;
    struct sockaddr_nl sa;
    struct nlmsghdr *nlhdr = NULL;
    struct msghdr msg;
    struct iovec iov;
    int ret;
    char nlhdr_buf[MAX_IPC_MESSAGE_SIZE];

    memset(nlhdr_buf, 0, sizeof(nlhdr_buf));
    nlhdr = (struct nlmsghdr *)nlhdr_buf;
    iov.iov_base = (void *)nlhdr;
    iov.iov_len = MAX_IPC_MESSAGE_SIZE;

    memset(&sa, 0, sizeof(sa));
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (void *)&(sa);
    msg.msg_namelen = sizeof(sa);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    ret = recvmsg(ctx->fd, &msg, 0);
    if (ret == -1) {
        loge("recvmsg failed %d:%s\n", errno, strerror(errno));
        return ret;
    }
    len = nlhdr->nlmsg_len - NLMSG_HDRLEN;
    memcpy(buf, NLMSG_DATA(nlhdr), len);
    nl_debug(ctx, nlhdr);
    if (ctx->role == IPC_SERVER) {
        if (nlhdr->nlmsg_type == SERVER_TO_CLIENT ||
            nlhdr->nlmsg_type == CLIENT_TO_CLIENT) {
            logd("ingore msg\n");
            return 0;
        }
    }
    if (ctx->role == IPC_CLIENT) {
        if (nlhdr->nlmsg_type == CLIENT_TO_SERVER ||
            nlhdr->nlmsg_type == SERVER_TO_SERVER) {
            logd("ingore msg\n");
            return 0;
        }
    }
    return ret;
}

static void on_error(int fd, void *arg)
{
    logi("error: %d\n", errno);
}

static void on_recv(int fd, void *arg)
{
    struct ipc *ipc = (struct ipc *)arg;
    int len;
    len = nl_recv(ipc, _nl_recv_buf, MAX_IPC_MESSAGE_SIZE);
    if (len == -1) {
        loge("nl_recv failed!\n");
        return;
    } else if (len == 0) {
        logd("nl_recv null msg!\n");
        return;
    }
    if (_nl_recv_cb) {
        _nl_recv_cb(ipc, _nl_recv_buf, len);
    }
}

static void on_conn_resp(int fd, void *arg)
{
    struct ipc *ipc = (struct ipc *)arg;
    struct nl_ctx *ctx = (struct nl_ctx *)ipc->ctx;
    int len;
    len = nl_recv(ipc, _nl_recv_buf, MAX_IPC_MESSAGE_SIZE);
    if (len == -1) {
        loge("nl_recv failed!\n");
        return;
    } else if (len == 0) {
        logd("nl_recv null msg!\n");
        return;
    }
    logd("recv len=%d, buf=%s\n", len, _nl_recv_buf);
    if (_nl_recv_cb) {
        _nl_recv_cb(ipc, _nl_recv_buf, len);
    }
    if (strncmp(_nl_recv_buf, ctx->rd_name, len)) {
        loge("connect response check failed!\n");
    }
    gevent_del(ctx->evbase, ev_on_conn_resp);
    ev_on_recv = gevent_create(ctx->fd, on_recv, NULL, on_error, ipc);
    if (-1 == gevent_add(ctx->evbase, ev_on_recv)) {
        loge("gevent_add failed!\n");
        return;
    }
    sem_post(&ctx->sem);
}

static void *connecting_thread(struct thread *thd, void *arg)
{
//wait connect response
    struct ipc *ipc = (struct ipc *)arg;
    struct nl_ctx *ctx = (struct nl_ctx *)ipc->ctx;
    ev_on_conn_resp = gevent_create(ctx->fd, on_conn_resp, NULL, on_error, ipc);
    if (-1 == gevent_add(ctx->evbase, ev_on_conn_resp)) {
        loge("gevent_add failed!\n");
        return NULL;
    }
    gevent_base_loop(ctx->evbase);

    return NULL;
}

static int nl_accept(struct ipc *ipc)
{
    struct nl_ctx *ctx = (struct nl_ctx *)ipc->ctx;
    logd("nl_send len=%d, buf=%s\n", strlen(ctx->rd_name), ctx->rd_name);
    if (-1 == nl_send(ipc, ctx->rd_name, strlen(ctx->rd_name))) {
        loge("nl_send failed!\n");
        return -1;
    }
    thread_create(connecting_thread, ipc);
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
    logd("nl_accept\n");
    if (-1 == sem_timedwait(&ctx->sem, &abs_time)) {
        loge("accept failed %d: %s\n", errno, strerror(errno));
        return -1;
    }
    ctx->connected = 1;
    return 0;
}

static int nl_connect(struct ipc *ipc, const char *name)
{
    struct nl_ctx *ctx = (struct nl_ctx *)ipc->ctx;
    logd("nl_send len=%d, buf=%s\n", strlen(ctx->rd_name), ctx->rd_name);
    nl_send(ipc, ctx->rd_name, strlen(ctx->rd_name));
    thread_create(connecting_thread, ipc);
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
        loge("connect %s failed %d: %s\n", name, errno, strerror(errno));
        return -1;
    }
    ctx->connected = 1;
    return 0;
}


static int nl_set_recv_cb(struct ipc *ipc, ipc_recv_cb *cb)
{
    _nl_recv_cb = cb;
    return 0;
}

static void nl_deinit(struct ipc *ipc)
{
    if (!ipc) {
        return;
    }
    struct nl_ctx *ctx = (struct nl_ctx *)ipc->ctx;
    close(ctx->fd);
    sem_destroy(&ctx->sem);
    if (_nl_recv_buf) {
        free(_nl_recv_buf);
    }
    free(ctx);
}

struct ipc_ops nlk_ops = {
    .init             = nl_init,
    .deinit           = nl_deinit,
    .accept           = nl_accept,
    .connect          = nl_connect,
    .register_recv_cb = nl_set_recv_cb,
    .send             = nl_send,
    .recv             = nl_recv,
};
