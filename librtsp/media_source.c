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

#include "media_source.h"
#include "sdp.h"
#include <libdict.h>
#include <libmacro.h>

void *media_source_pool_create()
{
    return (void *)dict_new();
}

void media_source_pool_destroy(void *pool)
{
    int rank = 0;
    char *key, *val;
    while (1) {
        rank = dict_enumerate((dict *)pool, rank, &key, &val);
        if (rank < 0) {
            break;
        }
        free(val);
    }
    dict_free((dict *)pool);
}

struct media_source *media_source_new(void *pool, char *name, size_t size)
{
    struct media_source *s = CALLOC(1, struct media_source);
    snprintf(s->name, size, "%s", name);
    snprintf(s->description, sizeof(s->description), "%s", "Session streamd by ipcam");
    snprintf(s->info, sizeof(s->info), "%s", name);
    gettimeofday(&s->tm_create, NULL);
    if (-1 == dict_add((dict *)pool, name, (char *)s)) {
        free(s);
        return NULL;
    }
    sdp_generate(s, name);
    return s;
}

void media_source_del(void *pool, char *name)
{
    dict_del((dict *)pool, name);
}

struct media_source *media_source_lookup(void *pool, char *name)
{
    return (struct media_source *)dict_get((dict *)pool, name, NULL);
}

void *client_session_pool_create()
{
    return (void *)dict_new();
}

void client_session_pool_destroy(void *pool)
{
    int rank = 0;
    char *key, *val;
    while (1) {
        rank = dict_enumerate((dict *)pool, rank, &key, &val);
        if (rank < 0) {
            break;
        }
        free(val);
    }
    dict_free((dict *)pool);
}

static uint32_t get_random_number()
{
    struct timeval now = {0};
    gettimeofday(&now, NULL);
    srand(now.tv_usec);
    return (rand() % ((uint32_t)-1));
}

struct client_session *client_session_new(void *pool)
{
    char key[9];
    struct client_session *s = CALLOC(1, struct client_session);
    s->session_id = get_random_number();
    snprintf(key, sizeof(key), "%08x", s->session_id);
    dict_add((dict *)pool, key, (char *)s);
    return s;
}

void client_session_del(void *pool, char *name)
{
    dict_del((dict *)pool, name);
}

struct client_session *client_session_lookup(void *pool, char *name)
{
    return (struct client_session *)dict_get((dict *)pool, name, NULL);
}

