/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libp2p.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-05-20 00:36
 * updated: 2015-05-20 00:36
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <libmacro.h>
#include <liblog.h>
#include "libp2p.h"


static const char *_rpc_ip = "192.168.1.211";
//static char *_rpc_ip = "180.153.102.147";
//static char *_rpc_ip = "116.228.149.106";

static const char *_stun_ip = "192.168.1.211";
//static char *_stun_ip = "180.153.102.147";
//static char *_stun_ip = "116.228.149.106";
static void *input_thread(void *arg)
{
    struct p2p *p2p = (struct p2p *)arg;
    uint32_t uuid_dst;
    int ret, i;
    int len = 1024;
    char *buf = (char *)calloc(1, len);
    for (i = 0; i < len; i++) {
        buf[i] = i;
    }
    p2p_get_peer_list(p2p);
    printf("input peer id> ");
    scanf("%x", &uuid_dst);
    p2p_connect(p2p, uuid_dst);
    while (1) {
        memset(buf, 0, len);
        logi("input message> ");
        scanf("%s", buf);
        usleep(500*1000);
        //sprintf(buf, "%s", "abcd");
        ret = p2p_send(p2p, buf, strlen(buf));
        logi("p2p_send ret = %d, errno=%d\n", ret, errno);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t tid;
    struct p2p *p2p = p2p_init(_rpc_ip, _stun_ip);
    if (!p2p) {
        logi("p2p_init failed!\n");
        return -1;
    }
    logi("p2p id: %x\n", p2p->rpc->send_pkt.header.uuid_src);
    pthread_create(&tid, NULL, input_thread, p2p);
    p2p_dispatch(p2p);
    return 0;
}
