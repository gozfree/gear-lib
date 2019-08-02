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

int system_noblock(char **argv);
ssize_t system_with_result(const char *cmd, void *buf, size_t count);
ssize_t system_noblock_with_result(char **argv, void *buf, size_t count);




#ifdef __cplusplus
}
#endif
#endif
