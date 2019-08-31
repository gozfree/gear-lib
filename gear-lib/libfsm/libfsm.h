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

struct fsm_event_table {
    int current_state;
    int trigger_event;
    int next_state;
    fsm_event_handle do_action;
};

struct fsm {
    int curr_state;
    struct fsm_event_table *table;
    int table_num;
    pthread_mutex_t mutex;
};

struct fsm *fsm_create();
void fsm_destroy(struct fsm *fsm);

int fsm_state_init(struct fsm *fsm, int state);
int fsm_action(struct fsm *fsm, int event_id, void *args);
int fsm_traval(struct fsm *fsm);

#ifdef __cplusplus
}
#endif
#endif
