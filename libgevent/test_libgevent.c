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
#include "libgevent.h"
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/sysinfo.h>
#include <signal.h>

struct gevent_base *evbase = NULL;

static void on_input(int fd, void *arg)
{
    printf("input %d", fd);
}

static int foo(void)
{
    int fd = 0;
    evbase = gevent_base_create();
    if (!evbase) {
        printf("gevent_base_create failed!\n");
        return -1;
    }
    struct gevent *event = gevent_create(fd, on_input, NULL, NULL, NULL);
    if (!event) {
        printf("gevent_create failed!\n");
        return -1;
    }
    if (-1 == gevent_add(evbase, event)) {
        printf("gevent_add failed!\n");
        return -1;
    }
    gevent_base_loop(evbase);
    gevent_del(evbase, event);
    gevent_base_destroy(evbase);

    return 0;
}

static void sigint_handler(int sig)
{
    gevent_base_loop_break(evbase);
}

void signal_init()
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, sigint_handler);
}

int main(int argc, char **argv)
{
    signal_init();
    foo();
    printf("xxx\n");
    return 0;
}
