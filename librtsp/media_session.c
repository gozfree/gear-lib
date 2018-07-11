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

#include "media_session.h"
#include <libdict.h>
#include <libmacro.h>

void *media_session_pool_create()
{
    return (void *)dict_new();
}

void media_session_pool_destroy(void *pool)
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

struct media_session *media_session_new(void *pool, char *name, size_t size)
{
    struct media_session *s = CALLOC(1, struct media_session);
    snprintf(s->name, size, "%s", name);
    snprintf(s->description, sizeof(s->description), "%s", "Session streamd by ipcam");
    snprintf(s->info, sizeof(s->info), "%s", name);
    gettimeofday(&s->tm_create, NULL);
    
    dict_add((dict *)pool, name, (char *)s);
    return s;
}

void media_session_del(void *pool, char *name)
{
    dict_del((dict *)pool, name);
}

struct media_session *media_session_lookup(void *pool, char *name)
{
    return (struct media_session *)dict_get((dict *)pool, name, NULL);
}
