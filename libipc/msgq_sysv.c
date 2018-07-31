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
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <sys/prctl.h>
#include <pthread.h>

struct mq_ctx {
    int msgid_c;
    int msgid_s;
    ipc_role role;
    pthread_t tid;
    pid_t pid;
    bool run;
    void *parent;
};

static ipc_recv_cb *_mq_recv_cb = NULL;

typedef struct {
    int mcode;
    int size;
    char buf[1];
} qmb_msg_t;

#define QMB_CMD_BIND        (-1)
#define QMB_CMD_UNBIND      (-2)
#define MSGQ_KEY  12345678    //!< FIXME:

enum {
    MSGTYPE_REQUEST = 1,    //!< Must be > 0
    MSGTYPE_RESPOND,
    MSGTYPE_NOTIFY
};

static int msgSend(int msgid, int code, const void *buf, int size)
{
    int ret=0;
    qmb_msg_t *msg;

    msg = (qmb_msg_t *)malloc(sizeof(qmb_msg_t)-1 + size);
    if (msg == NULL) {
        printf("malloc qmb_msg_t failed\n");
        return -1;
    }
    msg->mcode = code;
    if (size > 0) {
        memcpy((void *)msg->buf, buf, size);
    }

    printf("%s:%d msgid=%d, code=%d, size=%d\n", __func__, __LINE__, msgid, code, size);
    if (0 != (ret = msgsnd(msgid, (const void *)msg, sizeof(qmb_msg_t) -1+ size, 0))) {
        printf("msgsnd failed, errno %d\n", errno);
        size = -1;
        goto failed;
    }
failed:
    free((void *)msg);
    return size;
}

static int msgRecv(int msgid, int *code, void *buf, int len)
{
    int size;
    qmb_msg_t *msg;

    msg = (qmb_msg_t *)malloc(sizeof(qmb_msg_t)-1 + len);
    if (msg == NULL) {
        printf("malloc qmb_msg_t failed\n");
        return -1;
    }

    if (-1 == (size = msgrcv(msgid, (void *)msg, len + sizeof(qmb_msg_t)-1, 0, 0))) {
        printf("msgrcv failed, errno %d\n", errno);
        goto Quit;
    }
    *code = msg->mcode;
    size -= sizeof(msg);
    if (size > 0) {
        memcpy(buf, msg->buf, size);
    }

Quit:
    free((void *)msg);
    return size;
}

static void *server_thread(void *arg)
{
    int mcode;
    char buf[1024];
    int size;
    struct mq_ctx *c = (struct mq_ctx *)arg;
    struct ipc *ipc = (struct ipc *)c->parent;
    while (c->run) {
        memset(buf, 0, sizeof(buf));
        size = msgRecv(c->msgid_s, &mcode, buf, sizeof(buf));
        if (size < 0) {
            printf(("msgRecv failed\n"));
            break;
        }
        if (mcode == QMB_CMD_BIND) {
            printf("QMB_CMD_BIND(%d)\n", *(pid_t *)buf);

            c->pid = *(pid_t *)buf;
            c->msgid_c = msgget((key_t)c->pid, 0);
            if (c->msgid_c == -1) {
                printf("msgget to open client msgQ failed, errno %d\n", errno);
                continue;
            }

            if (-1 == msgSend(c->msgid_c, QMB_CMD_BIND,
                        (const void *)&c->pid, sizeof(pid_t))) {
                printf("msgSend(RESPOND, QMB_CMD_BIND) failed\n");
                continue;
            }
        } else if (mcode == QMB_CMD_UNBIND) {
            printf("QMB_CMD_UNBIND(%d)\n", *(pid_t *)buf);
            if(c->msgid_c == 0) {
                continue;
            }
            if (c->pid != *(pid_t *)buf) {
                continue;
            }
            c->pid = 0;
            c->msgid_c = 0;
        } else if (mcode >= 0) {
            printf(" xxx mcode = %d\n", mcode);
            process_msg(ipc, buf, size);
            //_mq_recv_cb(ipc, buf, size);
        } else {
            printf("invalid mcode %d\n", mcode);
        }
    }

    return NULL;
}

