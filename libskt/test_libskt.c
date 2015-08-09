/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    test_libskt.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-05-03 18:27
 * updated: 2015-07-11 20:01
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <libgevent.h>
#include "libskt.h"

struct conn {
    int fd;

};

static struct gevent_base *g_evbase;

int tcp_client(const char *host, uint16_t port)
{
    char str_ip[INET_ADDRSTRLEN];
    int n;
    char buf[64];
    struct skt_connection *sc;

    sc = skt_tcp_connect(host, port);
    if (sc == NULL) {
        printf("connect failed!\n");
        return -1;
    }
    skt_addr_ntop(str_ip, sc->local.ip);
    printf("local ip = %s, port = %d\n", str_ip, sc->local.port);
    skt_addr_ntop(str_ip, sc->remote.ip);
    printf("remote ip = %s, port = %d\n", str_ip, sc->remote.port);
    while (1) {
        memset(buf, 0, sizeof(buf));
        printf("input> ");
        scanf("%s", buf);
        n = skt_send(sc->fd, buf, strlen(buf));
        if (n == -1) {
            printf("skt_send failed!\n");
            return -1;
        }
//        sleep(1);
    }
}

void on_recv(int fd, void *arg)
{
    char buf[100];
    memset(buf, 0, sizeof(buf));
    skt_recv(fd, buf, 100);
    printf("recv buf = %s\n", buf);
}
void on_error(int fd, void *arg)
{
    printf("error: %d\n", errno);
}
void on_connect(int fd, void *arg)
{
    char str_ip[INET_ADDRSTRLEN];
    int afd;
    uint32_t ip;
    uint16_t port;
    afd = skt_accept(fd, &ip, &port);
    if (afd == -1) {
        printf("errno=%d %s\n", errno, strerror(errno));
        return;
    };
    skt_addr_ntop(str_ip, ip);
    printf("ip = %s, port = %d\n", str_ip, port);

    struct gevent *e = gevent_create(afd, on_recv, NULL, on_error, (void *)&afd);
    if (-1 == gevent_add(g_evbase, e)) {
        printf("event_add failed!\n");
    }
}



int tcp_server(uint16_t port)
{
    int fd;
    fd = skt_tcp_bind_listen(NULL, port, 0);
    if (fd == -1) {
        return -1;
    }
    g_evbase = gevent_base_create();
    if (!g_evbase) {
        return -1;
    }

    struct gevent *e = gevent_create(fd, on_connect, NULL, on_error, NULL);
    if (-1 == gevent_add(g_evbase, e)) {
        printf("event_add failed!\n");
    }
    gevent_base_loop(g_evbase);

    return 0;
}

void usage()
{
    fprintf(stderr, "./test_libskt -s port\n"
                    "./test_libskt -c ip port\n");

}

void addr_test()
{
    char str_ip[INET_ADDRSTRLEN];
    uint32_t net_ip;
    net_ip = skt_addr_pton("192.168.1.123");
    printf("ip = %d\n", net_ip);
    skt_addr_ntop(str_ip, net_ip);
    printf("ip = %s\n", str_ip);

}

void domain_test()
{
    void *p;
    char str[MAX_ADDR_STRING];
    skt_addr_list_t *tmp;
    if (0 == skt_get_local_list(&tmp, 0)) {
        for (; tmp; tmp = tmp->next) {
            skt_addr_ntop(str, tmp->addr.ip);
            printf("ip = %s port = %d\n", str, tmp->addr.port);
        }
    }

    if (0 == skt_getaddrinfo(&tmp, "www.sina.com", "3478")) {
        for (; tmp; tmp = tmp->next) {
            skt_addr_ntop(str, tmp->addr.ip);
            printf("ip = %s port = %d\n", str, tmp->addr.port);
        }
    }
    if (0 == skt_gethostbyname(&tmp, "www.baidu.com")) {
        for (; tmp; tmp = tmp->next) {
            skt_addr_ntop(str, tmp->addr.ip);
            printf("ip = %s port = %d\n", str, tmp->addr.port);
        }
    }

    do {
        p = tmp;
        if (tmp) tmp = tmp->next;
        if (p) free(p);
    } while (tmp);
}

int main(int argc, char **argv)
{
    uint16_t port;
    char *ip;
    if (argc < 2) {
        usage();
        exit(0);
    }
    if (!strcmp(argv[1], "-s")) {
        if (argc == 3)
            port = atoi(argv[2]);
        else
            port = 0;
        tcp_server(port);
    } else if (!strcmp(argv[1], "-c")) {
        if (argc == 3) {
            ip = "127.0.0.1";
            port = atoi(argv[2]);
        } else if (argc == 4) {
            ip = argv[2];
            port = atoi(argv[3]);
        }
        tcp_client(ip, port);
    }
    if (!strcmp(argv[1], "-t")) {
        addr_test();
    }
    if (!strcmp(argv[1], "-d")) {
        domain_test();
    }
    return 0;
}
