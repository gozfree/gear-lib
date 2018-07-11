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
#ifndef MEDIA_SESSION_H
#define MEDIA_SESSION_H
#include <stddef.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STREAM_NAME_LEN     (128)
#define DESCRIPTION_LEN     (128)

typedef struct media_session {
    char name[STREAM_NAME_LEN];
    char description[DESCRIPTION_LEN];
    char info[DESCRIPTION_LEN];
    struct timeval tm_create;
} media_session_t;

void *media_session_pool_create();
void media_session_pool_destroy(void *pool);

struct media_session *media_session_new(void *pool, char *name, size_t size);
void media_session_del(void *pool, char *name);
struct media_session *media_session_lookup(void *pool, char *name);

#ifdef __cplusplus
}
#endif
#endif
