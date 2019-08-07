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
#include "../librpc_stub.h"
#include "librpc.pb.h"
#include "hello.pb.h"

using namespace std;

static int on_calc_resp(struct rpc *r, void *arg, int len)
{
    string reqbuf((const char *)arg, len);
    hello::request req;
    if (!req.ParseFromString(reqbuf)) {
        fprintf(stderr, "parse message failed!\n");
    }
    cout << "request:>>>>>>>>\n" << req.DebugString() << ">>>>>>>>>>>>>>>\n" << endl;
    //printf("recv len = %d, buf = %s\n", buf->len, buf->addr);
    //dump_buffer(buf->addr, buf->len);
    return 0;
}


static int on_hello_resp(struct rpc *r, void *arg, int len)
{
    string reqbuf((const char *)arg, len);
    hello::request req;
    if (!req.ParseFromString(reqbuf)) {
        fprintf(stderr, "parse message failed!\n");
    }
    cout << "request:>>>>>>>>\n" << req.DebugString() << ">>>>>>>>>>>>>>>\n" << endl;
    //printf("recv len = %d, buf = %s\n", buf->len, buf->addr);
    //dump_buffer(buf->addr, buf->len);
    return 0;
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

int rpc_hello(struct rpc *r)
{

    hello::request req;
    string wbuf;
    req.set_id(HELLO);
    req.set_uint32_arg(1234);
    req.set_string_arg("hello");

    if (!req.SerializeToString(&wbuf)) {
        fprintf(stderr, "serialize to string failed!\n");
        return -1;
    }

    cout << "request:>>>>>>>>\n" << req.DebugString() << ">>>>>>>>>>>>>>>\n" << endl;

    rpc_call(r, RPC_HELLO, wbuf.data(), wbuf.length(), NULL, 0);
    return 0;
}

void *raw_data_thread(void *arg)
{
    struct rpc *r = (struct rpc *)arg;
    rpc_hello(r);
    return NULL;
}

BEGIN_RPC_MAP(BASIC_RPC_API_RESP)
RPC_MAP(RPC_HELLO, on_hello_resp)
RPC_MAP(RPC_CALC, on_calc_resp)
END_RPC_MAP()


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
    RPC_REGISTER_MSG_MAP(BASIC_RPC_API_RESP);
    pthread_create(&tid, NULL, raw_data_thread, r);
    while (1) {
        sleep(1);
    }
    return 0;
}

