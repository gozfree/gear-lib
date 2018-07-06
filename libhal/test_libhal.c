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
#include <stdio.h>
#include <stdlib.h>
#include "libhal.h"

int main(int argc, char **argv)
{
    struct network_info ni;
    struct cpu_info ci;
    network_get_info("lo", &ni);
    cpu_get_info(&ci);
    printf("%s\n", ni.ipaddr);
    printf("cores = %d, cores_available = %d\n", ci.cores, ci.cores_available);
    printf("features = %s\n", ci.features);
    printf("name = %s\n", ci.name);
    return 0;
}
