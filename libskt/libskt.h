/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libskt.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-05-03 18:27
 * updated: 2015-07-11 19:59
 *****************************************************************************/
#ifndef _LIBSKT_H_
#define _LIBSKT_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#ifndef __ANDROID__
#include <ifaddrs.h>
#endif
#include <netinet/in.h>
#include <netdb.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIBNAME         "libskt"
#define MAX_ADDR_STRING (65)

//socket structs

enum skt_connect_type {
    SKT_TCP = 0,
    SKT_UDP,
    SKT_UNIX,
};

typedef struct skt_addr {
    uint32_t ip;
    uint16_t port;
} skt_addr_t;

typedef struct skt_naddr {
    uint32_t ip;
    uint16_t port;
} skt_naddr_t;

typedef struct skt_paddr {
    char ip[INET_ADDRSTRLEN];
    uint16_t port;
} skt_saddr_t;

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
int skt_tcp_bind_listen(const char *host, uint16_t port, int reuse);
int skt_accept(int fd, uint32_t *ip, uint16_t *port);

//socket udp apis
struct skt_connection *skt_udp_connect(const char *host, uint16_t port);
int skt_udp_bind(const char *host, uint16_t port, int reuse);

//socket common apis
void skt_close(int fd);

int skt_send(int fd, const void *buf, size_t len);
int skt_sendto(int fd, const char *ip, uint16_t port, const void *buf, size_t len);
int skt_recv(int fd, void *buf, size_t len);
int skt_recvfrom(int fd, uint32_t *ip, uint16_t *port, void *buf, size_t len);

uint32_t skt_addr_pton(const char *ip);
int skt_addr_ntop(char *str, uint32_t ip);

int skt_set_noblk(int fd, int enable);
int skt_set_reuse(int fd, int enable);
int skt_set_tcp_keepalive(int fd, int enable);
int skt_set_buflen(int fd, int len);

int skt_get_local_list(struct skt_addr_list **list, int loopback);
int skt_gethostbyname(struct skt_addr_list **list, const char *name);
int skt_getaddrinfo(skt_addr_list_t **list, const char *domain, const char *port);
int skt_getaddr_by_fd(int fd, struct skt_addr *addr);
int skt_get_remote_addr(struct skt_addr *addr, int fd);

#ifdef __cplusplus
}
#endif
#endif
