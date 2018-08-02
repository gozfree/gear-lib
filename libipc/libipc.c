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
#include <unistd.h>
#include <sys/time.h>

extern const struct ipc_ops msgq_posix_ops;
extern const struct ipc_ops msgq_sysv_ops;
extern const struct ipc_ops socket_ops;
extern const struct ipc_ops nlk_ops;
extern const struct ipc_ops shm_ops;

static ipc_handler_t message_map[MAX_MESSAGES_IN_MAP];
static int           message_map_registered;
static struct ipc_packet *_pkt_sbuf = NULL;
static void *_arg_buf = NULL;

typedef enum ipc_backend_type {
    IPC_BACKEND_MQ_POSIX,
    IPC_BACKEND_MQ_SYSV,
    IPC_BACKEND_SOCKET,
    IPC_BACKEND_NLK,
    IPC_BACKEND_SHM,
} ipc_backend_type;

static const struct ipc_ops *ipc_ops[] = {
    &msgq_posix_ops,
    &msgq_sysv_ops,
    &socket_ops,
    &nlk_ops,
    &shm_ops,
    NULL
};

static int pack_msg(struct ipc_packet *pkt, uint32_t func_id,
                    const void *in_arg, size_t in_len)
{
    struct ipc_header *hdr;
    struct timeval now;

    if (!pkt) {
        printf("invalid paraments!\n");
        return -1;
    }
    gettimeofday(&now, NULL);
    if (in_len > MAX_IPC_MESSAGE_SIZE - sizeof(ipc_header_t)) {
        printf("cmd arg too long %zu\n", in_len);
        return -1;
    }

    hdr = &(pkt->header);
    hdr->func_id = func_id;
    hdr->time_stamp = now.tv_sec * 1000000L + now.tv_usec;

    if (in_arg) {
        hdr->payload_len = in_len;
        memcpy(pkt->payload, in_arg, in_len);
    } else {
        hdr->payload_len = 0;
    }
    return 0;
}

static int unpack_msg(struct ipc_packet *pkt, uint32_t *func_id,
                      void *out_arg, size_t *out_len)
{
    struct ipc_header *hdr;
    if (!pkt || !out_arg || !out_len) {
        printf("invalid paraments!\n");
        return -1;
    }
    hdr = &(pkt->header);
    *func_id = hdr->func_id;
    if (!IS_IPC_MSG_VALID(*func_id)) {
        printf("func_id is invalid!\n");
        return -1;
    }
    *out_len = MIN2(hdr->payload_len, MAX_IPC_MESSAGE_SIZE);
    memcpy(out_arg, pkt->payload, *out_len);
    return 0;
}

static int push_async_cmd(struct ipc *ipc, uint32_t func_id)
{
    char cmd_str[11];//0xFFFFFFFF
    snprintf(cmd_str, sizeof(cmd_str), "0x%08X", func_id);
    if (-1 == dict_add(ipc->async_cmd_list, cmd_str, cmd_str)) {
        return -1;
    }
    return 0;
}

static int pop_async_cmd(struct ipc *ipc, uint32_t func_id)
{
    int ret = 0;
    char *pair = NULL;
    char cmd_str[11];//0xFFFFFFFF
    snprintf(cmd_str, sizeof(cmd_str), "0x%08X", func_id);
    pair = dict_get(ipc->async_cmd_list, cmd_str, cmd_str);
    if (pair) {
        if (-1 == dict_del(ipc->async_cmd_list, cmd_str)) {
            ret = -1;
        } else {
            ret = 0;
        }
    } else {
        ret = -1;
    }
    return ret;
}

int ipc_call(struct ipc *ipc, uint32_t func_id,
             const void *in_arg, size_t in_len,
             void *out_arg, size_t out_len)
{
    if (!ipc) {
        printf("invalid parament!\n");
        return -1;
    }
    if (-1 == pack_msg(_pkt_sbuf, func_id, in_arg, in_len)) {
        printf("pack_msg failed!\n");
        return -1;
    }
    if (-1 == ipc->ops->send(ipc, _pkt_sbuf, sizeof(ipc_packet_t) + in_len)) {
        printf("send msg failed!\n");
        return -1;
    }
    if (IS_IPC_MSG_NEED_RETURN(func_id)) {
        struct timeval now;
        struct timespec abs_time;
        uint32_t timeout = 2000;//msec
        gettimeofday(&now, NULL);
        now.tv_usec += (timeout % 1000) * 1000;
        now.tv_sec += timeout / 1000;
        if ( now.tv_usec >= 1000000 ) {
            now.tv_usec -= 1000000;
            now.tv_sec ++;
        }
        abs_time.tv_sec = now.tv_sec;
        abs_time.tv_nsec = now.tv_usec * 1000;
        if (-1 == push_async_cmd(ipc, func_id)) {
            printf("push_async_cmd failed!\n");
        }
        if (-1 == sem_timedwait(&ipc->sem, &abs_time)) {
            printf("response failed %d:%s\n", errno, strerror(errno));
            return -1;
        }
        memcpy(out_arg, ipc->resp_buf, out_len);
    } else {

    }
    return 0;
}

