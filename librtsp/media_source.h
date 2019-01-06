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
#ifndef MEDIA_SOURCE_H
#define MEDIA_SOURCE_H
#include <stdint.h>
#include <stddef.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STREAM_NAME_LEN     (128)
#define DESCRIPTION_LEN     (128)
#define SDP_LEN_MAX         8192

typedef struct media_source {
    char name[STREAM_NAME_LEN];
    char info[DESCRIPTION_LEN];
    char sdp[SDP_LEN_MAX];
    struct timeval tm_create;
    int (*sdp_generate)(struct media_source *ms);
    int (*open)(struct media_source *ms, const char *uri);
    int (*read)(struct media_source *ms, void **data, size_t *len);
    int (*write)(struct media_source *ms, void *data, size_t len);
    void (*close)(struct media_source *ms);
    int (*get_frame)();
    void *opaque;
    struct media_source *next;
} media_source_t;

void media_source_register_all();
struct media_source *media_source_lookup(char *name);

#ifdef __cplusplus
}
#endif
#endif
