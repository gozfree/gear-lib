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
#ifndef LIBSOCK_EXT_H
#define LIBSOCK_EXT_H

#include "libsock.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sock_server {
    int fd;
    uint64_t fd64;
    struct sock_connection *conn;
    enum sock_type type;
    struct gevent_base *evbase;
    void (*on_buffer)(struct sock_server *s, void *buf, size_t len);
    void (*on_connect)(struct sock_server *s, struct sock_connection *conn);
    void (*on_disconnect)(struct sock_server *s, struct sock_connection *conn);
    void *priv;
};

/*
 * socket server high-level API
 */
GEAR_API struct sock_server *sock_server_create(const char *host, uint16_t port, enum sock_type type);
GEAR_API int sock_server_set_callback(struct sock_server *s,
        void (*on_connect)(struct sock_server *s, struct sock_connection *conn),
        void (*on_buffer)(struct sock_server *s, void *buf, size_t len),
        void (*on_disconnect)(struct sock_server *s, struct sock_connection *conn));
GEAR_API int sock_server_dispatch(struct sock_server *s);
GEAR_API void sock_server_destroy(struct sock_server *s);

/*
 * socket client high-level API
 */
struct sock_client {
    int fd;
    const char *host;
    uint16_t port;
    struct sock_connection *conn;
    enum sock_type type;
    struct gevent_base *evbase;
    struct thread *thread;
    void (*on_buffer)(struct sock_client *c, void *buf, size_t len);
    void (*on_connect)(struct sock_client *c, struct sock_connection *conn);
    void (*on_disconnect)(struct sock_client *c, struct sock_connection *conn);
    void *priv;
};

GEAR_API struct sock_client *sock_client_create(const char *host, uint16_t port, enum sock_type type);
GEAR_API int sock_client_set_callback(struct sock_client *c,
        void (*on_connect)(struct sock_client *c, struct sock_connection *conn),
        void (*on_buffer)(struct sock_client *c, void *buf, size_t len),
        void (*on_disconnect)(struct sock_client *c, struct sock_connection *conn));
GEAR_API int sock_client_connect(struct sock_client *c);
GEAR_API int sock_client_disconnect(struct sock_client *c);
GEAR_API void sock_client_destroy(struct sock_client *c);


#ifdef __cplusplus
}
#endif
#endif
