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
#include <linux/netlink.h>
#include <semaphore.h>
#include <libgzf.h>
#include <liblog.h>
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


struct nl_ctx {
    int fd;
    sem_t sem;
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
    if (-1 == sem_init(&ctx->sem, 0, 0)) {
        loge("sem_init failed %d:%s\n", errno, strerror(errno));
        return NULL;
    }
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
#if 1
    int ret;
    int recv_len = 1024;
    void *recv_buf = calloc(1, recv_len);
    if (!recv_buf) {
        loge("calloc failed!\n");
    }
    while (1) {
        ret = nl_recv(ipc, recv_buf, recv_len);
        if (ret <= 0) {
            loge("nl_recv failed: %d\n", ret);
        } else {
            break;
        }
    }
    loge("nl_recv ok: len=%d\n", ret);

#endif
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

static int nl_connect(struct ipc *ipc, const char *name)
{
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
