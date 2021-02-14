/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#include "libsock.h"
#include "libsock_ext.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#if defined (OS_LINUX)
#include <unistd.h>
#include <signal.h>
#endif
#include <libgevent.h>
#include <libthread.h>

struct sock_connection *g_sc = NULL;

static struct gevent_base *g_evbase;

static uint8_t socket_id = -1;
void on_read(int fd, void *arg)
{
    char buf[14];
    int ret=0;
    memset(buf, 0, sizeof(buf));
    ret = sock_recv(fd, buf, sizeof(buf));
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

static void on_connect_server(int fd, struct sock_connection *conn)
{
    printf("on_connect_server: fd=%d local=%s:%d, remote=%s:%d\n", conn->fd,
            conn->local.ip_str, conn->local.port,
            conn->remote.ip_str, conn->remote.port);
}

static void on_recv_buf(int fd, void *buf, size_t len)
{

    printf("xxx fd = %d, recv buf = %s\n", fd, (char *)buf);
}

void on_recv(int fd, void *arg)
{
    char buf[2048];
    int ret=0;
    memset(buf, 0, sizeof(buf));
    ret = sock_recv(fd, buf, 2048);
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
    struct gevent *e =NULL;
    struct sock_connection *sc = (struct sock_connection *)arg;
    g_evbase = gevent_base_create();
    if (!g_evbase) {
        return NULL;
    }
    if (0 > sock_set_nonblock(sc->fd)) {
        printf("sock_set_nonblock failed!\n");
        return NULL;
    }
    e = gevent_create(sc->fd, on_recv, NULL, on_error, (void *)&sc->fd);
    if (-1 == gevent_add(g_evbase, &e)) {
        printf("event_add failed!\n");
    }
    gevent_base_loop(g_evbase);
    return NULL;
}

int tcp_client(const char *host, uint16_t port)
{
    char str_ip[SOCK_ADDR_LEN];
    int n, ret;
    char buf[64];
    struct thread *thread =NULL;
#if defined (OS_LINUX)
    struct tcp_info tcpi;
#endif
    VERBOSE();
    g_sc = sock_tcp_connect(host, port);
    if (g_sc == NULL) {
        printf("connect failed!\n");
        return -1;
    }
    VERBOSE();
    //on_read(g_sc->fd, NULL);
    VERBOSE();
    sock_addr_ntop(str_ip, g_sc->local.ip);
    printf("local ip = %s, port = %d\n", str_ip, g_sc->local.port);
    sock_addr_ntop(str_ip, g_sc->remote.ip);
    printf("remote ip = %s, port = %d\n", str_ip, g_sc->remote.port);
    sock_set_tcp_keepalive(g_sc->fd, 1);
    VERBOSE();
    
#if defined (OS_LINUX)
    memset(&tcpi, 0, sizeof(tcpi));
    ret = sock_get_tcp_info(g_sc->fd, &tcpi);
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
#endif
    VERBOSE();
    thread = thread_create(tcp_client_thread, g_sc);
    if (!thread) {
        printf("thread_create failed!\n");
    }
    VERBOSE();
    while (1) {
        memset(buf, 0, sizeof(buf));
        printf("input> ");
        scanf("%s", buf);
        n = sock_send(g_sc->fd, buf, strlen(buf));
        if (n == -1) {
            printf("sock_send failed!\n");
            return -1;
        }
//        sleep(1);
    }
}

int udp_client(const char *host, uint16_t port)
{
    char buf[64];
    int n, ret;
    struct sock_connection *sc = sock_udp_connect(host, port);
    ret = sock_send(sc->fd, "aaa", 4);
    printf("fd = %d, ret = %d\n", sc->fd, ret);
    while (1) {
        memset(buf, 0, sizeof(buf));
        printf("input> ");
        scanf("%s", buf);
        n = sock_send(sc->fd, buf, strlen(buf));
        if (n == -1) {
            printf("sock_send failed!\n");
            return -1;
        }
//        sleep(1);
    }
    free(sc);
    return 0;
}

void usage()
{
    fprintf(stderr, "./test_libsock -s port\n"
                    "./test_libsock -c ip port\n");
}

void addr_test()
{
    char str_ip[SOCK_ADDR_LEN];
    uint32_t net_ip;
    net_ip = sock_addr_pton("192.168.1.123");
    printf("ip = %x\n", net_ip);
    sock_addr_ntop(str_ip, net_ip);
    printf("ip = %s\n", str_ip);
}

void domain_test()
{
    void *p;
    char str[SOCK_ADDR_LEN];
#if defined (OS_LINUX)
    sock_addr_list_t *tmp;
    if (0 == sock_get_local_list(&tmp, 0)) {
        for (; tmp; tmp = tmp->next) {
            sock_addr_ntop(str, tmp->addr.ip);
            printf("ip = %s port = %d\n", str, tmp->addr.port);
        }
    }

    if (0 == sock_getaddrinfo(&tmp, "www.sina.com", "3478")) {
        for (; tmp; tmp = tmp->next) {
            sock_addr_ntop(str, tmp->addr.ip);
            printf("ip = %s port = %d\n", str, tmp->addr.port);
        }
    }
    if (0 == sock_gethostbyname(&tmp, "www.baidu.com")) {
        for (; tmp; tmp = tmp->next) {
            sock_addr_ntop(str, tmp->addr.ip);
            printf("ip = %s port = %d\n", str, tmp->addr.port);
        }
    }

    do {
        p = tmp;
        if (tmp) tmp = tmp->next;
        if (p) free(p);
    } while (tmp);
    #endif
}

void ctrl_c_op(int signo)
{
    if (g_sc) {
        sock_close(g_sc->fd);
    }
    exit(0);
}

int main(int argc, char **argv)
{
    uint16_t port;
    const char *ip;
    struct sock_server *ss;
    if (argc < 2) {
        usage();
        exit(0);
    }
#if defined (OS_LINUX)
    sock_get_local_info();
    signal(SIGINT, ctrl_c_op);
#endif
    if (!strcmp(argv[1], "-s")) {
        if (argc == 3)
            port = atoi(argv[2]);
        else
            port = 0;
        ss = sock_server_create(NULL, port, SOCK_TYPE_TCP);
        sock_server_set_callback(ss, on_connect_server, on_recv_buf, NULL);
        sock_server_dispatch(ss);
    } else if (!strcmp(argv[1], "-c")) {
        if (argc == 3) {
            ip = "127.0.0.1";
            port = atoi(argv[2]);
        } else if (argc == 4) {
            ip = argv[2];
            port = atoi(argv[3]);
        }
        tcp_client(ip, port);
        //udp_client(ip, port);
    }
    if (!strcmp(argv[1], "-t")) {
        addr_test();
    }
    if (!strcmp(argv[1], "-d")) {
        domain_test();
    }
    while (1) sleep(1);
    return 0;
}
