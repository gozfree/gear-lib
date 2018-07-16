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
#include "librtp.h"
#include <libskt.h>
#include <string.h>
#include <time.h>
#include <libmacro.h>

static uint16_t g_base_port = 20000;//must even data

struct rtp_socket *rtp_socket_create(enum rtp_mode mode, int tcp_fd, const char* src_ip)
{
    struct rtp_socket *s = CALLOC(1, struct rtp_socket);
    if (!s) {
        return NULL;
    }
    unsigned short i;
    switch (mode) {
    case RTP_TCP:
        s->tcp_fd = tcp_fd;
        break;
    case RTP_UDP:
        srand((unsigned int)time(NULL));
        do {
            i = rand() % 30000;
            i = i/2*2 + g_base_port;

            if (-1 == (s->rtp_fd = skt_udp_bind(src_ip, i))) {
                continue;
            }

            if (-1 == (s->rtcp_fd = skt_udp_bind(src_ip, i+1))) {
                skt_close(s->rtp_fd);
                continue;
            }
            s->rtp_port = i;
            s->rtcp_port = i+1;
            strcpy(s->ip, src_ip);
            printf("rtp_port = %d, rtcp_port = %d\n", s->rtp_port, s->rtcp_port);
            break;
        } while(1);
        break;
    case RAW_UDP:
    default:
        return NULL;
        break;
    }

    return s;
}

void rtp_socket_destroy(struct rtp_socket *s)
{
    if (s) {
        free(s);
    }
}

ssize_t rtp_sendto(struct rtp_socket *s, const char *ip, uint16_t port, const void *buf, size_t len)
{
    return skt_sendto(s->rtp_fd, ip, port, buf, len);
}

ssize_t rtp_recvfrom(struct rtp_socket *s, uint32_t *ip, uint16_t *port, void *buf, size_t len)
{
    return skt_recvfrom(s->rtp_fd, ip, port, buf, len);
}

ssize_t rtcp_sendto(struct rtp_socket *s, const char *ip, uint16_t port, const void *buf, size_t len)
{
    return skt_sendto(s->rtcp_fd, ip, port, buf, len);
}

ssize_t rtcp_recvfrom(struct rtp_socket *s, uint32_t *ip, uint16_t *port, void *buf, size_t len)
{
    return skt_recvfrom(s->rtp_fd, ip, port, buf, len);
}

