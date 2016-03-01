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


static void *nl_init(const char *name, enum ipc_role role)
{
    struct sockaddr_nl saddr;
    int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_IPC_PORT);
    memset(&saddr, 0, sizeof(saddr));
    uint32_t pid = getpid();
    saddr.nl_family = AF_NETLINK;
    saddr.nl_pid = pid;
    saddr.nl_groups = 0;
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
        return NULL;
    }
    ctx->evbase = gevent_base_create();
    return ctx;
}


static int nl_recv(struct ipc *ipc, void *buf, size_t len);
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

    printf("nlmsg_pid=%d, nlmsg_len=%d", nlhdr->nlmsg_pid, nlhdr->nlmsg_len);
    //print_buffer(buf, len);
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

    nlhdr = (struct nlmsghdr *)buf;
    iov.iov_base = (void *)nlhdr;
    iov.iov_len = len;

    memset(&sa, 0, sizeof(sa));
    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (void *)&(sa);
    msg.msg_namelen = sizeof(sa);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    loge("nl_recv fd=%d\n", ctx->fd);
    return recvmsg(ctx->fd, &msg, 0);
}

static int nl_accept(struct ipc *ipc)
{
    return 0;
}

static void *wait_connect(struct thread *thd, void *arg)
{
//wait connect response

    return NULL;
}

static int nl_connect(struct ipc *ipc, const char *name)
{
    struct nl_ctx *ctx = ipc->ctx;
    nl_send(ipc, ctx->rd_name, strlen(ctx->rd_name));
    thread_create("netlink_connecting", wait_connect, ipc);
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

static ipc_recv_cb *_nl_recv_cb = NULL;

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
