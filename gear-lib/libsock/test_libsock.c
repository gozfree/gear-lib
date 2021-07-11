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

static void on_connect_server(int fd, struct sock_connection *conn)
{
    printf("on_connect_server: fd=%d local=%s:%d, remote=%s:%d\n", conn->fd,
            conn->local.ip_str, conn->local.port,
            conn->remote.ip_str, conn->remote.port);
}

static void on_connect_client(int fd, struct sock_connection *conn)
{
    int ret=0;
    printf("on_connect_client: fd=%d local=%s:%d, remote=%s:%d\n", conn->fd,
            conn->local.ip_str, conn->local.port,
            conn->remote.ip_str, conn->remote.port);
#if defined (OS_LINUX)
    struct tcp_info tcpi;
    if (conn->type == SOCK_STREAM) {
        memset(&tcpi, 0, sizeof(tcpi));
        ret = sock_get_tcp_info(conn->fd, &tcpi);
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
    }
#endif
}

static void on_recv_buf(int fd, void *buf, size_t len)
{
    printf("%s:%d fd = %d, recv buf = %s\n", __func__, __LINE__, fd, (char *)buf);
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
    exit(0);
}

int main(int argc, char **argv)
{
    char buf[64];
    int n;
    uint16_t port;
    const char *ip;
    struct sock_server *ss;
    struct sock_client *sc;
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
    } else if (!strcmp(argv[1], "-S")) {
        if (argc == 3)
            port = atoi(argv[2]);
        else
            port = 0;
        ss = sock_server_create(NULL, port, SOCK_TYPE_PTCP);
        printf("sock_server_create PTCP success!\n");
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
        sc = sock_client_create(ip, port, SOCK_TYPE_TCP);
        sock_client_set_callback(sc, on_connect_client, on_recv_buf, NULL);
        sock_client_connect(sc);
        while (1) {
            memset(buf, 0, sizeof(buf));
            printf("input> ");
            scanf("%s", buf);
            printf("%s:%d fd=%d\n", __func__, __LINE__, sc->fd);
            n = sock_send(sc->fd, buf, strlen(buf));
            if (n == -1) {
                printf("sock_send failed!\n");
                return -1;
            }
        }
    } else if (!strcmp(argv[1], "-C")) {
        if (argc == 3) {
            ip = "127.0.0.1";
            port = atoi(argv[2]);
        } else if (argc == 4) {
            ip = argv[2];
            port = atoi(argv[3]);
        }
        sc = sock_client_create(ip, port, SOCK_TYPE_PTCP);
        sock_client_set_callback(sc, on_connect_client, on_recv_buf, NULL);
        sock_client_connect(sc);
        while (1) {
            memset(buf, 0, sizeof(buf));
            printf("input> ");
            scanf("%s", buf);
            n = sock_send(sc->conn->fd64, buf, strlen(buf));
            if (n == -1) {
                printf("sock_send failed!\n");
                return -1;
            }
        }
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
