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
#include "libfsm.h"
#include <gear-lib/libgevent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static struct fsm *fsm = NULL;
struct gevent_base *evbase = NULL;

enum tcp_state {
    TCP_ESTABLISHED = 1,
    TCP_SYN_SENT,
    TCP_SYN_RECV,
    TCP_FIN_WAIT1,
    TCP_FIN_WAIT2,
    TCP_TIME_WAIT,
    TCP_CLOSE,
    TCP_CLOSE_WAIT,
    TCP_LAST_ACK,
    TCP_LISTEN,
    TCP_MAX_STATES
};

enum tcp_event {
    EV_SRV_LISTEN,
    EV_CLI_SEND_SYN,
    EV_CLI_RECV_SYN_SEND_SYN_ACK,
    EV_CLI_RECV_RST,
    EV_CLI_SEND_FIN,
    EV_SRV_RECV_SYN_SEND_SYN_ACK,
    EV_SRV_RECV_ACK,
    EV_CLI_SEND_FIN_AND_DISCONNECT,
    EV_CLI_RECV_ACK,
    EV_CLI_RECV_FIN,
    EV_CLI_RECV_FIN_SEND_ACK,
    EV_SRV_RECV_FIN,
    EV_SRV_RECV_FIN_SEND_ACK,
    EV_SRV_SEND_FIN,
    EV_DISCONNECT_TIMEOUT,
    EV_WAIT_2MSL,
};

static int do_fin_wait1(void *arg)
{
    printf("%s:%d xxxx\n", __func__, __LINE__);
    return 0;
}
static int do_close_wait(void *arg)
{
    printf("%s:%d xxxx\n", __func__, __LINE__);
    return 0;
}
static int do_establish(void *arg)
{
    printf("%s:%d xxxx\n", __func__, __LINE__);
    return 0;
}
static int do_close(void *arg)
{
    printf("%s:%d xxxx\n", __func__, __LINE__);
    return 0;
}
static int do_listen(void *arg)
{
    printf("%s:%d xxxx\n", __func__, __LINE__);
    return 0;
}
static int do_fin_wait2(void *arg)
{
    printf("%s:%d xxxx\n", __func__, __LINE__);
    return 0;
}
static int do_time_wait(void *arg)
{
    printf("%s:%d xxxx\n", __func__, __LINE__);
    return 0;
}
static int do_recv(void *arg)
{
    printf("%s:%d xxxx\n", __func__, __LINE__);
    return 0;
}
static int do_ack(void *arg)
{
    printf("%s:%d xxxx\n", __func__, __LINE__);
    return 0;
}

static struct fsm_event_table tcp_event_tbl[] = {
    {TCP_ESTABLISHED, EV_SRV_SEND_FIN, TCP_FIN_WAIT1, do_fin_wait1},
    {TCP_ESTABLISHED, EV_SRV_RECV_FIN_SEND_ACK, TCP_CLOSE_WAIT, do_close_wait},
    {TCP_SYN_SENT, EV_CLI_RECV_SYN_SEND_SYN_ACK, TCP_ESTABLISHED, do_establish},
    {TCP_SYN_SENT, EV_DISCONNECT_TIMEOUT, TCP_CLOSE, do_close},
    {TCP_SYN_RECV, EV_CLI_RECV_RST, TCP_LISTEN, do_listen},
    {TCP_SYN_RECV, EV_SRV_RECV_ACK, TCP_ESTABLISHED, do_establish},
    {TCP_SYN_RECV, EV_SRV_SEND_FIN, TCP_FIN_WAIT1, do_fin_wait1},
    {TCP_FIN_WAIT1, EV_SRV_RECV_ACK, TCP_FIN_WAIT2, do_fin_wait2},
    {TCP_FIN_WAIT2, EV_SRV_RECV_FIN_SEND_ACK, TCP_TIME_WAIT, do_time_wait},
    {TCP_TIME_WAIT, EV_WAIT_2MSL, TCP_CLOSE, do_close},
    {TCP_CLOSE, EV_SRV_LISTEN, TCP_LISTEN, do_listen},
    {TCP_CLOSE_WAIT, EV_CLI_SEND_FIN_AND_DISCONNECT, TCP_LAST_ACK, do_ack},
    {TCP_LAST_ACK, EV_CLI_RECV_ACK, TCP_CLOSE, do_close},
    {TCP_LISTEN, EV_SRV_RECV_FIN_SEND_ACK, TCP_SYN_RECV, do_recv},
};

int loop()
{
    gevent_base_loop(evbase);
    return 0;
}

static void ev_in(int fd, void *arg)
{
    int event_id;
    char c[8];
    struct fsm *fsm = (struct fsm *)arg;
    read(fd, &c, sizeof(c));
    event_id = c[0] - 'a';
    printf("event_id=%d\n", event_id);
    fsm_action(fsm, event_id, fsm);
}

int init()
{
    fsm = fsm_create();
    fsm->table = tcp_event_tbl;
    fsm->table_num = sizeof(tcp_event_tbl)/sizeof(struct fsm_event_table);
    printf("table_num = %d\n", fsm->table_num);
    fsm_state_init(fsm, TCP_CLOSE);
    evbase = gevent_base_create();
    struct gevent *ev = gevent_create(0, ev_in, NULL, NULL, fsm);
    gevent_add(evbase, ev);

    return 0;
}

int main(int argc, char **argv)
{
    init();
    loop();
    return 0;
}
