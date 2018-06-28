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
#ifndef LIBMP4PARSER_H
#define LIBMP4PARSER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mp4_parser {
    void *opaque_stream;
    void *opaque_root;
};

struct mp4_parser *mp4_parser_create(const char *file);
int mp4_get_duration(struct mp4_parser *mp, uint64_t *duration);
int mp4_get_creation(struct mp4_parser *mp, uint64_t *time);
int mp4_get_resolution(struct mp4_parser *mp, uint32_t *width, uint32_t *height);
void mp4_parser_destroy(struct mp4_parser *mp);

#ifdef __cplusplus
}
#endif
#endif
