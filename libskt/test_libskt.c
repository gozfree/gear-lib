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
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <libgevent.h>
#include <libthread.h>
#include <libmacro.h>
#include "libskt.h"

struct skt_connection *g_sc = NULL;

static struct gevent_base *g_evbase;

static uint8_t socket_id = -1;
void on_read(int fd, void *arg)
{
    char buf[14];
    memset(buf, 0, sizeof(buf));
    int ret = skt_recv(fd, buf, sizeof(buf));
    if (ret > 0) {
        DUMP_BUFFER(buf, sizeof(buf));
        socket_id = buf[6];
        printf("socket_id = %d\n", socket_id);
    } else if (ret == 0) {
        printf("delete connection fd:%d\n", fd);
    } else if (ret < 0) {
        printf("recv failed!\n");
    }
}
void on_recv(int fd, void *arg)
{
    char buf[2048];
    memset(buf, 0, sizeof(buf));
    int ret = skt_recv(fd, buf, 2048);
    if (ret > 0) {
        printf("recv buf = %s\n", buf);
    } else if (ret == 0) {
        printf("delete connection fd:%d\n", fd);
    } else if (ret < 0) {
        printf("recv failed!\n");
    }
}
void on_error(int fd, void *arg)
{
    printf("error: %d\n", errno);
}
static void *tcp_client_thread(struct thread *thread, void *arg)
{
    struct skt_connection *sc = (struct skt_connection *)arg;
    g_evbase = gevent_base_create();
    if (!g_evbase) {
        return NULL;
    }
    if (0 > skt_set_nonblock(sc->fd)) {
        printf("skt_set_nonblock failed!\n");
        return NULL;
    }
    struct gevent *e = gevent_create(sc->fd, on_recv, NULL, on_error, (void *)&sc->fd);
    if (-1 == gevent_add(g_evbase, e)) {
        printf("event_add failed!\n");
    }
    gevent_base_loop(g_evbase);
    return NULL;
}

int tcp_client(const char *host, uint16_t port)
{
    char str_ip[INET_ADDRSTRLEN];
    int n, ret;
    char buf[64];
    struct tcp_info tcpi;

    g_sc = skt_tcp_connect(host, port);
    if (g_sc == NULL) {
        printf("connect failed!\n");
        return -1;
    }
    on_read(g_sc->fd, NULL);
    skt_addr_ntop(str_ip, g_sc->local.ip);
    printf("local ip = %s, port = %d\n", str_ip, g_sc->local.port);
    skt_addr_ntop(str_ip, g_sc->remote.ip);
    printf("remote ip = %s, port = %d\n", str_ip, g_sc->remote.port);
    skt_set_tcp_keepalive(g_sc->fd, 1);
    memset(&tcpi, 0, sizeof(tcpi));
    ret = skt_get_tcp_info(g_sc->fd, &tcpi);
    if (ret == 0) {
        printf("unrecovered=%u "
             "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
             "lost=%u retrans=%u rtt=%u rttvar=%u "
             "sshthresh=%u cwnd=%u total_retrans=%u\n",
             tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
             tcpi.tcpi_rto,          // Retransmit timeout in usec
             tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
             tcpi.tcpi_snd_mss,
             tcpi.tcpi_rcv_mss,
             tcpi.tcpi_lost,         // Lost packets
             tcpi.tcpi_retrans,      // Retransmitted packets out
             tcpi.tcpi_rtt,          // Smoothed round trip time in usec
             tcpi.tcpi_rttvar,       // Medium deviation
             tcpi.tcpi_snd_ssthresh,
             tcpi.tcpi_snd_cwnd,
             tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
    }
    struct thread *thread = thread_create(tcp_client_thread, g_sc);
    if (!thread) {
        printf("thread_create failed!\n");
    }
    while (1) {
        memset(buf, 0, sizeof(buf));
        printf("input> ");
        scanf("%s", buf);
        n = skt_send(g_sc->fd, buf, strlen(buf));
        if (n == -1) {
            printf("skt_send failed!\n");
            return -1;
        }
//        sleep(1);
    }
}

int udp_client(const char *host, uint16_t port)
{
    struct skt_connection *sc = skt_udp_connect(host, port);
    int ret = skt_send(sc->fd, "aaa", 4);
    printf("fd = %d, ret = %d\n", sc->fd, ret);
    return 0;
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
    printf("new connect comming: ip = %s, port = %d\n", str_ip, port);
    if (0 > skt_set_nonblock(afd)) {
        printf("skt_set_nonblock failed!\n");
        return;
    }
    struct gevent *e = gevent_create(afd, on_recv, NULL, on_error, (void *)&afd);
    if (-1 == gevent_add(g_evbase, e)) {
        printf("event_add failed!\n");
    }
}

int udp_server(uint16_t port)
{
    int fd;
    fd = skt_udp_bind(NULL, port);
    if (fd == -1) {
        return -1;
    }
    g_evbase = gevent_base_create();
    if (!g_evbase) {
        return -1;
    }

    skt_set_noblk(fd, true);
    struct gevent *e = gevent_create(fd, on_recv, NULL, on_error, NULL);
    if (-1 == gevent_add(g_evbase, e)) {
        printf("event_add failed!\n");
    }
    gevent_base_loop(g_evbase);

    return 0;
}

int tcp_server(uint16_t port)
{
    int fd;
    fd = skt_tcp_bind_listen(NULL, port);
    if (fd == -1) {
        return -1;
    }
    struct skt_addr addr;
    skt_getaddr_by_fd(fd, &addr);
    printf("addr = %s\n", addr.ip_str);
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
    printf("ip = %x\n", net_ip);
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

void ctrl_c_op(int signo)
{
    if (g_sc) {
        skt_close(g_sc->fd);
    }
    exit(0);
}

int main(int argc, char **argv)
{
    uint16_t port;
    const char *ip;
    skt_get_local_info();
    if (argc < 2) {
        usage();
        exit(0);
    }
    signal(SIGINT, ctrl_c_op);
    if (!strcmp(argv[1], "-s")) {
        if (argc == 3)
            port = atoi(argv[2]);
        else
            port = 0;
        //tcp_server(port);
        udp_server(port);
    } else if (!strcmp(argv[1], "-c")) {
        if (argc == 3) {
            ip = "127.0.0.1";
            port = atoi(argv[2]);
        } else if (argc == 4) {
            ip = argv[2];
            port = atoi(argv[3]);
        }
        //tcp_client(ip, port);
        udp_client(ip, port);
    }
    if (!strcmp(argv[1], "-t")) {
        addr_test();
    }
    if (!strcmp(argv[1], "-d")) {
        domain_test();
    }
    return 0;
}
