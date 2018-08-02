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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <pthread.h>

#define MQ_CMD_BIND         (-1)
#define MQ_CMD_UNBIND       (-2)
#define MQ_CMD_QUIT         (-3)
#define MSGQ_KEY            12345678

struct mq_sysv_ctx {
    int msgid_c;
    int msgid_s;
    pthread_t tid;
    pid_t pid;
    bool run;
    void *parent;
};

static ipc_recv_cb *_mq_recv_cb = NULL;

typedef struct {
    int cmd;
    char buf[1];
} mq_msg_t;

static int msg_send(int msgid, int code, const void *buf, int size)
{
    mq_msg_t *msg = (mq_msg_t *)calloc(1, sizeof(mq_msg_t)-1+size);
    if (msg == NULL) {
        printf("malloc mq_msg_t failed\n");
        return -1;
    }
    msg->cmd = code;
    if (size > 0) {
        memcpy((void *)msg->buf, buf, size);
    }
    if (0 != msgsnd(msgid, (const void *)msg, sizeof(mq_msg_t)-1+size, 0)) {
        printf("msgsnd failed, errno %d\n", errno);
        size = -1;
    }
    free((void *)msg);
    return size;
}

static int msg_recv(int msgid, int *code, void *buf, int len)
{
    int size;
    mq_msg_t *msg = (mq_msg_t *)calloc(1, sizeof(mq_msg_t)-1+len);
    if (msg == NULL) {
        printf("malloc mq_msg_t failed\n");
        return -1;
    }

    if (-1 == (size = msgrcv(msgid, (void *)msg, len+sizeof(mq_msg_t)-1, 0, 0))) {
        printf("msgrcv failed, errno %d\n", errno);
        goto end;
    }
    *code = msg->cmd;
    size -= sizeof(msg);
    if (size > 0) {
        memcpy(buf, msg->buf, size);
    }
end:
    free((void *)msg);
    return size;
}

static void *server_thread(void *arg)
{
    int cmd;
    char buf[1024];
    int size;
    struct mq_sysv_ctx *c = (struct mq_sysv_ctx *)arg;
    struct ipc *ipc = (struct ipc *)c->parent;
    while (c->run) {
        memset(buf, 0, sizeof(buf));
        size = msg_recv(c->msgid_s, &cmd, buf, sizeof(buf));
        if (size < 0) {
            printf(("msg_recv failed\n"));
            continue;
        }
        switch (cmd) {
        case MQ_CMD_BIND:
            c->pid = *(pid_t *)buf;
            c->msgid_c = msgget((key_t)c->pid, 0);
            if (c->msgid_c == -1) {
                printf("msgget to open client msgQ failed, errno %d\n", errno);
                continue;
            }
            if (-1 == msg_send(c->msgid_c, MQ_CMD_BIND,
                        (const void *)&c->pid, sizeof(pid_t))) {
                printf("msg_send(MQ_CMD_BIND) failed\n");
                continue;
            }
            break;
        case MQ_CMD_UNBIND:
            if(c->msgid_c == 0) {
                continue;
            }
            if (c->pid != *(pid_t *)buf) {
                continue;
            }
            c->pid = 0;
            c->msgid_c = 0;
            break;
        case MQ_CMD_QUIT:
            break;
        default:
            if (_mq_recv_cb) {
                _mq_recv_cb(ipc, buf, size);
            } else {
                printf("_mq_recv_cb is NULL!\n");
            }
            break;
        }
    }
    return NULL;
}

static void *client_thread(void *arg)
{
    char buf[1024];
    int size;
    int cmd;
    struct mq_sysv_ctx *c = (struct mq_sysv_ctx *)arg;
    struct ipc *ipc = (struct ipc *)c->parent;
    while (c->run) {
        memset(buf, 0, sizeof(buf));
        size = msg_recv(c->msgid_c, &cmd, buf, sizeof(buf));
        if (size == -1) {
            printf("msg_recv failed!\n");
            continue;
        }
        switch (cmd) {
        case MQ_CMD_QUIT:
        case MQ_CMD_BIND:
        case MQ_CMD_UNBIND:
            break;
        default:
            if (_mq_recv_cb) {
                _mq_recv_cb(ipc, buf, size);
            } else {
                printf("_mq_recv_cb is NULL!\n");
            }
            break;
        }
    }
    return NULL;
}

