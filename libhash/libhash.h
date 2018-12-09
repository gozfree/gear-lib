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
#ifndef LIBHASH_H
#define LIBHASH_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct hash {
    int bucket;
    void *opaque_list;
    void (*destory)(void *val);
};

struct hash *hash_create(int bucket);
void hash_destroy(struct hash *h);
void hash_set_destory(struct hash *h, void (*destory)(void *val));

uint32_t hash_gen32(const char *key, size_t len);
void *hash_get(struct hash *h, const char *key);
int hash_set(struct hash *h, const char *key, void *val);
int hash_del(struct hash *h, const char *key);
void *hash_get_and_del(struct hash *h, const char *key);

#ifdef __cplusplus
}
#endif
#endif
