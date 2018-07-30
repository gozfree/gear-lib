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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <assert.h>
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
    long mtype;
    int mcode;
    char mtext[1];
} qmb_msg_t;

#define QMB_CMD_BIND        (-1)
#define QMB_CMD_UNBIND      (-2)
#define MSGQ_KEY  12345678    //!< FIXME:

enum {
    MSGTYPE_REQUEST = 1,    //!< Must be > 0
    MSGTYPE_RESPOND,
    MSGTYPE_NOTIFY
};

static int msgSend(int msgid, int type, int code, const void *buf, int size)
{
    int ret=0;
    qmb_msg_t *msg;

    printf("msgSend(%d, %d, %d, %p, %d)\n", msgid, type, code, buf, size);
    assert(size >= 0);

    /*!< Fill message body */
    msg = (qmb_msg_t *)malloc(sizeof(qmb_msg_t) - 1/*mtext[1]*/ + size);
    if (msg == NULL) {
        printf("malloc qmb_msg_t failed\n");
        return -1;
    }
    msg->mtype = type;
    msg->mcode = code;
    if (size > 0) {
        assert(buf);
        memcpy((void *)msg->mtext, buf, size);
    }

    /*!< Send message */
    if (msgsnd(msgid, (const void *)msg, sizeof(int)/*mcode*/ + size, 0/*msgflg*/) != 0) {
        printf("msgsnd() failed, errno %d\n", errno);
        ret = -1;
        goto Quit;
    }
    printf("msgsnd(%d) OK\n", code);

Quit:
    free((void *)msg);
    return ret;
}

#if 0
static int msgRecv(int msgid, int *type, int *code, void *buf, int len)
{
    int size;
    qmb_msg_t *msg;

    printf("msgRecv(%d)\n", msgid);
    assert(type);
    assert(code);
    assert(buf);

    /*!< Allocate msg */
    msg = (qmb_msg_t *)malloc(sizeof(qmb_msg_t) -1 + len);
    if (msg == NULL) {
        printf("malloc qmb_msg_t failed\n");
        return -1;
    }

    /*!< Receive message */
    if ((size = msgrcv(msgid, (void *)msg, len + sizeof(int)/*mcode*/, 0/*msgtyp*/, 0/*msgflg*/)) == -1) {
        printf("msgrcv() failed, errno %d\n", errno);
        goto Quit;
    }
    *type = msg->mtype;
    *code = msg->mcode;
    size -= sizeof(int)/*mcode*/;
    if (size > 0) {
        memcpy(buf, (void *)msg->mtext, size);
    }
    printf("msgRecv: type %d, code %d, size %d\n", *type, *code, size);

Quit:
    free((void *)msg);
    return size;
}
#endif

#if 0
static void * serverMessageReceiver(void *arg)
{
    int mtype, mcode;
    char *buf;
    int size;

    struct mq_ctx *ctx = (struct mq_ctx *)arg;
    //struct ipc *ipc = (struct ipc *)ctx->parent;
    prctl(PR_SET_NAME, "serverMessageReceiver");

    buf = (char *)malloc(256);
    if (buf == NULL) {
        printf("malloc message receiver buffer failed\n");
        /*!< FIXME: */
        return NULL;
    }

    while (ctx->run) {
        printf("xxx\n");
        size = msgRecv(ctx->msgid_s, &mtype, &mcode, buf, 256);
        if (size < 0) {
            printf(("msgRecv failed\n"));
            //continue;
            break;
        }
        printf("Server received message: mtype %d, mcode %d, size %d\n", mtype, mcode, size);
        assert(mtype == MSGTYPE_REQUEST);

        if (mcode == QMB_CMD_BIND) {
            printf("QMB_CMD_BIND(%d)\n", *(pid_t *)buf);

            /*!< Open client msgQ */
            ctx->pid = *(pid_t *)buf;
            ctx->msgid_c = msgget((key_t)ctx->pid, 0);
            if (ctx->msgid_c == -1) {
                printf("msgget to open client msgQ failed, errno %d\n", errno);
                continue;
                //break; 
            }

            /*!< Responed bind request */
            if (msgSend(ctx->msgid_c, MSGTYPE_RESPOND, QMB_CMD_BIND,
                        (const void *)&ctx->pid, sizeof(pid_t)) != 0) {
                printf("msgSend(RESPOND, QMB_CMD_BIND) failed\n");
                //!< DON'T NEED destroy msgQ.
                continue;
                //break;
            }
        }
        else if (mcode == QMB_CMD_UNBIND) {
            printf("QMB_CMD_UNBIND(%d)\n", *(pid_t *)buf);
            if(ctx->msgid_c == 0) {
                continue;
            }
            if (ctx->pid != *(pid_t *)buf) {
                continue;
            }
            ctx->pid = 0;
            ctx->msgid_c = 0;
        }
        else if (mcode >= 0) {
            //_mq_recv_cb(ipc, buf, size);
        }
        else {
            printf("invalid mcode %d\n", mcode);
        }
    }

    printf("serverMessageReceiver()--\n");
    return NULL;
}
#endif

