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
#ifndef LIBSTUN_H
#define LIBSTUN_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t port;
    uint32_t addr;
} stun_addr;

int stun_init(const char *ip);
int stun_socket(const char *ip, uint16_t port, stun_addr *map);
int stun_nat_type();
void stun_keep_alive(int fd);


#ifdef __cplusplus
}
#endif
#endif
