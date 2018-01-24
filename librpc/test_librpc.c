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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <libdict.h>
#include <liblog.h>
#include "librpc.h"
#include "librpc_stub.h"

#define MAX_UUID_LEN                (21)
static int on_get_connect_list_resp(struct rpc *r, void *arg, int len)
{
    char *ptr;
    int num = 0;
    len = r->recv_pkt.header.payload_len;
    logi("strlen = %d, %s\n", strlen((const char *)arg), arg);
    logi("on_get_connect_list, len = %d\n", r->recv_pkt.header.payload_len);
    num = len / MAX_UUID_LEN;
    printf("\n");
    for (ptr = (char *)arg; num > 0; --num) {
        logi("uuid list: %s\n", (char *)ptr);
        len = MAX_UUID_LEN;
        ptr += len;
    }
    return 0;
}

static int on_test_resp(struct rpc *r, void *arg, int len)
{
    logi("on_test_resp\n");
    return 0;
}

static int on_peer_post_msg_resp(struct rpc *r, void *arg, int len)
{
    //logi("on_peer_post_msg_resp len = %d\n", len);
    logi("msg from %x:\n%s\n", r->send_pkt.header.uuid_src, (char *)arg);
    return 0;
}

static int on_shell_help(struct rpc *r, void *arg, int len)
{
    printf("msg from %x:\n%s\n", r->send_pkt.header.uuid_src, (char *)arg);
    return 0;
}

BEGIN_RPC_MAP(BASIC_RPC_API_RESP)
RPC_MAP(RPC_TEST, on_test_resp)
RPC_MAP(RPC_GET_CONNECT_LIST, on_get_connect_list_resp)
RPC_MAP(RPC_PEER_POST_MSG, on_peer_post_msg_resp)
RPC_MAP(RPC_SHELL_HELP, on_shell_help)
END_RPC_MAP()

typedef struct rpc_connect {
//    char uuid[MAX_UUID_LEN];

} rpc_connect_t;


static int rpc_get_connect_list(struct rpc *r, struct rpc_connect *list, int *num)
{
    int len = 100;
    char *buf = (char *)calloc(1, len);
    memset(buf, 0xA5, len);
    rpc_call(r, RPC_GET_CONNECT_LIST, buf, len, NULL, 0);
    //printf("func_id = %x\n", RPC_GET_CONNECT_LIST);
    //dump_packet(&r->packet);
    return 0;
}

static int rpc_shell_help(struct rpc *r, void *buf, size_t len)
{
    char res[1024] = {0};
    rpc_call(r, RPC_SHELL_HELP, buf, len, res, sizeof(res));
    printf("return buffer\n%s", res);
    //dump_packet(&r->packet);
    return 0;
}

static int rpc_peer_post_msg(struct rpc *r, void *buf, size_t len)
{
    rpc_call(r, RPC_PEER_POST_MSG, buf, len, NULL, 0);
    //printf("func_id = %x\n", RPC_PEER_POST_MSG);
    //dump_packet(&r->packet);
    return 0;
}

static void usage(void)
{
    fprintf(stderr, "./test_libskt <ip> <port>\n");
    fprintf(stderr, "e.g. ./test_libskt 116.228.149.106 12345\n");
}

static void cmd_usage(void)
{
    printf("====rpc cmd====\n"
            "a: get all connect list\n"
            "p: post message to peer\n"
            "s: remote shell help\n"
            "q: quit\n"
            "\n");
}

static void *raw_data_thread(void *arg)
{
    struct rpc *r = (struct rpc *)arg;
    uint32_t uuid_dst;
    char cmd[512];
    int loop = 1;
    int i;
    int len = 1024;
    char ch;
    char *buf = (char *)calloc(1, len);
    for (i = 0; i < len; i++) {
        buf[i] = i;
    }

    while (loop) {
        memset(buf, 0, len);
        printf("input cmd> ");
        ch = getchar();
        switch (ch) {
        case 'a':
            rpc_get_connect_list(r, NULL, NULL);
            break;
        case 'p':
            printf("input uuid_dst> ");
            scanf("%x", &uuid_dst);
            printf("uuid_dst = %x\n", uuid_dst);
            r->send_pkt.header.uuid_dst = uuid_dst;
            sprintf(buf, "%s", "hello world");
            rpc_peer_post_msg(r, buf, 12);
            break;
        case 'q':
            loop = 0;
            rpc_destroy(r);
            break;
        case 's':
            printf("input shell cmd> ");
            scanf("%c", &ch);
            scanf("%[^\n]", cmd);
            printf("cmd = %s\n", cmd);
            rpc_shell_help(r, cmd, sizeof(cmd));
            break;
        case 'h':
            cmd_usage();
            break;
        default:
            break;
        }
        //dump_buffer(buf, len);
    }

    exit(0);
    return NULL;
}

int main(int argc, char **argv)
{
//116.228.149.106
    uint16_t port;
    char *ip;
    pthread_t tid;
    if (argc < 3) {
        usage();
        exit(0);
    }
    ip = argv[1];
    port = atoi(argv[2]);
    struct rpc *r = rpc_create(ip, port);
    if (!r) {
        printf("rpc_create failed\n");
        return -1;
    }
    RPC_REGISTER_MSG_MAP(BASIC_RPC_API_RESP);
    pthread_create(&tid, NULL, raw_data_thread, r);
    //rpc_dispatch(r);
    while (1) {
        sleep(1);
    }
    return 0;
}

#if 0
static int cmd_main(int argc, char **argv)
{
//116.228.149.106
    uint16_t port;
    char *ip;
    if (argc < 3) {
        usage();
        exit(0);
    }
    ip = argv[1];
    port = atoi(argv[2]);
    struct rpc *r = rpc_create(ip, port);
    if (!r) {
        printf("rpc_create failed\n");
        return -1;
    }
    int len = 100;
    char *buf = (char *)calloc(1, len);
    int olen = 100;
    char *obuf = (char *)calloc(1, olen);
    memset(obuf, 0xA5, olen);
    sprintf(buf, "%s", "I just want to get all connect list\n");
    rpc_call(r, RPC_GET_CONNECT_LIST, buf, len, obuf, olen);

    on_get_connect_list_resp(r, obuf, olen);
    rpc_destroy(r);
    return 0;
}
#endif