static void *_mq_init_client(struct ipc *ipc)
{
    struct mq_sysv_ctx *ctx = CALLOC(1, struct mq_sysv_ctx);
    if (!ctx) {
        printf("malloc failed!\n");
        return NULL;
    }
    ipc->ctx = ctx;
    ctx->parent = ipc;
    ctx->pid = getpid();
    ctx->msgid_s = msgget(MSGQ_KEY, 0);
    if (ctx->msgid_s == -1) {
        printf("ipc server not run!\n");
        goto failed;
    }
    ctx->msgid_c = msgget((key_t)ctx->pid, IPC_CREAT|IPC_EXCL|S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    if (ctx->msgid_c == -1) {
        printf("msgget failed!\n");
        goto failed;
    }
    if (-1 == msg_send(ctx->msgid_s, MQ_CMD_BIND, (const void *)&ctx->pid, sizeof(pid_t))) {
        printf("msg_send failed!\n");
    }

    int code = 0;
    pid_t pid = 0;
    if (-1 == msg_recv(ctx->msgid_c, &code, (void *)&pid, sizeof(pid_t))) {
        printf("msg_recv failed!\n");
    }
    ctx->run = true;
    if (0 != pthread_create(&ctx->tid, NULL, client_thread, ctx)) {
        printf("pthread_create failed!\n");
        goto failed;
    }

    return ctx;

failed:
    if (ctx->msgid_c != -1) {
        msgctl(ctx->msgid_c, IPC_RMID, NULL);
    }
    free(ctx);
    return NULL;
}

static void _mq_deinit_client(struct mq_sysv_ctx *ctx)
{
    ctx->run = false;
    msg_send(ctx->msgid_c, MQ_CMD_QUIT, "a", 1);
    pthread_join(ctx->tid, NULL);
    msg_send(ctx->msgid_s, MQ_CMD_UNBIND, (const void *)&ctx->pid, sizeof(pid_t));
    msgctl(ctx->msgid_c, IPC_RMID, NULL);
    free(ctx);
}

static void *_mq_init_server(struct ipc *ipc)
{
    struct mq_sysv_ctx *ctx = CALLOC(1, struct mq_sysv_ctx);
    if (!ctx) {
        printf("malloc failed!\n");
        return NULL;
    }
    ipc->ctx = ctx;
    ctx->parent = ipc;
    ctx->msgid_s = msgget(MSGQ_KEY, IPC_CREAT|IPC_EXCL|S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    if (ctx->msgid_s == -1) {
        printf("msgget failed: %d\n", errno);
        goto failed;
    }
    ctx->run = true;
    if (0 != pthread_create(&ctx->tid, NULL, server_thread, ctx)) {
        printf("pthread_create failed!\n");
        goto failed;
    }
    return ctx;

failed:
    printf("init server failed!\n");
    if (ctx->msgid_s != -1) {
        msgctl(ctx->msgid_s, IPC_RMID, NULL);
    }
    free(ctx);
    return NULL;
}

static void _mq_deinit_server(struct mq_sysv_ctx *ctx)
{
    ctx->run = false;
    msg_send(ctx->msgid_s, MQ_CMD_QUIT, "a", 1);
    pthread_join(ctx->tid, NULL);
    msgctl(ctx->msgid_s, IPC_RMID, NULL);
    free(ctx);
}

static void *_mq_init(struct ipc *ipc, uint16_t port, enum ipc_role role)
{
    if (role == IPC_SERVER) {
        return _mq_init_server(ipc);
    } else if (role == IPC_CLIENT) {
        return _mq_init_client(ipc);
    } else {
        return NULL;
    }
}

static void _mq_deinit(struct ipc *ipc)
{
    if (!ipc) {
        return;
    }
    struct mq_sysv_ctx *ctx = (struct mq_sysv_ctx *)ipc->ctx;
    if (ipc->role == IPC_SERVER) {
        _mq_deinit_server(ctx);
    } else if (ipc->role == IPC_CLIENT) {
        _mq_deinit_client(ctx);
    }
}

static int _mq_send(struct ipc *ipc, const void *buf, size_t len)
{
    struct mq_sysv_ctx *c = (struct mq_sysv_ctx *)ipc->ctx;
    int msgid = 0;
    if (ipc->role == IPC_SERVER) {
        msgid = c->msgid_c;
    } else if (ipc->role == IPC_CLIENT) {
        msgid = c->msgid_s;
    }
    int ret = msg_send(msgid, 1, buf, len);
    if (ret == -1) {
        printf("msg_send failed\n");
        return -1;
    }
    return len;
}

static int _mq_recv(struct ipc *ipc, void *buf, size_t len)
{
    struct mq_sysv_ctx *c = (struct mq_sysv_ctx *)ipc->ctx;
    int code;
    int msgid = 0;
    if (ipc->role == IPC_SERVER) {
        msgid = c->msgid_c;
    } else if (ipc->role == IPC_CLIENT) {
        msgid = c->msgid_s;
    }
    int ret = msg_recv(msgid, &code, buf, len);
    if (ret == -1) {
        printf("msgrcv failed\n");
    }
    return ret;
}

static int _mq_set_recv_cb(struct ipc *ipc, ipc_recv_cb *cb)
{
    _mq_recv_cb = cb;
    return 0;
}

struct ipc_ops msgq_sysv_ops = {
     .init             = _mq_init,
     .deinit           = _mq_deinit,
     .accept           = NULL,
     .connect          = NULL,
     .register_recv_cb = _mq_set_recv_cb,
     .send             = _mq_send,
     .recv             = _mq_recv,
};
