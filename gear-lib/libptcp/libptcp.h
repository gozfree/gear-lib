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
#ifndef LIBPTCP_H
#define LIBPTCP_H

#include <stdio.h>
#include <arpa/inet.h>

#define LIBPTCP_VERSION "0.0.1"

#ifdef __cplusplus
extern "C" {
#endif

typedef int* ptcp_socket_t;

ptcp_socket_t ptcp_socket();
ptcp_socket_t ptcp_socket_by_fd(int fd);
int ptcp_get_socket_fd(ptcp_socket_t sock);
int ptcp_close(ptcp_socket_t sock);
int ptcp_close_by_fd(ptcp_socket_t sock, int fd);
int ptcp_bind(ptcp_socket_t sock, const struct sockaddr *sa, socklen_t addrlen);
int ptcp_listen(ptcp_socket_t sock, int backlog);
int ptcp_accept(ptcp_socket_t sock, struct sockaddr *addr, socklen_t *addrlen);
int ptcp_connect(ptcp_socket_t sock, const struct sockaddr *sa, socklen_t addrlen);
ssize_t ptcp_send(ptcp_socket_t sock, const void *buf, size_t len, int flags);
ssize_t ptcp_recv(ptcp_socket_t sock, void *buf, size_t len, int flags);

#ifdef __cplusplus
}
#endif
#endif
