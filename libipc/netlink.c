/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    netlink.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-10 18:36
 * updated: 2015-11-10 18:36
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
#include <libgzf.h>
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
    struct gevent_base *evbase;
};

static char *_nl_recv_buf = NULL;
static ipc_recv_cb *_nl_recv_cb = NULL;

static int nl_recv(struct ipc *ipc, void *buf, size_t len);

static void *nl_init(const char *name, enum ipc_role role)
{
    struct sockaddr_nl saddr;
    int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_IPC_PORT);
    memset(&saddr, 0, sizeof(saddr));
    uint32_t pid = getpid();
    saddr.nl_family = AF_NETLINK;
    saddr.nl_pid = pid;
    saddr.nl_groups = NETLINK_IPC_GROUP;
    saddr.nl_pad = 0;
    if (0 > bind(fd, (struct sockaddr *)&saddr, sizeof(saddr))) {
        loge("bind failed: %d:%s\n", errno, strerror(errno));
        return NULL;
    }
    struct nl_ctx *ctx = CALLOC(1, struct nl_ctx);
    if (!ctx) {
        loge("malloc failed!\n");
        return NULL;
    }
    ctx->fd = fd;
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
    _nl_recv_buf = calloc(1, MAX_IPC_MESSAGE_SIZE);
    if (!_nl_recv_buf) {
        loge("malloc failed!\n");
        free(ctx);
        return NULL;
    }
    return ctx;
}


static int nl_send(struct ipc *ipc, const void *buf, size_t len)
{
    struct nl_ctx *ctx = ipc->ctx;
    struct sockaddr_nl daddr;
    struct msghdr msg;
    struct nlmsghdr *nlhdr = NULL;
    struct iovec iov;

    memset(&daddr, 0, sizeof(daddr));
    daddr.nl_family = AF_NETLINK;
    daddr.nl_pid = 0;
    daddr.nl_groups = 0;
    //daddr.nl_groups = NETLINK_IPC_GROUP_CLIENT;
    daddr.nl_pad = 0;

    nlhdr = (struct nlmsghdr *)CALLOC(len, struct nlmsghdr);
    nlhdr->nlmsg_pid = getpid();
    nlhdr->nlmsg_len = NLMSG_LENGTH(len);
    nlhdr->nlmsg_flags = 0;
    memcpy(NLMSG_DATA(nlhdr), buf, len);

    memset(&msg, 0, sizeof(struct msghdr));
    iov.iov_base = (void *)nlhdr;
    iov.iov_len = nlhdr->nlmsg_len;
    msg.msg_name = (void *)&daddr;
    msg.msg_namelen = sizeof(daddr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    logi("nlmsg_pid=%d, nlmsg_len=%d\n", nlhdr->nlmsg_pid, nlhdr->nlmsg_len);
    logi("sendmsg len=%d\n", sizeof(msg))
    sendmsg(ctx->fd, &msg, 0);
    free(nlhdr);
    return 0;
}

static int nl_recv(struct ipc *ipc, void *buf, size_t len)
{
    struct nl_ctx *ctx = ipc->ctx;
    struct sockaddr_nl sa;
    struct nlmsghdr *nlhdr = NULL;
    struct msghdr msg;
    struct iovec iov;
    int ret;
    char nlhdr_buf[MAX_IPC_MESSAGE_SIZE];

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
    return ret;
}

static int nl_accept(struct ipc *ipc)
{
    return 0;
}


static void on_conn_resp(int fd, void *arg)
{
    struct ipc *ipc = (struct ipc *)arg;
    struct nl_ctx *ctx = ipc->ctx;
    int len;
    len = nl_recv(ipc, _nl_recv_buf, MAX_IPC_MESSAGE_SIZE);
    if (len == -1) {
        loge("nl_recv failed!\n");
        return;
    }
    logi("recv len=%d, buf=%s\n", len, _nl_recv_buf);
    if (_nl_recv_cb) {
        _nl_recv_cb(ipc, _nl_recv_buf, len);
    }
    if (strncmp(_nl_recv_buf, ctx->rd_name, len)) {
        loge("connect response check failed!\n");
    }
    sem_post(&ctx->sem);
}

static void on_error(int fd, void *arg)
{
    logi("error: %d\n", errno);
}

static void *connecting_thread(struct thread *thd, void *arg)
{
//wait connect response
    struct ipc *ipc = (struct ipc *)arg;
    struct nl_ctx *ctx = ipc->ctx;
    struct gevent *e = gevent_create(ctx->fd, on_conn_resp, NULL, on_error, ipc);
    if (-1 == gevent_add(ctx->evbase, e)) {
        loge("gevent_add failed!\n");
        return NULL;
    }
    gevent_base_loop(ctx->evbase);

    return NULL;
}

static int nl_connect(struct ipc *ipc, const char *name)
{
    struct nl_ctx *ctx = ipc->ctx;
    logi("nl_send len=%d, buf=%s\n", strlen(ctx->rd_name), ctx->rd_name);
    nl_send(ipc, ctx->rd_name, strlen(ctx->rd_name));
    thread_create("connecting", connecting_thread, ipc);
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
    struct nl_ctx *ctx = ipc->ctx;
    close(ctx->fd);
    sem_destroy(&ctx->sem);
    if (_nl_recv_buf) {
        free(_nl_recv_buf);
    }
    free(ctx);
}


const struct ipc_ops nlk_ops = {
    nl_init,
    nl_deinit,
    nl_accept,
    nl_connect,
    nl_set_recv_cb,
    nl_send,
    nl_recv,
};
