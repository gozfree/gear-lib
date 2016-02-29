/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    msgq.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-10 17:24
 * updated: 2015-11-10 17:24
 *****************************************************************************/
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <sys/time.h>
#include <mqueue.h>
#include <semaphore.h>
#include <libgzf.h>
#include <liblog.h>
#include "libipc.h"

/*
 * mq_xxx api is POSIX message queue,
 * seems better than SysV message queue?
 */

/*
 * message queue IPC build flow:
 *
 *            client (test_libipc)           server (ipcd)
 * step.1                                    create /dev/mqueue/IPC_SREVER.5555
 *                                           create thread to wait message
 * step.2 create /dev/mqueue/IPC_CLIENT.$pid
 *        send "/IPC_CLIENT.$pid", and wait
 * step.3                                    accept message and post sem,
 *                                           send "/IPC_CLIENT.$pid"
 * step.4 post sem and strcmp recv msg and sent msg, 
 *        message queue IPC is built
 */

#define MQ_MAXMSG       5
#define MQ_MSGSIZE      1024
#define MQ_MSG_PRIO     10

#define MAX_MQ_NAME     256

struct mq_ctx {
    mqd_t mq_wr;
    mqd_t mq_rd;
    char mq_wr_name[MAX_MQ_NAME];
    char mq_rd_name[MAX_MQ_NAME];
    ipc_role role;
    sem_t sem;
    void *parent;
};

typedef void (mq_notify_cb)(union sigval sv);
static ipc_recv_cb *_mq_recv_cb = NULL;
static void *_mq_recv_buf = NULL;
static int _mq_recv(struct ipc *ipc, void *buf, size_t len);
static int _mq_send(struct ipc *ipc, const void *buf, size_t len);

