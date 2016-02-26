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
#include <libgzf.h>
#include <liblog.h>
#include "libipc.h"


struct nl_ctx {
    int fd;
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
    struct nl_ctx *nc = CALLOC(1, struct nl_ctx);
    if (!nc) {
        loge("malloc failed!\n");
        return NULL;
    }
    nc->fd = fd;
    return nc;
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

static int nl_set_recv_cb(struct ipc *ipc, ipc_recv_cb *cb)
{
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
