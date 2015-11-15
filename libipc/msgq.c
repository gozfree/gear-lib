/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    msgq.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-10 17:24
 * updated: 2015-11-10 17:24
 *****************************************************************************/

#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>
#include <libgzf.h>
#include <liblog.h>
#include "libipc.h"

#define MQ_MAXMSG       5
#define MQ_MSGSIZE      1024
#define MQ_MSG_PRIO     10
struct mq_ctx {
    mqd_t mq;

};

static void *_mq_init(enum ipc_role role)
{
    int oflag;
    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = MQ_MAXMSG;
    attr.mq_msgsize = MQ_MSGSIZE;
    attr.mq_curmsgs = 0;

    if (role == IPC_SENDER) {
        //send is  block send
        oflag = O_RDWR | O_CREAT | O_EXCL;
    } else {
        //receive is nonblock receive
        oflag = O_RDWR | O_CREAT | O_EXCL | O_NONBLOCK;
    }
    struct mq_ctx *mc = CALLOC(1, struct mq_ctx);
    mqd_t mq = mq_open("/tmp/ipc", oflag , S_IRWXU | S_IRWXG, &attr);
    if (mq < 0) {
        loge("mq_open failed: %d:%s\n", errno, strerror(errno));
        return NULL;
    }
    mc->mq = mq;
    return mc;
}

static void _mq_deinit(struct ipc *ipc)
{
    if (!ipc) {
        return;
    }
    struct mq_ctx *ctx = ipc->ctx;
    mq_close(ctx->mq);
}

static int _mq_send(struct ipc *ipc, const void *buf, size_t len)
{
    struct mq_ctx *ctx = ipc->ctx;
    return mq_send(ctx->mq, buf, len, MQ_MSG_PRIO);
}

static int _mq_recv(struct ipc *ipc, void *buf, size_t len)
{
    struct mq_ctx *ctx = ipc->ctx;
    return mq_receive(ctx->mq, buf, len, NULL);
}


const struct ipc_ops msgq_ops = {
    _mq_init,
    _mq_deinit,
    _mq_send,
    _mq_recv,
};
