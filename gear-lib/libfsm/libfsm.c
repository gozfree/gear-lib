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
#include <stdio.h>
#include <stdlib.h>

static void *_fsm_loop(void *arg)
{
    struct fsm *fsm = (struct fsm *)arg;
    while (1) {
        if (!fsm->is_running) {
            pthread_mutex_lock(&fsm->mutex);
            pthread_cond_wait(&fsm->cond, &fsm->mutex);
            pthread_mutex_unlock(&fsm->mutex);
        } else if (fsm->is_exit) {
            break;
        } else {
        }
    }

    return NULL;
}

struct fsm *fsm_create()
{
    int ret;
    struct fsm *fsm = (struct fsm *)calloc(1, sizeof(struct fsm));
    if (!fsm) {
        printf("malloc failed!\n");
        return NULL;
    }
    fsm->is_running = false;
    fsm->is_exit = false;
    pthread_mutex_init(&fsm->mutex, NULL);
    pthread_cond_init(&fsm->cond, NULL);
    ret = pthread_create(&fsm->tid, NULL, _fsm_loop, fsm);
    if (ret) {
        printf("pthread_create fsm loop failed!\n");
    }

    return fsm;
}

void fsm_destroy(struct fsm *fsm)
{
    if (fsm) {
        free(fsm);
    }
}

struct fsm_state *fsm_state_create(int id, struct fsm_event *enter, struct fsm_event *leave, void *arg)
{
    struct fsm_state *state = (struct fsm_state *)calloc(1, sizeof(struct fsm_state));
    if (!state) {
        printf("malloc failed!\n");
        return NULL;
    }
    state->id = id;
    state->enter_event = enter;
    state->leave_event = leave;
    state->arg = arg;

    return state;
}

int fsm_register(struct fsm *fsm, struct fsm_state *state, struct fsm_event *event)
{
    if (!fsm || !state || !event) {
        return -1;
    }
    return 0;
}

int fsm_start(struct fsm *fsm)
{
    if (!fsm) {
        return -1;
    }
    fsm->is_running = true;
    fsm->is_exit = false;
    pthread_mutex_lock(&fsm->mutex);
    pthread_cond_signal(&fsm->cond);
    pthread_mutex_unlock(&fsm->mutex);
    return 0;
}

int fsm_stop(struct fsm *fsm)
{
    if (!fsm) {
        return -1;
    }
    fsm->is_running = false;
    fsm->is_exit = false;
    return 0;
}

int fsm_exit(struct fsm *fsm)
{
    if (!fsm) {
        return -1;
    }
    fsm->is_running = false;
    fsm->is_exit = true;
    return 0;
}
