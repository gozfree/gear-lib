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
#ifndef LIBFSM_H
#define LIBFSM_H

#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *      |------|  event1 |------|
 *      |state1| ------> |state2|  
 *      |------| <------ |------|
 *        | /|\   event2   | /|\
 *  event3|  | event4      |  |
 *       \|/ |             |  |
 *      |------|   event5  |  | event6
 *      |state3|<-----------  |
 *      |------|--------------
 */

typedef int (*fsm_event_handle)(void *arg);
struct fsm_state;

struct fsm_event {
    int id;
    fsm_event_handle *handle;
    struct fsm_event *prev_state;
    struct fsm_event *next_state;
    void *arg;
};

struct fsm_state {
    int id;
    struct fsm_event *enter_event;
    struct fsm_event *leave_event;
    void *arg;
};

struct fsm_graph {
    struct fsm_state *state_list;
    struct fsm_event *event_list;

};

struct fsm {
    pthread_t tid;
    struct fsm_graph graph;
    bool is_running;
    bool is_exit;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    struct fsm_state *curr_state;

};

struct fsm *fsm_create();
void fsm_destroy(struct fsm *fsm);
int fsm_register(struct fsm *fsm, struct fsm_state *state, struct fsm_event *event);
int fsm_unregister(struct fsm *fsm, struct fsm_state *state, struct fsm_event *event);

int fsm_start(struct fsm *fsm);
int fsm_stop(struct fsm *fsm);
int fsm_state_init(struct fsm_state *state);

#ifdef __cplusplus
}
#endif
#endif
