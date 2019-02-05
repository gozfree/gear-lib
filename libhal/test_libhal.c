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
#include "libhal.h"
#include <stdio.h>
#include <stdlib.h>

void foo2()
{
    char *cmd = "date";
    char buf[64];
    system_with_result(cmd, buf, sizeof(buf));
    printf("buf = %s\n", buf);
}
int main(int argc, char **argv)
{
    struct network_ports ports;
    struct network_info ni;
    struct cpu_info ci;
    foo2();
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
