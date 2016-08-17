/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_file_transfer.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-10 00:15
 * updated: 2015-08-10 00:15
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#ifndef __ANDROID__
#include <ifaddrs.h>
#endif
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include "libptcp.h"

static long filesize(FILE *fd)
{
    long curpos, length;
    curpos = ftell(fd);
    fseek(fd, 0L, SEEK_END);
    length = ftell(fd);
    fseek(fd, curpos, SEEK_SET);
    return length;
}

struct xfer_callback {
    void *(*xfer_server_init)(const char *host, uint16_t port);
    void *(*xfer_client_init)(const char *host, uint16_t port);
    int (*xfer_send)(void *arg, void *buf, size_t len);
    int (*xfer_recv)(void *arg, void *buf, size_t len);
    int (*xfer_close)(void *arg);
    int (*xfer_errno)(void *arg);
};

//=====================tcp=======================
static void *tcp_server_init(const char *host, uint16_t port)
{
    int fd;
    struct sockaddr_in si;
    socklen_t len = sizeof(si);

    si.sin_family = AF_INET;
    si.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;
    si.sin_port = htons(port);
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == fd) {
        printf("socket: %s\n", strerror(errno));
        return NULL;
    }
    if (-1 == bind(fd, (struct sockaddr*)&si, sizeof(si))) {
        printf("bind: %s\n", strerror(errno));
        close(fd);
        return NULL;
    }
    if (-1 == listen(fd, SOMAXCONN)) {
        printf("listen: %s\n", strerror(errno));
        close(fd);
        return NULL;
    }

    int *afd = (int *)calloc(1, sizeof(int));
    *afd = accept(fd, (struct sockaddr *)&si, &len);
    if (*afd == -1) {
        printf("accept: %s\n", strerror(errno));
        return NULL;
    }
    return (void *)afd;
}

static void *tcp_client_init(const char *host, uint16_t port)
{
    int *fd = (int *)calloc(1, sizeof(int));
    struct sockaddr_in si;
    *fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == *fd) {
        printf("socket: %s\n", strerror(errno));
        return NULL;
    }
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);

    if (-1 == connect(*fd, (struct sockaddr*)&si, sizeof(si))) {
        printf("connect: %s\n", strerror(errno));
        close(*fd);
        return NULL;
    }
    return (void *)fd;
}

static int tcp_send(void *arg, void *buf, size_t len)
{
    int *fd = (int *)arg;
    return send(*fd, buf, len, 0);
}

static int tcp_recv(void *arg, void *buf, size_t len)
{
    int *fd = (int *)arg;
    return recv(*fd, buf, len, 0);
}

static int tcp_close(void *arg)
{
    int *fd = (int *)arg;
    return close(*fd);
}

static int tcp_errno(void *arg)
{
    return errno;
}

//=================udp=======================
static void *udp_server_init(const char *host, uint16_t port)
{
    int *fd = (int *)calloc(1, sizeof(int));
    struct sockaddr_in si;
    *fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (-1 == *fd) {
        printf("socket: %s\n", strerror(errno));
        return NULL;
    }
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);

    if (-1 == bind(*fd, (struct sockaddr*)&si, sizeof(si))) {
        printf("connect: %s\n", strerror(errno));
        close(*fd);
        return NULL;
    }
    return (void *)fd;
}

static void *udp_client_init(const char *host, uint16_t port)
{
    int *fd = (int *)calloc(1, sizeof(int));
    struct sockaddr_in si;
    *fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (-1 == *fd) {
        printf("socket: %s\n", strerror(errno));
        return NULL;
    }
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);

    if (-1 == connect(*fd, (struct sockaddr*)&si, sizeof(si))) {
        printf("connect: %s\n", strerror(errno));
        close(*fd);
        return NULL;
    }
    return (void *)fd;
}

static int udp_send(void *arg, void *buf, size_t len)
{
    int *fd = (int *)arg;
    return send(*fd, buf, len, 0);
}

static int udp_recv(void *arg, void *buf, size_t len)
{
    int *fd = (int *)arg;
    return recv(*fd, buf, len, 0);
}

static int udp_close(void *arg)
{
    int *fd = (int *)arg;
    return close(*fd);
}

static int udp_errno(void *arg)
{
    return errno;
}

//=================ptcp=======================
static void *ptcp_server_init(const char *host, uint16_t port)
{
    ptcp_socket_t *ps = ptcp_socket();
    if (ps == NULL) {
        printf("error!\n");
        return NULL;
    }

    struct sockaddr_in si;
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;
    si.sin_port = htons(port);

    ptcp_bind(ps, (struct sockaddr*)&si, sizeof(si));
    ptcp_listen(ps, 0);
    return ps;
}

