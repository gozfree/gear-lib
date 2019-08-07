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
#include "libp2p.h"
#include <libmacro.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>


static const char *_rpc_ip = "192.168.1.211";
//static char *_rpc_ip = "180.153.102.147";
//static char *_rpc_ip = "116.228.149.106";

static const char *_stun_ip = "192.168.1.211";
//static char *_stun_ip = "180.153.102.147";
//static char *_stun_ip = "116.228.149.106";
static void *input_thread(void *arg)
{
    struct p2p *p2p = (struct p2p *)arg;
    uint32_t uuid_dst;
    int ret, i;
    int len = 1024;
    char *buf = (char *)calloc(1, len);
    for (i = 0; i < len; i++) {
        buf[i] = i;
    }
    p2p_get_peer_list(p2p);
    printf("input peer id> ");
    scanf("%x", &uuid_dst);
    p2p_connect(p2p, uuid_dst);
    while (1) {
        memset(buf, 0, len);
        printf("input message> ");
        scanf("%s", buf);
        usleep(500*1000);
        //sprintf(buf, "%s", "abcd");
        ret = p2p_send(p2p, buf, strlen(buf));
        printf("p2p_send ret = %d, errno=%d\n", ret, errno);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t tid;
    struct p2p *p2p = p2p_init(_rpc_ip, _stun_ip);
    if (!p2p) {
        printf("p2p_init failed!\n");
        return -1;
    }
    printf("p2p id: %x\n", p2p->rpc->send_pkt.header.uuid_src);
    pthread_create(&tid, NULL, input_thread, p2p);
    p2p_dispatch(p2p);
    return 0;
}
