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
#include <gear-lib/libmacro.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>

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

static rpc_recv_cb *_mq_recv_cb = NULL;

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
    struct rpc *rpc = (struct rpc *)c->parent;
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
                _mq_recv_cb(rpc, buf, size);
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
    struct rpc *rpc = (struct rpc *)c->parent;
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
                _mq_recv_cb(rpc, buf, size);
            } else {
                printf("_mq_recv_cb is NULL!\n");
            }
            break;
        }
    }
    return NULL;
}

static void *_mq_init_client(struct rpc *rpc)
{
    struct mq_sysv_ctx *ctx = CALLOC(1, struct mq_sysv_ctx);
    if (!ctx) {
        printf("malloc failed!\n");
        return NULL;
    }
    rpc->ctx = ctx;
    ctx->parent = rpc;
    ctx->pid = getpid();
    ctx->msgid_s = msgget(MSGQ_KEY, 0);
    if (ctx->msgid_s == -1) {
        printf("rpc server not run!\n");
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

static void *_mq_init_server(struct rpc *rpc)
{
    struct mq_sysv_ctx *ctx = CALLOC(1, struct mq_sysv_ctx);
    if (!ctx) {
        printf("malloc failed!\n");
        return NULL;
    }
    rpc->ctx = ctx;
    ctx->parent = rpc;
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

static void *_mq_init(struct rpc *rpc, const char *host, uint16_t port, enum rpc_role role)
{
    if (role == RPC_SERVER) {
        return _mq_init_server(rpc);
    } else if (role == RPC_CLIENT) {
        return _mq_init_client(rpc);
    } else {
        return NULL;
    }
}

static void _mq_deinit(struct rpc *rpc)
{
    if (!rpc) {
        return;
    }
    struct mq_sysv_ctx *ctx = (struct mq_sysv_ctx *)rpc->ctx;
    if (rpc->role == RPC_SERVER) {
        _mq_deinit_server(ctx);
    } else if (rpc->role == RPC_CLIENT) {
        _mq_deinit_client(ctx);
    }
}

static int _mq_send(struct rpc *rpc, const void *buf, size_t len)
{
    struct mq_sysv_ctx *c = (struct mq_sysv_ctx *)rpc->ctx;
    int msgid = 0;
    if (rpc->role == RPC_SERVER) {
        msgid = c->msgid_c;
    } else if (rpc->role == RPC_CLIENT) {
        msgid = c->msgid_s;
    }
    int ret = msg_send(msgid, 1, buf, len);
    if (ret == -1) {
        printf("msg_send failed\n");
        return -1;
    }
    return len;
}

static int _mq_recv(struct rpc *rpc, void *buf, size_t len)
{
    struct mq_sysv_ctx *c = (struct mq_sysv_ctx *)rpc->ctx;
    int code;
    int msgid = 0;
    if (rpc->role == RPC_SERVER) {
        msgid = c->msgid_c;
    } else if (rpc->role == RPC_CLIENT) {
        msgid = c->msgid_s;
    }
    int ret = msg_recv(msgid, &code, buf, len);
    if (ret == -1) {
        printf("msgrcv failed\n");
    }
    return ret;
}

static int _mq_set_recv_cb(struct rpc *rpc, rpc_recv_cb *cb)
{
    _mq_recv_cb = cb;
    return 0;
}

struct rpc_ops msgq_sysv_ops = {
     .init             = _mq_init,
     .deinit           = _mq_deinit,
     .accept           = NULL,
     .connect          = NULL,
     .register_recv_cb = _mq_set_recv_cb,
     .send             = _mq_send,
     .recv             = _mq_recv,
};
