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
#include "libgevent.h"
#include <stdio.h>
#if defined (__linux__) || defined (__CYGWIN__)
#include <unistd.h>
#include <sys/sysinfo.h>
#endif
#include <signal.h>

struct gevent_base *evbase = NULL;

static void on_input(int fd, void *arg)
{
    char ch[2];
    read(fd, &ch, 2);
    printf("on_input fd = %d\n", fd);
}

static int foo(void)
{
    struct gevent *event_2000;
    struct gevent *event_1500;
    struct gevent *event_stdin;
    evbase = gevent_base_create();
    if (!evbase) {
        printf("gevent_base_create failed!\n");
        return -1;
    }
    event_stdin = gevent_create(0, on_input, NULL, NULL, NULL);
    if (!event_stdin) {
        printf("gevent_create failed!\n");
        return -1;
    }
    event_2000 = gevent_timer_create(2000, TIMER_PERSIST, on_input, NULL);
    if (!event_2000) {
        printf("gevent_create failed!\n");
        return -1;
    }
    event_1500 = gevent_timer_create(1500, TIMER_PERSIST, on_input, NULL);
    if (!event_1500) {
        printf("gevent_create failed!\n");
        return -1;
    }
    if (-1 == gevent_add(evbase, event_stdin)) {
        printf("gevent_add failed!\n");
        return -1;
    }
    if (-1 == gevent_add(evbase, event_2000)) {
        printf("gevent_add failed!\n");
        return -1;
    }
    if (-1 == gevent_add(evbase, event_1500)) {
        printf("gevent_add failed!\n");
        return -1;
    }
    gevent_base_loop_start(evbase);
    sleep(30);
    gevent_base_loop_stop(evbase);
    gevent_del(evbase, event_1500);
    gevent_del(evbase, event_2000);
    gevent_base_destroy(evbase);

    return 0;
}

static void sigint_handler(int sig)
{
    gevent_base_loop_break(evbase);
}

void signal_init()
{
    //signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, sigint_handler);
}

int main(int argc, char **argv)
{
    signal_init();
    foo();
    printf("xxx\n");
    return 0;
}