static void *_mq_init_client()
{
    struct mq_ctx *ctx = CALLOC(1, struct mq_ctx);
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

    return ctx;

failed:
    if (ctx->msgid_c != -1) {
        msgctl(ctx->msgid_c, IPC_RMID, NULL);
    }
    free(ctx);
    return NULL;
}

static void _mq_deinit_client(struct mq_ctx *ctx)
{
    if (0 != msgSend(ctx->msgid_s, MSGTYPE_REQUEST, QMB_CMD_UNBIND, (const void *)&ctx->pid, sizeof(pid_t))) {
    }
    msgctl(ctx->msgid_c, IPC_RMID, NULL);
    free(ctx);
}

static void *_mq_init_server()
{
    struct mq_ctx *ctx = CALLOC(1, struct mq_ctx);
    ctx->role = IPC_SERVER;
    ctx->msgid_s = msgget(MSGQ_KEY, IPC_CREAT|IPC_EXCL|S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    if (ctx->msgid_s == -1) {
        printf("msgget failed: %d\n", errno);
        goto failed;
    }
    ctx->run = true;
#if 0
    if (0 != pthread_create(&ctx->tid, NULL, serverMessageReceiver, ctx)) {
        printf("pthread_create failed!\n");
        goto failed;
    }
#endif
    return ctx;

failed:
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

static void *_mq_init(const char *name, enum ipc_role role)
{
    if (role == IPC_SERVER) {
        return _mq_init_server();
    } else if (role == IPC_CLIENT) {
        return _mq_init_client();
    } else {
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
    if (msgsnd(c->msgid_s, buf, len, IPC_NOWAIT) != 0) {
        printf("msgsnd failed, errno %d\n", errno);
        return -1;
    }

    return 0;
}

static int _mq_recv(struct ipc *ipc, void *buf, size_t len)
{
    struct mq_ctx *c = (struct mq_ctx *)ipc->ctx;
    if (msgrcv(c->msgid_s, buf, len, 0, 0) != 0) {
        printf("msgsnd failed, errno %d\n", errno);
        return -1;
    }

    return 0;
}
static int _mq_connect(struct ipc *ipc, const char *name)
{
    char buf[128];
    struct mq_ctx *c = (struct mq_ctx *)ipc->ctx;
    if (0 != _mq_send(ipc, (const void *)&c->pid, sizeof(pid_t))) {
        printf("msgSend failed!\n");
        return -1;
    }
    if (0 > _mq_recv(ipc, buf, sizeof(buf))) {
        printf("msgRecv failed!\n");
    }
    return 0;
}

static int _mq_accept(struct ipc *ipc)
{
    char buf[128];
    _mq_recv(ipc, buf, sizeof(buf));
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
