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
#ifndef LIBHAL_H
#define LIBHAL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <linux/wireless.h>

#ifdef __cplusplus
extern "C" {
#endif

struct cpu_info {
    int cores;
    int cores_available;
    char name[128];
    char features[1024];
};

struct sdcard_info {
    bool is_insert;
    bool is_mounted;
    char name[64];
    char serial[64];
    char manfid[64];
    char oemid[64];
    char date[64];
    uint64_t used_size;
    uint64_t capacity;
};

struct network_info {
    bool is_probed;
    bool is_running;
    char ipaddr[16];
    char macaddr[32];
    char ssid[IW_ESSID_MAX_SIZE+1];
    char pswd[64];
};

struct network_ports {
    uint16_t tcp[8192];
    uint16_t tcp_cnt;
    uint16_t udp[8192];
    uint16_t udp_cnt;
};


int network_get_info(const char *interface, struct network_info *info);
int network_get_port_occupied(struct network_ports *ports);
int sdcard_get_info(const char *mount_point, struct sdcard_info *info);
int cpu_get_info(struct cpu_info *info);



#ifdef __cplusplus
}
#endif
#endif
