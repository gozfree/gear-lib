/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libipc.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-10 16:29:41
 * updated: 2015-11-10 16:29:41
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <liblog.h>
#include "libipc.h"

int ipc_packet_format(struct ipc *ipc, uint32_t func_id)
{
    struct ipc_header *h = &ipc->packet.header;
    h->func_id = func_id;
    return 0;
}


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
    ipc_packet_format(ipc, func_id);
    ipc_send(ipc, in_arg, in_len);
    //wait_response();
    return ipc_recv(ipc, out_arg, out_len);
}
