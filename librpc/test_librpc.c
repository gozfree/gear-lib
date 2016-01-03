/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_librpc.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-05-07 00:04
 * updated: 2015-08-01 23:51
 *****************************************************************************/
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
    void *ptr;
    int num = 0;
    //logi("on_get_connect_list, len = %d\n", len);
    num = len / MAX_UUID_LEN;
    printf("\n");
    for (ptr = arg; num > 0; --num) {
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
    logd("msg from %s:\n%s\n", r->packet.header.uuid_src, (char *)arg);
    return 0;
}

static int on_shell_help(struct rpc *r, void *arg, int len)
{
    printf("msg from %x:\n%s\n", r->packet.header.uuid_src, (char *)arg);
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


int rpc_get_connect_list(struct rpc *r, struct rpc_connect *list, int *num)
{
    int len = 100;
    char *buf = calloc(1, len);
    memset(buf, 0xA5, len);
    rpc_call(r, RPC_GET_CONNECT_LIST, buf, len, NULL, 0);
    //printf("func_id = %x\n", RPC_GET_CONNECT_LIST);
    //dump_packet(&r->packet);
    return 0;
}

int rpc_shell_help(struct rpc *r, void *buf, size_t len)
{
    char res[1024] = {0};
    rpc_call(r, RPC_SHELL_HELP, buf, len, res, sizeof(res));
    //printf("func_id = %x\n", RPC_GET_CONNECT_LIST);
    //dump_packet(&r->packet);
    return 0;
}

int rpc_peer_post_msg(struct rpc *r, void *buf, size_t len)
{
    rpc_call(r, RPC_PEER_POST_MSG, buf, len, NULL, 0);
    //printf("func_id = %x\n", RPC_PEER_POST_MSG);
    //dump_packet(&r->packet);
    return 0;
}


void on_read(int fd, void *arg)
{
    //printf("on_read fd= %d\n", fd);
    struct rpc *r = (struct rpc *)arg;
    struct iovec *buf = rpc_recv_buf(r);
    if (!buf) {
        return;
    }
    process_msg(r, buf);
}

void on_write(int fd, void *arg)
{
//    printf("on_write fd= %d\n", fd);
}

void on_error(int fd, void *arg)
{
    printf("on_error fd= %d\n", fd);
}

void usage()
{
    fprintf(stderr, "./test_libskt <ip> <port>\n");
    fprintf(stderr, "e.g. ./test_libskt 116.228.149.106 12345\n");
}

void cmd_usage()
{
    printf("====rpc cmd====\n"
            "a: get all connect list\n"
            "p: post message to peer\n"
            "s: remote shell help\n"
            "q: quit\n"
            "\n");
}

void *raw_data_thread(void *arg)
{
    struct rpc *r = (struct rpc *)arg;
    uint32_t uuid_dst;
    char cmd[512];
    int loop = 1;
    int i;
    int len = 1024;
    char ch;
    char *buf = calloc(1, len);
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
            scanf("%d", &uuid_dst);
            r->packet.header.uuid_dst = uuid_dst;
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

int cmd_main(int argc, char **argv)
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
    rpc_set_cb(r, on_read, on_write, on_error, r);
    pthread_create(&tid, NULL, raw_data_thread, r);
    rpc_dispatch(r);
    return 0;
}

int main(int argc, char **argv)
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
    char *buf = calloc(1, len);
    memset(buf, 0xA5, len);
    int olen = 100;
    char *obuf = calloc(1, olen);
    memset(obuf, 0xA5, olen);
    rpc_call(r, RPC_GET_CONNECT_LIST, buf, len, obuf, olen);

    on_get_connect_list_resp(r, obuf, olen);
    return 0;
}
