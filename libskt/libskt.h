/******************************************************************************
 * Copyright (C) 2014-2018 Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/
#ifndef LIBSKT_H
#define LIBSKT_H

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#ifndef __ANDROID__
#include <ifaddrs.h>
#endif
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ADDR_STRING (65)

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
