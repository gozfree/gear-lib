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
#include "libhal.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    struct network_ports ports;
    struct network_info ni;
    struct cpu_info ci;
    network_get_info("lo", &ni);
    cpu_get_info(&ci);
    printf("%s\n", ni.ipaddr);
    printf("cores = %d, cores_available = %d\n", ci.cores, ci.cores_available);
    printf("features = %s\n", ci.features);
    printf("name = %s\n", ci.name);
    network_get_port_occupied(&ports);
    for (int i = 0; i < ports.tcp_cnt; i++) {
        printf("tcp_ports = %d\n", ports.tcp[i]);
    }
    for (int i = 0; i < ports.udp_cnt; i++) {
        printf("udp_ports = %d\n", ports.udp[i]);
    }
    return 0;
}