static void *_mq_init_client(struct ipc *ipc)
{
    struct mq_ctx *ctx = CALLOC(1, struct mq_ctx);
    if (!ctx) {
        printf("malloc failed!\n");
    }
    printf("ipc = %p, mq_ctx=%p\n", ipc, ctx);
    ipc->ctx = ctx;
    ctx->parent = ipc;
    ctx->role = IPC_CLIENT;
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
    if (-1 == msgSend(ctx->msgid_s, QMB_CMD_BIND, (const void *)&ctx->pid, sizeof(pid_t))) {
        printf("msgSend failed!\n");
    }

    int code = 0;
    pid_t pid = 0;
    if (-1 == msgRecv(ctx->msgid_c, &code, (void *)&pid, sizeof(pid_t))) {
        printf("msgRecv failed!\n");
    }

    return ctx;

failed:
    printf("init client failed!\n");
    if (ctx->msgid_c != -1) {
        msgctl(ctx->msgid_c, IPC_RMID, NULL);
    }
    free(ctx);
    return NULL;
}

static void _mq_deinit_client(struct mq_ctx *ctx)
{
    if (0 != msgSend(ctx->msgid_s, QMB_CMD_UNBIND, (const void *)&ctx->pid, sizeof(pid_t))) {
    }
    msgctl(ctx->msgid_c, IPC_RMID, NULL);
    free(ctx);
}

static void *_mq_init_server(struct ipc *ipc)
{
    struct mq_ctx *ctx = CALLOC(1, struct mq_ctx);
    if (!ctx) {
        printf("malloc failed!\n");
    }
    ipc->ctx = ctx;
    ctx->parent = ipc;
    ctx->role = IPC_SERVER;
    ctx->msgid_s = msgget(MSGQ_KEY, IPC_CREAT|IPC_EXCL|S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    if (ctx->msgid_s == -1) {
        printf("msgget failed: %d\n", errno);
        goto failed;
    }
    ipc->fd = ctx->msgid_s;
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

static void _mq_deinit_server(struct mq_ctx *ctx)
{
    ctx->run = false;
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
        printf("xxxxxxxxxxxx\n");
        return NULL;
    }
}

static void _mq_deinit(struct ipc *ipc)
{
    if (!ipc) {
        return;
    }
    struct mq_ctx *ctx = (struct mq_ctx *)ipc->ctx;
    if (ctx->role == IPC_SERVER) {
        _mq_deinit_server(ctx);
    } else if (ctx->role == IPC_CLIENT) {
        _mq_deinit_client(ctx);
    }
}

static int _mq_send(struct ipc *ipc, const void *buf, size_t len)
{
    struct mq_ctx *c = (struct mq_ctx *)ipc->ctx;
    int msgid = 0;
    if (ipc->role == IPC_SERVER) {
        msgid = c->msgid_c;
    } else if (ipc->role == IPC_CLIENT) {
        msgid = c->msgid_s;
    }
    int ret = msgSend(msgid, 1, buf, len);
    if (ret == -1) {
        printf("msgSend failed\n");
        return -1;
    }
    return len;
}

static int _mq_recv(struct ipc *ipc, void *buf, size_t len)
{
    struct mq_ctx *c = (struct mq_ctx *)ipc->ctx;
    int code;
    int msgid = 0;
    if (ipc->role == IPC_SERVER) {
        msgid = c->msgid_c;
    } else if (ipc->role == IPC_CLIENT) {
        msgid = c->msgid_s;
    }
    int ret = msgRecv(msgid, &code, buf, len);
    if (ret == -1) {
        printf("msgrcv failed\n");
    }

    return ret;
}

static int _mq_connect(struct ipc *ipc, const char *name)
{
    char buf[128];
    struct mq_ctx *c = (struct mq_ctx *)ipc->ctx;

    printf("%s:%d xxxx\n", __func__, __LINE__);
    if (-1 == _mq_send(ipc, (const void *)&c->pid, sizeof(pid_t))) {
        printf("_mq_send failed!\n");
        return -1;
    }
    if (-1 == _mq_recv(ipc, buf, sizeof(buf))) {
        printf("msgRecv failed!\n");
    }
    return 0;
}

static int _mq_accept(struct ipc *ipc)
{
    char buf[128];
    printf("%s:%d xxxx\n", __func__, __LINE__);
    _mq_recv(ipc, buf, sizeof(buf));
    printf("%s:%d xxxx\n", __func__, __LINE__);
    return 0;
}

static int _mq_set_recv_cb(struct ipc *ipc, ipc_recv_cb *cb)
{
    _mq_recv_cb = cb;
    return 0;
}

struct ipc_ops msgq_sysv_ops = {
     .init             = _mq_init,
     .deinit           = _mq_deinit,
     .accept           = _mq_accept,
     .connect          = _mq_connect,
     .register_recv_cb = _mq_set_recv_cb,
     .send             = _mq_send,
     .recv             = _mq_recv,
};
