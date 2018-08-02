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
#include "libipc.h"
#include <libmacro.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <mqueue.h>
#include <semaphore.h>

/*
 * message queue IPC build flow:
 *
 *            client (test_libipc)           server (ipcd)
 * step.1                                    create /dev/mqueue/IPC_SERVER.5555
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

#define IPC_SERVER_NAME "/IPC_SERVER"
#define IPC_CLIENT_NAME "/IPC_CLIENT"


struct mq_posix_ctx {
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

static int _mq_notify_update(struct mq_posix_ctx *ctx, mq_notify_cb cb)
{
    struct sigevent sv;
    memset(&sv, 0, sizeof(sv));
    sv.sigev_notify = SIGEV_THREAD;
    sv.sigev_notify_function = cb;
    sv.sigev_notify_attributes = NULL;
    sv.sigev_value.sival_ptr = ctx;
    if (mq_notify(ctx->mq_rd, &sv) == -1) {//vagrind
        printf("mq_notify failed %d: %s\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

static int _mq_connect(struct ipc *ipc, const char *name)
{//for client
    struct mq_posix_ctx *ctx = (struct mq_posix_ctx *)ipc->ctx;
    struct mq_attr attr;
    ctx->parent = ipc;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MQ_MAXMSG;
    attr.mq_msgsize = MQ_MSGSIZE;
    attr.mq_curmsgs = 0;

    ctx->mq_wr = mq_open(name, O_WRONLY|O_EXCL, S_IRWXU|S_IRWXG, &attr);
    if (ctx->mq_wr < 0) {
        printf("mq_open %s failed: %d:%s\n", name, errno, strerror(errno));
        return -1;
    }
    strncpy(ctx->mq_wr_name, name, sizeof(ctx->mq_wr_name));
    if (0 > _mq_send(ipc, ctx->mq_rd_name, strlen(ctx->mq_rd_name))) {
        printf("_mq_send failed!\n");
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
        printf("connect %s failed %d: %s\n", name, errno, strerror(errno));
        return -1;
    }
    return 0;
}

static int _mq_accept(struct ipc *ipc)
{//for server
    struct mq_posix_ctx *ctx = (struct mq_posix_ctx *)ipc->ctx;
    ctx->parent = ipc;
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MQ_MAXMSG;
    attr.mq_msgsize = MQ_MSGSIZE;
    attr.mq_curmsgs = 0;

    sem_wait(&ctx->sem);

    ctx->mq_wr = mq_open(ctx->mq_wr_name, O_WRONLY, S_IRWXU | S_IRWXG, &attr);
    if (ctx->mq_wr < 0) {
        printf("mq_open %s failed: %d:%s\n", ctx->mq_wr_name, errno, strerror(errno));
        return -1;
    }
    if (0 > _mq_send(ipc, ctx->mq_wr_name, strlen(ctx->mq_wr_name))) {
        printf("_mq_send failed!\n");
        return -1;
    }
    return 0;
}

static void on_recv(union sigval sv)
{
    int len;
    struct mq_posix_ctx *ctx = (struct mq_posix_ctx *)sv.sival_ptr;
    struct ipc *ipc = (struct ipc *)ctx->parent;
    if (-1 == _mq_notify_update(ctx, on_recv)) {
        printf("_mq_notify_update failed!\n");
        return;
    }
    //recv len must greater than mq_msgsize;
    len = _mq_recv(ipc, _mq_recv_buf, MAX_IPC_MESSAGE_SIZE);
    if (len == -1) {
        printf("_mq_recv failed!\n");
        return;
    }
    if (_mq_recv_cb) {
        _mq_recv_cb(ipc, _mq_recv_buf, len);
    } else {
        printf("_mq_recv_cb is NULL!\n");
    }
}

static void on_connect(union sigval sv)
{
    char buf[MAX_IPC_MESSAGE_SIZE];
    int len;
    struct mq_posix_ctx *ctx = (struct mq_posix_ctx *)sv.sival_ptr;
    struct ipc *ipc = (struct ipc *)ctx->parent;
    if (-1 == _mq_notify_update(ctx, on_recv)) {
        printf("_mq_notify_update failed!\n");
        return;
    }
    memset(&buf, 0, sizeof(buf));
    len = _mq_recv(ipc, buf, sizeof(buf));
    if (len <= 0) {
        printf("_mq_recv failed!\n");
        return;
    }
    if (ctx->role == IPC_SERVER) {
        strncpy(ctx->mq_wr_name, buf, sizeof(ctx->mq_wr_name));
    } else {//IPC_CLIENT
        if (strcmp(buf, ctx->mq_rd_name)) {
            printf("buf = %s\n", buf);
            printf("connect response check failed!\n");
            return;
        }
    }
    sem_post(&ctx->sem);
}

static void *_mq_init(struct ipc *ipc, uint16_t port, enum ipc_role role)
{
    struct mq_attr attr;
    struct mq_posix_ctx *ctx = CALLOC(1, struct mq_posix_ctx);
    if (!ctx) {
        printf("malloc failed!\n");
        return NULL;
    }
    ipc->ctx = ctx;
    ctx->parent = ipc;
    if (role == IPC_SERVER) {
        snprintf(ctx->mq_rd_name, sizeof(ctx->mq_rd_name), "%s.%d", IPC_SERVER_NAME, port);
    } else if (role == IPC_CLIENT) {
        snprintf(ctx->mq_rd_name, sizeof(ctx->mq_rd_name), "%s.%d", IPC_CLIENT_NAME, port);
        snprintf(ctx->mq_wr_name, sizeof(ctx->mq_wr_name), "%s.%d", IPC_SERVER_NAME, port);
    }
    attr.mq_flags = 0;
    attr.mq_maxmsg = MQ_MAXMSG;
    attr.mq_msgsize = MQ_MSGSIZE;
    attr.mq_curmsgs = 0;
    mq_unlink(ctx->mq_rd_name);
    ctx->mq_rd = mq_open(ctx->mq_rd_name, O_RDWR|O_EXCL|O_CREAT|O_NONBLOCK, S_IRWXU|S_IRWXG, &attr);
    if (ctx->mq_rd == -1) {
        printf("mq_open %s failed %d:%s\n", ctx->mq_rd_name, errno, strerror(errno));
        goto failed;
    }

    ctx->role = role;
    if (-1 == sem_init(&ctx->sem, 0, 0)) {
        printf("sem_init failed %d:%s\n", errno, strerror(errno));
        return NULL;
    }
    _mq_recv_buf = calloc(1, MAX_IPC_MESSAGE_SIZE);
    if (!_mq_recv_buf) {
        printf("malloc failed!\n");
        return NULL;
    }
    if (-1 == _mq_notify_update(ctx, on_connect)) {
        printf("_mq_notify_update failed!\n");
        return NULL;
    }
    if (role == IPC_CLIENT) {
        _mq_connect(ipc, ctx->mq_wr_name);
    } else if (role == IPC_SERVER) {
        _mq_accept(ipc);
    }
    return ctx;
failed:
    if (ctx->mq_rd) {
        mq_close(ctx->mq_rd);
    }
    if (ctx) {
        free(ctx);
    }
    return NULL;
}

static int _mq_set_recv_cb(struct ipc *ipc, ipc_recv_cb *cb)
{
    _mq_recv_cb = cb;
    return 0;
}

static void _mq_deinit(struct ipc *ipc)
{
    if (!ipc) {
        return;
    }
    struct mq_posix_ctx *ctx = (struct mq_posix_ctx *)ipc->ctx;
    mq_close(ctx->mq_rd);
    mq_close(ctx->mq_wr);
    mq_unlink(ctx->mq_rd_name);
    sem_destroy(&ctx->sem);
    if (_mq_recv_buf) {
        free(_mq_recv_buf);
    }
    free(ctx);
}

static int _mq_send(struct ipc *ipc, const void *buf, size_t len)
{
    struct mq_posix_ctx *ctx = (struct mq_posix_ctx *)ipc->ctx;
    if (0 != mq_send(ctx->mq_wr, (const char *)buf, len, MQ_MSG_PRIO)) {
        printf("mq_send failed %d: %s\n", errno, strerror(errno));
        return -1;
    }
    return len;
}

static int _mq_recv(struct ipc *ipc, void *buf, size_t len)
{
    struct mq_posix_ctx *ctx = (struct mq_posix_ctx *)ipc->ctx;
    ssize_t ret = mq_receive(ctx->mq_rd, (char *)buf, len, NULL);
    if (-1 == ret) {
        printf("mq_receive failed %d: %s\n", errno, strerror(errno));
        return -1;
    }
    return ret;
}

struct ipc_ops msgq_posix_ops = {
     .init             = _mq_init,
     .deinit           = _mq_deinit,
     .accept           = NULL,
     .connect          = NULL,
     .register_recv_cb = _mq_set_recv_cb,
     .send             = _mq_send,
     .recv             = _mq_recv,
};
