/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    echo.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-07-20 00:01
 * updated: 2015-08-02 17:44
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <libgzf.h>
#include <liblog.h>
#include <libgevent.h>
#include <libdict.h>
#include <libskt.h>
#include <libworkq.h>
#include "echod.h"

#define MAX_UUID_LEN                (21)
#define ECHO_LISTEN_PORT    12345

struct echod *_echod;
struct echo_connection {
    int fd;
    uint32_t uuid_dst;
    uint32_t uuid_src;
    struct gevent *ev;

};


struct wq_arg {
    struct echo_connection r;
    void *buf;
    size_t len;

};

void echo_connect_destroy(struct echod *echod, struct echo_connection *ec);
void on_recv(int fd, void *arg)
{
    int ret;
    char tmp[2048];
    char key[9];
    snprintf(key, sizeof(key), "%08x", fd);
    struct echo_connection *r = (struct echo_connection *)dict_get(_echod->dict_fd2rpc, key, NULL);
    if (!r) {
        loge("dict_get failed: key=%s", key);
        return;
    }
    ret = skt_recv(r->fd, tmp, 2048);
    if (ret > 0) {
        printf("skt_recv: fd = %d, ret = %d, tmp=%s\n", fd, ret, tmp);
        ret = skt_send(r->fd, "HTTP/1.1 200 OK\n\n", 20);
//        ret = skt_send(r->fd, "Content-Type: application/json\n\n", 35);
        printf("skt_send: ret = %d\n", ret);
//        ret = skt_send(r->fd, "{\"code\":\"200\", \"msg\":\"OK\" }\n", 22);
//        printf("skt_send: ret = %d\n", ret);
    }
    //r->fd = fd;//must be reset
    echo_connect_destroy(_echod, r);
}

void on_error(int fd, void *arg)
{
    loge("error: %d\n", errno);
}

int create_uuid(char *uuid, int len, int fd, uint32_t ip, uint16_t port)
{
    snprintf(uuid, MAX_UUID_LEN, "%08x%08x%04x", fd, ip, port);
    return 0;
}

int echod_connect_add(struct echod *echod, struct echo_connection *r, int fd, uint32_t uuid)
{
    char fd_str[9];
    char uuid_str[9];
    char *fdval = (char *)calloc(1, 9);
    snprintf(fd_str, sizeof(fd_str), "%08x", fd);
    snprintf(uuid_str, sizeof(uuid_str), "%08x", uuid);
    snprintf(fdval, 9, "%08x", fd);
    dict_add(echod->dict_fd2rpc, fd_str, (char *)r);
    dict_add(echod->dict_uuid2fd, uuid_str, fdval);
    logd("add connection fd:%s, uuid:%s\n", fd_str, uuid_str);
    return 0;
}

int echod_connect_del(struct echod *echod, int fd, uint32_t uuid)
{
    char uuid_str[9];
    char fd_str[9];
    snprintf(fd_str, sizeof(fd_str), "%08x", fd);
    snprintf(uuid_str, sizeof(uuid_str), "%08x", uuid);
    dict_del(echod->dict_fd2rpc, fd_str);
    dict_del(echod->dict_uuid2fd, uuid_str);
    skt_close(fd);
    logi("delete connection fd:%s, uuid:%s\n", fd_str, uuid_str);
    return 0;
}

void echo_connect_create(struct echod *echod,
                int fd, uint32_t ip, uint16_t port)
{
    char str_ip[INET_ADDRSTRLEN];
    char uuid[MAX_UUID_LEN];
    uint32_t uuid_hash;

    struct echo_connection *ec = CALLOC(1, struct echo_connection);
    if (!ec) {
        loge("malloc failed!\n");
        return;
    }
    ec->fd = fd;
    create_uuid(uuid, MAX_UUID_LEN, fd, ip, port);
    uuid_hash = hash_murmur(uuid, sizeof(uuid));
    struct gevent *e = gevent_create(fd, on_recv, NULL, on_error, (void *)ec);
    if (-1 == gevent_add(echod->evbase, e)) {
        loge("event_add failed!\n");
    }
    ec->ev = e;
    ec->uuid_dst = uuid_hash;
    ec->uuid_src = uuid_hash;
    echod_connect_add(echod, ec, fd, uuid_hash);
    skt_addr_ntop(str_ip, ip);
    logd("on_connect fd = %d, remote_addr = %s:%d, uuid=0x%08x\n",
                    fd, str_ip, port, uuid_hash);

    return;
}

void echo_connect_destroy(struct echod *echod, struct echo_connection *ec)
{
    if (!echod || !ec) {
        loge("invalid paramets!\n");
        return;
    }
    int fd = ec->fd;
    uint32_t uuid = ec->uuid_src;
    struct gevent *e = ec->ev;
    gevent_del(echod->evbase, e);
    echod_connect_del(echod, fd, uuid);
}

void on_connect(int fd, void *arg)
{
    int afd;
    uint32_t ip;
    uint16_t port;
    struct echod *echod = (struct echod *)arg;

    afd = skt_accept(fd, &ip, &port);
    if (afd == -1) {
        loge("skt_accept failed: %d\n", errno);
        return;
    }
    echo_connect_create(echod, afd, ip, port);
}

int echod_init(uint16_t port)
{
    int fd;
    fd = skt_tcp_bind_listen(NULL, port, 1);
    if (fd == -1) {
        loge("skt_tcp_bind_listen port:%d failed!\n", port);
        return -1;
    }
    if (0 > skt_set_tcp_keepalive(fd, 1)) {
        loge("skt_set_tcp_keepalive failed!\n");
        return -1;
    }
    logi("echod listen port = %d\n", port);
    _echod = CALLOC(1, struct echod);
    _echod->listen_fd = fd;
    _echod->evbase = gevent_base_create();
    if (!_echod->evbase) {
        loge("gevent_base_create failed!\n");
        return -1;
    }
    struct gevent *e = gevent_create(fd, on_connect, NULL, on_error,
                    (void *)_echod);
    if (-1 == gevent_add(_echod->evbase, e)) {
        loge("event_add failed!\n");
        gevent_destroy(e);
    }
    _echod->dict_fd2rpc = dict_new();
    _echod->dict_uuid2fd = dict_new();
    _echod->wq = wq_create();
    return 0;
}

int echod_dispatch()
{
    gevent_base_loop(_echod->evbase);
    return 0;
}

void echod_deinit()
{
    gevent_base_loop_break(_echod->evbase);
    gevent_base_destroy(_echod->evbase);
    wq_destroy(_echod->wq);
}

void usage()
{
    printf("usage: run as daemon: ./echod -d\n"
            "      run for debug: ./echod\n");
}

void ctrl_c_op(int signo)
{
    skt_close(_echod->listen_fd);
    exit(0);
}

int main(int argc, char **argv)
{
    uint16_t port = ECHO_LISTEN_PORT;
    if (argc > 2) {
        usage();
        exit(-1);
    }
    if (argc == 2) {
        if (!strcmp(argv[1], "-d")) {
            daemon(0, 0);
            log_init(LOG_RSYSLOG, "local2");
        } else {
            port = atoi(argv[1]);
        }
    } else {
        log_init(LOG_STDERR, NULL);
    }
    log_set_level(LOG_INFO);
    signal(SIGINT , ctrl_c_op);
    echod_init(port);
    echod_dispatch();
    echod_deinit();
    return 0;
}