static int _ptcp_recv(void *arg, void *buf, size_t len)
{
    ptcp_socket_t *ps = (ptcp_socket_t *)arg;
    int ret = ptcp_recv(ps, buf, len);
    return ret;
}

static void *ptcp_client_init(const char *host, uint16_t port)
{
    ptcp_socket_t *ps = ptcp_socket();
    if (ps == NULL) {
        printf("error!\n");
        return NULL;
    }

    struct sockaddr_in si;
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);
    if (0 != ptcp_connect(ps, (struct sockaddr*)&si, sizeof(si))) {
        printf("ptcp_connect failed!\n");
    } else {
        printf("ptcp_connect success\n");
    }
    return ps;
}

static int _ptcp_send(void *arg, void *buf, size_t len)
{
    ptcp_socket_t *ps = (ptcp_socket_t *)arg;
    return ptcp_send(ps, buf, len);
}

static int _ptcp_close(void *arg)
{
    ptcp_socket_t *ps = (ptcp_socket_t *)arg;
    ptcp_close(ps);
    return 0;
}

#define ECLOSED 123
static int ptcp_errno(void *arg)
{
    ptcp_socket_t *ps = (ptcp_socket_t *)arg;

    if (ptcp_is_closed(ps) || ptcp_is_closed_remotely(ps)) {
        return ECLOSED;
    }
    return ptcp_get_error(ps);
}

static int file_send(char *name, struct xfer_callback *cbs)
{
    int len, flen, slen, total;
    char buf[1024] = {0};
    FILE *fp = fopen(name, "r");
    assert(fp);
    total = flen = filesize(fp);
    void *arg = cbs->xfer_client_init("127.0.0.1", 5555);

    sleep(1);
    while (flen > 0) {
        usleep(20 * 1000);
        len = fread(buf, 1, sizeof(buf), fp);
        if (len == -1) {
            printf("%s:%d xxxx\n", __func__, __LINE__);
            break;
        }
        slen = cbs->xfer_send(arg, buf, len);
        if (slen <= 0) {
            printf("xfer_send error: %d\n", cbs->xfer_errno(arg));
            break;
        }
        if (slen != len) {
            printf("%s:%d slen=%d, len=%d\n", __func__, __LINE__, slen, len);
        }
        flen -= len;
        printf("already sent %s file %dKB/%dKB = %02f%%\n", name, (total-flen), total, (total-flen)/(total*1.0)*100);
    }
    sleep(1);
    cbs->xfer_close(arg);
    fclose(fp);
    printf("file %s length is %u\n", name, flen);
    return total;
}
static int file_recv(char *name, struct xfer_callback *cbs)
{
    int len, flen, rlen;
    char buf[1024] = {0};
    FILE *fp = fopen(name, "w");
    assert(fp);
    void *arg = cbs->xfer_server_init("127.0.0.1", 5555);
    assert(arg);

    flen = 0;
    while (1) {
        usleep(10 * 1000);
        memset(buf, 0, sizeof(buf));
        rlen = cbs->xfer_recv(arg, buf, sizeof(buf));
        if (rlen <= 0) {
            if (ECLOSED == cbs->xfer_errno(arg)){
                printf("xfer closed\n");
                break;
            } else {
                printf("%s:%d errno:%d\n", __func__, __LINE__, cbs->xfer_errno(arg));
                usleep(100 * 1000);
                continue;
            }
        }
        len = fwrite(buf, 1, rlen, fp);
        assert(len==rlen);
        flen += len;
        printf("already recv %s file %dKB\n", name, flen);
    }
    cbs->xfer_close(arg);
    fclose(fp);
    return flen;
}

static void sigterm_handler(int sig)
{
    exit(0);
}

static struct xfer_callback tcp_cbs = {
    tcp_server_init,
    tcp_client_init,
    tcp_send,
    tcp_recv,
    tcp_close,
    tcp_errno
};
static struct xfer_callback udp_cbs = {
    udp_server_init,
    udp_client_init,
    udp_send,
    udp_recv,
    udp_close,
    udp_errno
};
static struct xfer_callback ptcp_cbs = {
    ptcp_server_init,
    ptcp_client_init,
    _ptcp_send,
    _ptcp_recv,
    _ptcp_close,
    ptcp_errno
};
int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("./test -s / -r filename\n");
        return 0;
    }
    signal(SIGPIPE,SIG_IGN);
    signal(SIGINT, sigterm_handler);
    struct xfer_callback *cbs;
    cbs = &tcp_cbs;
    cbs = &udp_cbs;
    cbs = &ptcp_cbs;
    if (!strcmp(argv[1], "-s")) {
        file_send(argv[2], cbs);
    }
    if (!strcmp(argv[1], "-r")) {
        file_recv(argv[2], cbs);
    }
    return 0;
}
