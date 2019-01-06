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
#ifndef LIBDICT_H
#define LIBDICT_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _keypair_ {
    char *key;
    char *val;
    uint32_t hash;
} keypair;

typedef struct _dict_ {
    uint32_t fill;
    uint32_t used;
    uint32_t size;
    keypair *table;
} dict;

typedef struct _key_list_ {
    char *key;
    struct _key_list_ *next;
} key_list;


dict *dict_new(void);
void dict_free(dict *d);
int dict_add(dict *d, char *key, char *val);
int dict_del(dict *d, char * key);
char *dict_get(dict *d, char *key, char *defval);
int dict_enumerate(dict *d, int rank, char **key, char **val);
void dict_dump(dict *d, FILE *out);
void dict_get_key_list(dict *d, key_list **klist);

#ifdef __cplusplus
}
#endif
#endif
