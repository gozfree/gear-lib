/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libptcp.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-10 00:15
 * updated: 2015-08-10 00:15
 *****************************************************************************/
#ifndef LIBPTCP_H
#define LIBPTCP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#ifndef __ANDROID__
#include <ifaddrs.h>
#endif
#include <sys/socket.h>
#include <net/if.h>

typedef struct _ptcp_socket ptcp_socket_t;


typedef enum {
    PSEUDO_TCP_DEBUG_NONE = 0,
    PSEUDO_TCP_DEBUG_NORMAL,
    PSEUDO_TCP_DEBUG_VERBOSE,
} ptcp_debug_level_t;

typedef enum {
    WR_SUCCESS,
    WR_TOO_LARGE,
    WR_FAIL
} ptcp_write_result_t;

typedef enum {
    PSEUDO_TCP_SHUTDOWN_RD,
    PSEUDO_TCP_SHUTDOWN_WR,
    PSEUDO_TCP_SHUTDOWN_RDWR,
} ptcp_shutdown_t;

typedef struct ptcp_callbacks {
    void *data;
    void (*on_opened)(ptcp_socket_t *p, void *data);
    void (*on_readable)(ptcp_socket_t *p, void *data);
    void (*on_writable)(ptcp_socket_t *p, void *data);
    void (*on_closed)(ptcp_socket_t *p, uint32_t error, void *data);
    ptcp_write_result_t (*write)(ptcp_socket_t *p, const char *buf, uint32_t len, void *data);
} ptcp_callbacks_t;


typedef enum {
    OPT_NODELAY,    // Whether to enable Nagle's algorithm (0 == off)
    OPT_ACKDELAY,   // The Delayed ACK timeout (0 == off).
    OPT_RCVBUF,     // Set the receive buffer size, in bytes.
    OPT_SNDBUF,     // Set the send buffer size, in bytes.
} ptcp_option_t;



ptcp_socket_t *ptcp_socket();
ptcp_socket_t *ptcp_socket_by_fd(int fd);

int ptcp_socket_fd(ptcp_socket_t *p);

int ptcp_bind(ptcp_socket_t *p, const struct sockaddr *addr,
                socklen_t addrlen);
int ptcp_listen(ptcp_socket_t *p, int backlog);
int ptcp_connect(ptcp_socket_t *p, const struct sockaddr *addr,
                   socklen_t addrlen);
int ptcp_recv(ptcp_socket_t *p, void *buf, size_t len);
int ptcp_send(ptcp_socket_t *p, const void *buf, size_t len);
void ptcp_close(ptcp_socket_t *p);
void ptcp_shutdown(ptcp_socket_t *p, ptcp_shutdown_t how);
int ptcp_get_error(ptcp_socket_t *p);
int ptcp_get_next_clock(ptcp_socket_t *p, uint64_t *timeout);
void ptcp_notify_clock(ptcp_socket_t *p);
void ptcp_notify_mtu(ptcp_socket_t *p, uint16_t mtu);
int ptcp_notify_packet(ptcp_socket_t *p, const char *buf, uint32_t len);
int ptcp_notify_message(ptcp_socket_t *p, void *msg);
void ptcp_set_debug_level(ptcp_debug_level_t level);
int ptcp_get_available_bytes(ptcp_socket_t *p);
int ptcp_can_send(ptcp_socket_t *p);
size_t ptcp_get_available_send_space(ptcp_socket_t *p);
void ptcp_set_time(ptcp_socket_t *p, uint32_t current_time);
int ptcp_is_closed(ptcp_socket_t *p);
int ptcp_is_closed_remotely(ptcp_socket_t *p);
void ptcp_get_option(ptcp_option_t opt, int *value);
void ptcp_set_option(ptcp_option_t opt, int value);


#ifdef __cplusplus
}
#endif
#endif
