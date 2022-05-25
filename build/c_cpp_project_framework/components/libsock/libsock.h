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
#ifndef LIBSOCK_H
#define LIBSOCK_H

#include <libposix.h>
#include <stdint.h>
#include <stdlib.h>
#if defined (OS_LINUX)
#include <netinet/in.h>
#include <netinet/tcp.h>
#elif defined (OS_RTOS)
#include <lwip/inet.h>
#include <lwip/netdb.h>
#else
//#define INET_ADDRSTRLEN 16
#endif

#ifdef ENABLE_PTCP
#include <libptcp.h>
#endif

#define LIBSOCK_VERSION "0.1.1"

#ifdef __cplusplus
extern "C" {
#endif

#define SOCK_ADDR_LEN   (INET_ADDRSTRLEN)

//socket structs

enum sock_type {
    SOCK_TYPE_UDP = 0,
    SOCK_TYPE_TCP,
    SOCK_TYPE_UNIX,
#ifdef ENABLE_PTCP
    SOCK_TYPE_PTCP,
#endif
    SOCK_TYPE_MAX,
};

typedef struct sock_addr {
    char ip_str[SOCK_ADDR_LEN];
    uint32_t ip;
    uint16_t port;
} sock_addr_t;

typedef struct sock_addr_list {
    sock_addr_t addr;
    struct sock_addr_list *next;
} sock_addr_list_t;

typedef struct sock_connection {
    int fd;
    uint64_t fd64;
    int type;
    struct sock_addr local;
    struct sock_addr remote;
} sock_connection_t;

//socket tcp apis
GEAR_API struct sock_connection *sock_tcp_connect(const char *host, uint16_t port);
GEAR_API int sock_tcp_bind_listen(const char *host, uint16_t port);
#ifdef ENABLE_PTCP
GEAR_API uint64_t sock_ptcp_bind_listen(const char *host, uint16_t port);
struct sock_connection *sock_ptcp_connect(const char *host, uint16_t port);
#endif
GEAR_API int sock_accept(uint64_t fd, uint32_t *ip, uint16_t *port);
GEAR_API struct sock_connection *sock_accept_connect(int fd);

//socket udp apis
struct sock_connection *sock_udp_connect(const char *host, uint16_t port);
int sock_udp_bind(const char *host, uint16_t port);

//socket unix domain apis
struct sock_connection *sock_unix_connect(const char *host, uint16_t port);
int sock_unix_bind_listen(const char *host, uint16_t port);

//socket common apis
void sock_close(int fd);

int sock_send(uint64_t fd, const void *buf, size_t len);
int sock_sendto(int fd, const char *ip, uint16_t port,
                const void *buf, size_t len);
int sock_send_sync_recv(int fd, const void *sbuf, size_t slen,
                void *rbuf, size_t rlen, int timeout);
int sock_recv(uint64_t fd, void *buf, size_t len);
int sock_recvfrom(int fd, uint32_t *ip, uint16_t *port, void *buf, size_t len);

uint32_t sock_addr_pton(const char *ip);
int sock_addr_ntop(char *str, uint32_t ip);

int sock_set_noblk(int fd, int enable);
int sock_set_block(int fd);
int sock_set_nonblock(int fd);
int sock_set_reuse(int fd, int enable);
int sock_set_tcp_keepalive(int fd, int enable);
int sock_set_buflen(int fd, int len);

#if defined (OS_LINUX)
int sock_get_tcp_info(int fd, struct tcp_info *ti);
int sock_get_local_info(void);
#endif
int sock_get_local_list(struct sock_addr_list **list, int loopback);
int sock_gethostbyname(struct sock_addr_list **list, const char *name);
int sock_getaddrinfo(sock_addr_list_t **list,
                const char *domain, const char *port);
int sock_getaddr_by_fd(int fd, struct sock_addr *addr);
int sock_get_remote_addr_by_fd(int fd, struct sock_addr *addr);

#ifdef __cplusplus
}
#endif
#endif
