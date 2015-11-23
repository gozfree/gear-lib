/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libipc.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-10 16:29:41
 * updated: 2015-11-10 16:29:41
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <libgzf.h>
#include <liblog.h>
#include "libipc.h"

extern const struct ipc_ops msgq_ops;
extern const struct ipc_ops nlk_ops;
extern const struct ipc_ops shm_ops;

static ipc_handler_t message_map[MAX_MESSAGES_IN_MAP];
static int           message_map_registered;

static const struct ipc_ops *ipc_ops[] = {
    &msgq_ops,
    NULL
};

int ipc_packet_format(struct ipc *ipc, uint32_t func_id)
{
    struct ipc_header *h = &ipc->packet.header;
    h->func_id = func_id;
    return 0;
}

static int pack_msg(struct ipc_packet *pkt, uint32_t func_id,
                const void *in_arg, size_t in_len)
{
    struct ipc_header *hdr;
    struct timeval curtime;

    if (!pkt) {
        loge("invalid paraments!\n");
        return -1;
    }
    gettimeofday(&curtime, NULL);
    if (in_len > MAX_IPC_MESSAGE_SIZE - sizeof(ipc_header_t)) {
        loge("cmd arg too long %d \n", in_len);
        return -1;
    }

    hdr = &(pkt->header);

    hdr->func_id = func_id;
    hdr->time_stamp = curtime.tv_sec * 1000000L + curtime.tv_usec;
    hdr->len = sizeof(ipc_header_t);

    if (in_arg) {
        hdr->payload_len = in_len;
        memcpy(pkt->data, in_arg, in_len);
    } else {
        hdr->payload_len = 0;
    }
    return 0;
}

#if 0
static int unpack_msg(struct ipc_packet *pkt, uint32_t *func_id,
                void *out_arg, size_t *out_len)
{
    struct ipc_header *hdr;
    if (!pkt) {
        loge("invalid paraments!\n");
        return -1;
    }
    hdr = &(pkt->header);
    *func_id = hdr->func_id;
    if (out_arg) {
        *out_len = min(hdr->payload_len, MAX_IPC_MESSAGE_SIZE);
        memcpy(out_arg, hdr + hdr->len, *out_len);
    } else {
        *out_len = 0;
    }
    return 0;
}
#endif

int ipc_call_async(struct ipc *ipc, uint32_t func_id,
                const void *in_arg, size_t in_len)
{
    ipc_packet_format(ipc, func_id);
    return ipc_send(ipc, in_arg, in_len);
}


int ipc_call(struct ipc *ipc, uint32_t func_id,
                const void *in_arg, size_t in_len,
                void *out_arg, size_t out_len)
{
    struct ipc_packet pkt;
    memset(&pkt, 0, sizeof(pkt));
    if (0 > pack_msg(&pkt, func_id, in_arg, in_len)) {
        loge("pack_msg failed!\n");
        return -1;
    }
    ipc_send(ipc, &pkt, sizeof(pkt) + in_len);
    //wait_response();
    return ipc_recv(ipc, out_arg, out_len);
}

int register_msg_proc(ipc_handler_t *handler)
{
    int i;
    int msg_id_registered  = 0;
    int msg_slot = -1;
    uint32_t func_id;

    if (!handler) {
        loge("Cannot register null msg proc \n");
        return -1;
    }

    func_id = handler->func_id;

    for (i=0; i < message_map_registered; i++) {
        if (message_map[i].func_id == func_id) {
            loge("overwrite existing msg proc for func_id %d \n", func_id);
            msg_id_registered = 1;
            msg_slot = i;
            break;
        }
    }

    if ((!msg_id_registered)) {
        if ((message_map_registered == MAX_MESSAGES_IN_MAP)) {
            loge("too many msg proc registered \n");
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
        loge("register_msg_map: null map \n");
        return -1;
    }

    if ((num_entry <= 0) || (num_entry > MAX_MESSAGES_IN_MAP)) {
        loge("register_msg_map:invalid num_entry %d \n", num_entry);
        return -1;
    }

    for (i = 0; i < num_entry; i++) {
        ret = register_msg_proc(&map[i]);
        if (ret < 0) {
            loge("register_msg_map:register failed  at %d \n", i);
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

int process_msg(struct ipc *ipc, void *buf, size_t len)
{
    ipc_handler_t handler;
    uint32_t func_id = ipc->packet.header.func_id;
    logi("func_id = %d\n", func_id);
    if (find_ipc_handler(func_id, &handler) == 0 ) {
        handler.cb(ipc, buf, len, NULL, 0);//direct call cb is not good, will be change to workq
    } else {
        loge("no callback for this MSG ID in process_msg\n");
    }
    return 0;
}

static void on_recv(struct ipc *ipc, void *buf, size_t len)
{
    logi("recv buf\n");
    process_msg(ipc, buf, len);
}

struct ipc *ipc_create(enum ipc_role role)
{
    struct ipc *ipc = CALLOC(1, struct ipc);
    if (!ipc) {
        loge("malloc failed!\n");
        return NULL;
    }
    ipc->ops = ipc_ops[0];
    ipc->ctx = ipc->ops->init(role);
    if (role == IPC_RECVER) {
        ipc->ops->register_recv_cb(ipc, on_recv);
    }
    return ipc;
}


ssize_t ipc_send(struct ipc *ipc, const void *buf, size_t len)
{
    int ret, head_size;
    struct ipc_packet *pkt = &ipc->packet;
    pkt->header.len = len;
    pkt->data = (void *)buf;
    head_size = sizeof(ipc_header_t);
    ret = ipc->ops->send(ipc, (void *)&pkt->header, head_size);
    if (ret != head_size) {
        loge("send failed ret = %d, head_size = %d\n", ret, head_size);
        return -1;
    }
    ret = ipc->ops->send(ipc, buf, len);
    if (ret != len) {
        loge("send failed!\n");
        return -1;
    }
    return ret;
}

ssize_t ipc_recv(struct ipc *ipc, void *buf, size_t len)
{
    struct ipc_packet pkt;
    int ret, rlen;
    int head_size = sizeof(ipc_header_t);
    memset(&pkt, 0, sizeof(ipc_packet_t));
    pkt.data = buf;

    ret = ipc->ops->recv(ipc, (void *)&pkt.header, head_size);
    if (ret == 0) {
        loge("peer connect closed\n");
        return -1;
    } else if (ret != head_size) {
        loge("recv failed, head_size = %d, ret = %d\n", head_size, ret);
        return -1;
    }
    //strncpy(ipc->uuid_dst, pkt.header.uuid_dst, MAX_UUID_LEN);
    if (len < pkt.header.len) {
        loge("recv pkt.header.len = %d\n", pkt.header.len);
    }
    rlen = min(len, pkt.header.len);
    ret = ipc->ops->recv(ipc, buf, rlen);
    if (ret == 0) {
        loge("peer connect closed\n");
        return -1;
    } else if (ret != rlen) {
        loge("recv failed: rlen=%d, ret=%d\n", rlen, ret);
        return -1;
    }
    //dump_packet(&pkt);
    return ret;
}
