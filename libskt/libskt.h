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
#ifndef LIBSKT_H
#define LIBSKT_H

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#if defined (__linux__) || defined (__CYGWIN__)
#include <unistd.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#elif defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
#include "libposix4win.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ADDR_STRING (65)
#define INET_ADDRSTRLEN 16

//socket structs

enum skt_connect_type {
    SKT_TCP = 0,
    SKT_UDP,
    SKT_UNIX,
};

typedef struct skt_addr {
    char ip_str[INET_ADDRSTRLEN];
    uint32_t ip;
    uint16_t port;
} skt_addr_t;

typedef struct skt_addr_list {
    skt_addr_t addr;
    struct skt_addr_list *next;
} skt_addr_list_t;

typedef struct skt_connection {
    int fd;
    int type;
    struct skt_addr local;
    struct skt_addr remote;
} skt_connection_t;

//socket tcp apis
struct skt_connection *skt_tcp_connect(const char *host, uint16_t port);
int skt_tcp_bind_listen(const char *host, uint16_t port);
int skt_accept(int fd, uint32_t *ip, uint16_t *port);

//socket udp apis
struct skt_connection *skt_udp_connect(const char *host, uint16_t port);
int skt_udp_bind(const char *host, uint16_t port);

//socket unix domain apis
struct skt_connection *skt_unix_connect(const char *host, uint16_t port);
int skt_unix_bind_listen(const char *host, uint16_t port);

//socket common apis
void skt_close(int fd);

int skt_send(int fd, const void *buf, size_t len);
int skt_sendto(int fd, const char *ip, uint16_t port,
                const void *buf, size_t len);
int skt_recv(int fd, void *buf, size_t len);
int skt_recvfrom(int fd, uint32_t *ip, uint16_t *port,
                void *buf, size_t len);

uint32_t skt_addr_pton(const char *ip);
int skt_addr_ntop(char *str, uint32_t ip);

int skt_set_noblk(int fd, int enable);
int skt_set_block(int fd);
int skt_set_nonblock(int fd);
int skt_set_reuse(int fd, int enable);
int skt_set_tcp_keepalive(int fd, int enable);
int skt_set_buflen(int fd, int len);

int skt_get_tcp_info(int fd, struct tcp_info *ti);
int skt_get_local_list(struct skt_addr_list **list, int loopback);
int skt_gethostbyname(struct skt_addr_list **list, const char *name);
int skt_getaddrinfo(skt_addr_list_t **list,
                const char *domain, const char *port);
int skt_getaddr_by_fd(int fd, struct skt_addr *addr);
int skt_get_remote_addr_by_fd(int fd, struct skt_addr *addr);
int skt_get_local_info(void);

#ifdef __cplusplus
}
#endif
#endif