static int register_msg_proc(ipc_handler_t *handler)
{
    int i;
    int msg_id_registered  = 0;
    int msg_slot = -1;
    uint32_t func_id;

    if (!handler) {
        printf("Cannot register null msg proc \n");
        return -1;
    }

    func_id = handler->func_id;

    for (i=0; i < message_map_registered; i++) {
        if (message_map[i].func_id == func_id) {
            printf("overwrite existing msg proc for func_id %d \n", func_id);
            msg_id_registered = 1;
            msg_slot = i;
            break;
        }
    }

    if ((!msg_id_registered)) {
        if ((message_map_registered == MAX_MESSAGES_IN_MAP)) {
            printf("too many msg proc registered \n");
            return -1;
        }
        //increase the count
        msg_slot = message_map_registered;
        message_map_registered++;
    }

    //if the handler registered is NULL, then just fill NULL handler
    message_map[msg_slot] = *handler;
    return 0;
}

int ipc_register_map(ipc_handler_t *map, int num_entry)
{
    int i;
    int ret;

    if (map == NULL) {
        printf("register_msg_map: null map \n");
        return -1;
    }

    if ((num_entry <= 0) || (num_entry > MAX_MESSAGES_IN_MAP)) {
        printf("register_msg_map:invalid num_entry %d \n", num_entry);
        return -1;
    }

    for (i = 0; i < num_entry; i++) {
        ret = register_msg_proc(&map[i]);
        if (ret < 0) {
            printf("register_msg_map:register failed  at %d \n", i);
            return -1;
        }
    }

    return 0;
}

int find_ipc_handler(uint32_t func_id, ipc_handler_t *handler)
{
    int i;
    for (i = 0; i < message_map_registered; i++) {
        if (message_map[i].func_id == func_id) {
            if (handler)  {
                *handler = message_map[i];
            }
            return 0;
        }
    }
    return -1;
}

static int process_msg(struct ipc *ipc, void *buf, size_t len)
{
    ipc_handler_t handler;
    uint32_t func_id = 0;
    char out_arg[1024];
    size_t out_len;
    size_t ret_len;
    struct ipc_packet *pkt = (struct ipc_packet *)buf;
    if (-1 == unpack_msg(pkt, &func_id, &out_arg, &out_len)) {
        printf("unpack_msg failed!\n");
        return -1;
    }
    if (find_ipc_handler(func_id, &handler) == 0 ) {
        handler.cb(ipc, out_arg, out_len, _arg_buf, &ret_len);//direct call cb is not good, will be change to workq
    } else {
        printf("no callback for this MSG ID in process_msg\n");
    }
    if (ret_len > 0) {
        if (0 > pack_msg(pkt, func_id, _arg_buf, ret_len)) {
            printf("pack_msg failed!\n");
            return -1;
        }
        ipc->ops->send(ipc, pkt, sizeof(ipc_packet_t) + ret_len);
    }
    return 0;
}

static int on_return(struct ipc *ipc, void *buf, size_t len)
{
    uint32_t func_id;
    size_t out_len;
    struct ipc_packet *pkt = (struct ipc_packet *)buf;
    memset(ipc->resp_buf, 0, MAX_IPC_RESP_BUF_LEN);
    if (-1 == unpack_msg(pkt, &func_id, ipc->resp_buf, &out_len)) {
        printf("unpack_msg failed!\n");
        return -1;
    }
    if (-1 == pop_async_cmd(ipc, func_id)) {
        printf("msg received is not the response of cmd_id!\n");
        return -1;
    }

    ipc->resp_len = out_len;
    sem_post(&ipc->sem);
    return 0;
}

struct ipc *ipc_create(enum ipc_role role, uint16_t port)
{
    struct ipc *ipc = CALLOC(1, struct ipc);
    if (!ipc) {
        printf("malloc failed!\n");
        return NULL;
    }
    ipc->role = role;
    ipc->ops = ipc_ops[IPC_BACKEND_MQ_POSIX];
    ipc->ctx = ipc->ops->init(ipc, port, ipc->role);
    if (!ipc->ctx) {
        printf("init failed!\n");
        goto failed;
    }
    if (ipc->role == IPC_SERVER) {
        _arg_buf = (struct ipc_packet *)calloc(1, MAX_IPC_MESSAGE_SIZE);
        ipc->ops->register_recv_cb(ipc, process_msg);
    } else if (ipc->role == IPC_CLIENT) {
        ipc->async_cmd_list = dict_new();
        if (!ipc->async_cmd_list) {
            printf("create async_cmd_list failed!\n");
            goto failed;
        }
        ipc->resp_buf = calloc(1, MAX_IPC_RESP_BUF_LEN);
        _pkt_sbuf = (struct ipc_packet *)calloc(1, MAX_IPC_MESSAGE_SIZE);
        ipc->ops->register_recv_cb(ipc, on_return);
        sem_init(&ipc->sem, 0, 0);
    }
    return ipc;
failed:
    if (ipc) {
        free(ipc);
    }
    return NULL;
}

void ipc_destroy(struct ipc *ipc)
{
    if (!ipc) {
        return;
    }
    sem_destroy(&ipc->sem);
    ipc->ops->deinit(ipc);

    if (_arg_buf) free(_arg_buf);
    if (_pkt_sbuf) free(_pkt_sbuf);
    if (ipc->resp_buf) free(ipc->resp_buf);
    if (ipc->async_cmd_list) dict_free(ipc->async_cmd_list);
    free(ipc);
}
