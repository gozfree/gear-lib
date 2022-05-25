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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "libptcp.h"

static int server(const char *host, uint16_t port)
{
    int len;
    struct sockaddr_in si;
    char buf[32] = {0};

    ptcp_socket_t *ps = ptcp_socket();
    if (ps == NULL) {
        printf("error!\n");
        return -1;
    }

    si.sin_family = AF_INET;
    si.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;
    si.sin_port = htons(port);

    ptcp_bind(ps, (struct sockaddr*)&si, sizeof(si));
    ptcp_listen(ps, 0);

    while (1) {
        memset(buf, 0, sizeof(buf));
        printf("before ptcp_recv\n");
        len = ptcp_recv(ps, buf, sizeof(buf));
        buf[sizeof(buf)-1] = '\0';
        if (len > 0) {
            printf("ptcp_recv len=%d, buf=%s\n", len, buf);
        } else if (ptcp_is_closed(ps)) {
            printf("ptcp is closed\n");
            return -1;
        } else if (EWOULDBLOCK == ptcp_get_error(ps)){
            printf("ptcp is error: %d\n", ptcp_get_error(ps));
            usleep(1000 * 1000);
        }
    }
    return 0;
}

static int client(const char *host, uint16_t port)
{
    int i, len;
    char buf[128] = {0};
    struct sockaddr_in si;

    ptcp_socket_t *ps = ptcp_socket();
    if (ps == NULL) {
        printf("error!\n");
        return -1;
    }

    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);
    if (0 != ptcp_connect(ps, (struct sockaddr*)&si, sizeof(si))) {
        printf("ptcp_connect failed!\n");
    } else {
        printf("ptcp_connect success\n");
    }

    //sleep(1);
    i = 1;
    for (i = 0; i < 100; i++) {
    //while (1) {
        usleep(10 * 1000);
        memset(buf, 0, sizeof(buf));
        snprintf(buf, sizeof(buf), "client %d\n", i);
        len = 0;
        ptcp_send(ps, buf, strlen(buf));
        printf("ptcp_send i=%d, len=%d, buf=%s\n", i, len, buf);
    }
    sleep(1);
    ptcp_close(ps);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("./test -c / -s\n");
        return 0;
    }
    if (!strcmp(argv[1], "-c"))
        client("127.0.0.1", 2333);
        //client("192.168.1.100", 2333);
    if (!strcmp(argv[1], "-s"))
        server("127.0.0.1", 2333);
    while (1) {
        sleep(1);
    }
    return 0;
}
