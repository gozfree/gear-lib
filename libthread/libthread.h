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
#ifndef LIBTHREAD_H
#define LIBTHREAD_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <liblock.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct thread {
    pthread_t tid;
    pthread_attr_t attr;
    enum lock_type type;
    union {
        spin_lock_t spin;
        mutex_lock_t mutex;
        mutex_cond_t cond;
        sem_lock_t sem;
    } lock;
    bool run;
    void *(*func)(struct thread *, void *);
    void *arg;
} thread_t;

struct thread *thread_create(void *(*func)(struct thread *, void *), void *arg, ...);
void thread_destroy(struct thread *t);
void thread_info(struct thread *t);

int thread_lock(struct thread *t);
int thread_unlock(struct thread *t);

int thread_wait(struct thread *t, int64_t ms);
int thread_signal(struct thread *t);
int thread_signal_all(struct thread *t);


#ifdef __cplusplus
}
#endif
#endif