static int _mq_notify_update(struct mq_ctx *ctx, mq_notify_cb cb)
{
    struct sigevent sv;
    memset(&sv, 0, sizeof(sv));
    sv.sigev_notify = SIGEV_THREAD;
    sv.sigev_notify_function = cb;
    sv.sigev_notify_attributes = NULL;
    sv.sigev_value.sival_ptr = ctx;
    if (mq_notify(ctx->mq_rd, &sv) == -1) {//vagrind
        loge("mq_notify failed %d: %s\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

static void on_recv(union sigval sv)
{
    int len;
    struct mq_ctx *ctx = (struct mq_ctx *)sv.sival_ptr;
    struct ipc *ipc = ctx->parent;
    if (-1 == _mq_notify_update(ctx, on_recv)) {
        loge("_mq_notify_update failed!\n");
        return;
    }
    //recv len must greater than mq_msgsize;
    len = _mq_recv(ipc, _mq_recv_buf, MAX_IPC_MESSAGE_SIZE);
    if (_mq_recv_cb) {
        _mq_recv_cb(ipc, _mq_recv_buf, len);
    }
}

static void on_connect(union sigval sv)
{
    char buf[MAX_IPC_MESSAGE_SIZE];
    int len;
    struct mq_ctx *ctx = (struct mq_ctx *)sv.sival_ptr;
    struct ipc *ipc = ctx->parent;
    if (-1 == _mq_notify_update(ctx, on_recv)) {
        loge("_mq_notify_update failed!\n");
        return;
    }
    memset(&buf, 0, sizeof(buf));
    len = _mq_recv(ipc, buf, sizeof(buf));
    if (len <= 0) {
        loge("_mq_recv failed!\n");
        return;
    }
    if (ctx->role == IPC_SERVER) {
        strncpy(ctx->mq_wr_name, buf, sizeof(ctx->mq_wr_name));
    } else {//IPC_CLIENT
        if (strcmp(buf, ctx->mq_rd_name)) {
            loge("connect response check failed!\n");
            return;
        }
    }
    sem_post(&ctx->sem);
}

static void *_mq_init(const char *name, enum ipc_role role)
{
    struct mq_attr attr;
    struct mq_ctx *ctx = CALLOC(1, struct mq_ctx);
    if (!ctx) {
        loge("malloc failed!\n");
        return NULL;
    }
    attr.mq_flags = 0;
    attr.mq_maxmsg = MQ_MAXMSG;
    attr.mq_msgsize = MQ_MSGSIZE;
    attr.mq_curmsgs = 0;
    int rflag = O_RDWR | O_EXCL | O_CREAT | O_NONBLOCK;
    mq_unlink(name);
    ctx->mq_rd = mq_open(name, rflag, S_IRWXU | S_IRWXG, &attr);
    if (ctx->mq_rd < 0) {
        loge("mq_open %s failed %d:%s\n", name, errno, strerror(errno));
        return NULL;
    }

    ctx->role = role;
    strncpy(ctx->mq_rd_name, name, sizeof(ctx->mq_rd_name));
    if (-1 == sem_init(&ctx->sem, 0, 0)) {
        loge("sem_init failed %d:%s\n", errno, strerror(errno));
        return NULL;
    }
    _mq_recv_buf = calloc(1, MAX_IPC_MESSAGE_SIZE);
    if (!_mq_recv_buf) {
        loge("malloc failed!\n");
        return NULL;
    }
    if (-1 == _mq_notify_update(ctx, on_connect)) {
        loge("_mq_notify_update failed!\n");
        return NULL;
    }
    return ctx;
}


static int _mq_set_recv_cb(struct ipc *ipc, ipc_recv_cb *cb)
{
    _mq_recv_cb = cb;
    return 0;
}

static int _mq_accept(struct ipc *ipc)
{//for server
    struct mq_ctx *ctx = ipc->ctx;
    ctx->parent = ipc;
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MQ_MAXMSG;
    attr.mq_msgsize = MQ_MSGSIZE;
    attr.mq_curmsgs = 0;

    sem_wait(&ctx->sem);

    ctx->mq_wr = mq_open(ctx->mq_wr_name, O_WRONLY, S_IRWXU | S_IRWXG, &attr);
    if (ctx->mq_wr < 0) {
        loge("mq_open %s failed: %d:%s\n", ctx->mq_wr_name, errno, strerror(errno));
        return -1;
    }
    if (0 > _mq_send(ipc, ctx->mq_wr_name, strlen(ctx->mq_wr_name))) {
        loge("_mq_send failed!\n");
        return -1;
    }
    return 0;
}

static int _mq_connect(struct ipc *ipc, const char *name)
{//for client
    struct mq_ctx *ctx = ipc->ctx;
    struct mq_attr attr;
    ctx->parent = ipc;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MQ_MAXMSG;
    attr.mq_msgsize = MQ_MSGSIZE;
    attr.mq_curmsgs = 0;

    int wflag = O_WRONLY | O_EXCL;
    ctx->mq_wr = mq_open(name, wflag, S_IRWXU | S_IRWXG, &attr);
    if (ctx->mq_wr < 0) {
        loge("mq_open %s failed: %d:%s\n", name, errno, strerror(errno));
        return -1;
    }
    strncpy(ctx->mq_wr_name, name, sizeof(ctx->mq_wr_name));
    if (0 > _mq_send(ipc, ctx->mq_rd_name, strlen(ctx->mq_rd_name))) {
        loge("_mq_send failed!\n");
        return -1;
    }
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

static void _mq_deinit(struct ipc *ipc)
{
    if (!ipc) {
        return;
    }
    struct mq_ctx *ctx = ipc->ctx;
    mq_close(ctx->mq_rd);
    mq_close(ctx->mq_wr);
    mq_unlink(ctx->mq_rd_name);
    mq_unlink(ctx->mq_wr_name);
    sem_destroy(&ctx->sem);
    if (_mq_recv_buf) {
        free(_mq_recv_buf);
    }
    free(ctx);
}

static int _mq_send(struct ipc *ipc, const void *buf, size_t len)
{
    struct mq_ctx *ctx = ipc->ctx;
    if (0 != mq_send(ctx->mq_wr, buf, len, MQ_MSG_PRIO)) {
        loge("mq_send failed %d: %s\n", errno, strerror(errno));
        return -1;
    }
    return len;
}

static int _mq_recv(struct ipc *ipc, void *buf, size_t len)
{
    struct mq_ctx *ctx = ipc->ctx;
    ssize_t ret = mq_receive(ctx->mq_rd, buf, len, NULL);
    if (-1 == ret) {
        loge("mq_receive failed %d: %s\n", errno, strerror(errno));
        return -1;
    }
    return ret;
}

const struct ipc_ops msgq_ops = {
    _mq_init,
    _mq_deinit,
    _mq_accept,
    _mq_connect,
    _mq_set_recv_cb,
    _mq_send,
    _mq_recv,
};
