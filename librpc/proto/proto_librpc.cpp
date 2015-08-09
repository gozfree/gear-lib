/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    proto_librpc.cpp
 * author:  gozfree <gozfree@163.com>
 * created: 2015-05-07 00:04
 * updated: 2015-05-07 00:04
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
#include <iostream>
#include "../librpc.h"
#include "librpc.pb.h"
#include "hello.pb.h"

using namespace std;

#if 0
void dump_buffer(void *buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (!(i%16))
           printf("\n%p: ", buf+i);
        printf("%02x ", (*((char *)buf + i)) & 0xff);
    }
    printf("\n");
}
#endif


void on_read(int fd, void *arg)
{
    struct rpc *r = (struct rpc *)arg;
    struct iobuf *buf = rpc_recv_buf(r);
    if (!buf) {
        return;
    }
    string reqbuf((const char *)buf->addr, buf->len);
    hello::request req;
    if (!req.ParseFromString(reqbuf)) {
        fprintf(stderr, "parse message failed!\n");
    }
    cout << "request:>>>>>>>>\n" << req.DebugString() << ">>>>>>>>>>>>>>>\n" << endl;
    //printf("recv len = %d, buf = %s\n", buf->len, buf->addr);
    //dump_buffer(buf->addr, buf->len);
}

void on_write(int fd, void *arg)
{
    printf("on_write fd= %d\n", fd);
}

void on_error(int fd, void *arg)
{
    printf("on_error fd= %d\n", fd);
}

void usage()
{
    fprintf(stderr, "./test_libskt <ip> <port>\n");
}

void *input_thread(void *arg)
{
    struct rpc *r = (struct rpc *)arg;
    char buf[640];
    char uuid_dst[MAX_UUID_LEN];
    int ret;
    while (1) {
        memset(buf, 0, sizeof(buf));
        printf("input uuid_dst> ");
        scanf("%s", uuid_dst);
        strncpy(r->uuid_dst, uuid_dst, MAX_UUID_LEN);
        printf("input strbuf[64]> ");
        scanf("%s", buf);
        ret = rpc_send(r, buf, strlen(buf));
        printf("rpc_send ret = %d, errno=%d\n", ret, errno);
    }

}

void *raw_data_thread(void *arg)
{
    struct rpc *r = (struct rpc *)arg;
    char uuid_dst[MAX_UUID_LEN];
    int ret, i;
    int len = 1024;
    char *buf = (char *)calloc(1, len);
    for (i = 0; i < len; i++) {
        buf[i] = i;
    }
    hello::request req;
    string wbuf;
    req.set_id(HELLO);
    req.set_uint32_arg(1234);
    req.set_string_arg("hello");

    if (!req.SerializeToString(&wbuf)) {
        fprintf(stderr, "serialize to string failed!\n");
        return NULL;
    }

    cout << "request:>>>>>>>>\n" << req.DebugString() << ">>>>>>>>>>>>>>>\n" << endl;

    while (1) {
        memset(buf, 0, sizeof(buf));
        printf("input uuid_dst> ");
        scanf("%s", uuid_dst);
        strncpy(r->uuid_dst, uuid_dst, MAX_UUID_LEN);
        ret = rpc_send(r, wbuf.data(), wbuf.length());
        printf("rpc_send ret = %d, errno=%d\n", ret, errno);
        //dump_buffer(buf, len);
    }

}

int main(int argc, char **argv)
{
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
    rpc_set_cb(r, on_read, on_write, on_error, r);
    //pthread_create(&tid, NULL, input_thread, r);
    pthread_create(&tid, NULL, raw_data_thread, r);
    rpc_dispatch(r);
    return 0;
}
