/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libhash.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-05-10 00:42
 * updated: 2015-05-10 00:42
 ******************************************************************************/

#ifndef LIBHASH_H
#define LIBHASH_H

#include <libmacro.h>
#ifdef __cplusplus
extern "C" {
#endif

struct hash {
    char *name;
    struct hlist_head *list;
    int bucket_size;
    void (*destory)(void *value);
};

struct hash_entry {
    struct hlist_node entry;
    char *key;
    void *value;
};

void *hash_get(struct hash *dict, const char *key);

void hash_set(struct hash *dict, const char *key, void *value);

void hash_del(struct hash *dict, const char *key);

void *hash_get_and_del(struct hash *dict, const char *key);

struct hash *hash_create(int bucket_size);

void hash_destroy(struct hash *dict);

void hash_set_destory(struct hash *dict, void (*destory)(void *value));


#ifdef __cplusplus
}
#endif
#endif
