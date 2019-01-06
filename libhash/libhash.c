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
#include "libhash.h"
#include <libmacro.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * [opaque_list]
 * [bucket0] -> item[1] -> item[2] -> ... -> item[m0]
 * [bucket1] -> item[1] -> item[2] -> ... -> item[m1]
 * [bucket2] -> item[1] -> item[2] -> ... -> item[m2]
 * ...
 * [bucketn] -> item[1] -> item[2] -> ... -> item[mn]
 * 
 */

struct hash_item {
    struct hlist_node item;
    uint32_t hash;
    char *key;
    void *val;
};

enum hash_fn_id {
    HASH_DOBBS,
    HASH_MURMUR,
    HASH_CITY,
    HASH_SPOOKY
};

typedef struct hash_fn_item {
    int id;
    uint32_t (*func)(const char *key, size_t len);
} hash_fn_item;

static uint32_t hash_dobbs(const char *key, size_t len)
{
//https://en.wikipedia.org/wiki/Jenkins_hash_function
    uint32_t hash = 0;
    size_t i = 0;
    for (i = 0; i < len; ++i) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

static uint32_t hash_murmur(const char *key, size_t len)
{
//https://en.wikipedia.org/wiki/MurmurHash from variety of dict
//https://github.com/ndevilla/dict
#define MURMUR_MAGIC_1  0x5bd1e995
#define MURMUR_MAGIC_2  0x0badcafe
    uint32_t h, k;
    unsigned char * data;
    h = MURMUR_MAGIC_2 ^ len;
    data = (uint8_t *)key;
    while(len >= 4) {
        k = *(uint32_t *)data;
        k *= MURMUR_MAGIC_1;
        k ^= k >> 24;
        k *= MURMUR_MAGIC_1;
        h *= MURMUR_MAGIC_1;
        h ^= k;
        data += 4;
        len -= 4;
    }
    switch (len) {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0];
            h *= MURMUR_MAGIC_1;
    }
    h ^= h >> 13;
    h *= MURMUR_MAGIC_1;
    h ^= h >> 15;
    return h;
}

static uint32_t hash_city(const char *key, size_t len)
{
//https://github.com/google/cityhash
//TODO
    printf("unsupport yet\n");
    return 0;
}

static uint32_t hash_spooky(const char *key, size_t len)
{
//http://burtleburtle.net/bob/hash/spooky.html
//TODO
    printf("unsupport yet\n");
    return 0;
}

static hash_fn_item hash_fn_table[] = {
    {HASH_DOBBS, &hash_dobbs},
    {HASH_MURMUR, &hash_murmur},
    {HASH_CITY, &hash_city},
    {HASH_SPOOKY, &hash_spooky},
};

uint32_t hash_gen32(const char *key, size_t len)
{
    return hash_fn_table[HASH_MURMUR].func(key, len);
}

static struct hash_item *hash_lookup(struct hash *h, const char *key, uint32_t *hash)
{
    *hash = hash_gen32(key, strlen(key));
    struct hlist_head *list;
    struct hash_item *hi;
    struct hlist_node *next;
    uint32_t i;
    i = *hash & (h->bucket-1);
    list = &((struct hlist_head *)h->opaque_list)[i];

    hlist_for_each_entry_safe(hi, next, list, item) {
        if ((hi->hash == *hash) && strcmp(hi->key, key) == 0) {
            return hi;
        }
    }
#if 0
    uint32_t perturb;
    for (perturb = *hash; ; perturb >>= 5) {
        i = (i<<2) + i + perturb + 1;
        i &= (h->bucket-1);
        list = &((struct hlist_head *)h->opaque_list)[i];
        hi = container_of(list->first, struct hash_item, item);
        if (!hi) {
            return NULL;
        }
        if ((hi->hash == *hash) && strcmp(hi->key, key) == 0) {
            return hi;
        }
    }
#endif

    return NULL;
}

struct hash *hash_create(int bucket)
{
    struct hash *h = (struct hash *)calloc(1, sizeof(*h));
    if (!h) {
        return NULL;
    }
    struct hlist_head *list = (struct hlist_head *)calloc(bucket, sizeof(struct hlist_head));
    if (!list) {
        free(h);
        return NULL;
    }
    h->bucket = bucket;
    for (int i = 0; i < bucket; i++) {
        INIT_HLIST_HEAD(&list[i]);
    }
    h->opaque_list = list;
    return h;
}

void hash_destroy(struct hash *h)
{
    if (!h) {
        return;
    }
    struct hlist_head *list = h->opaque_list;
    for (int i = 0; i < h->bucket; i++) {
        struct hash_item *hi;
        struct hlist_node *next;
        hlist_for_each_entry_safe(hi, next, &list[i], item) {
            hlist_del((struct hlist_node *)hi);
            free(hi->key);
            if (h->destory) {
                h->destory(hi->val);
            }
            free(hi);
        }
    }
    free(list);
    free(h);
}

void hash_set_destory(struct hash *h, void (*destory)(void *val))
{
    h->destory = destory;
}

void *hash_get(struct hash *h, const char *key)
{
    uint32_t hash = 0;
    struct hash_item *hi = hash_lookup(h, key, &hash);
    if (hi) {
        return hi->val;
    }
    return NULL;
}

int hash_set(struct hash *h, const char *key, void *val)
{
    uint32_t hash = 0;
    struct hlist_head *list = h->opaque_list;
    struct hash_item *hi = hash_lookup(h, key, &hash);
    if (hi) {
        hi->val = val;
        return 0;
    }

    hi = (struct hash_item *)calloc(1, sizeof(*hi));
    if (!hi) {
        printf("calloc hash_item failed!\n");
        return -1;
    }

    hi->key = strdup(key);
    hi->val = val;
    hi->hash = hash;
    INIT_HLIST_NODE(&hi->item);
    int pos = hash & (h->bucket-1);
    hlist_add_head(&hi->item, &list[pos]);
    return 0;
}

int hash_del(struct hash *h, const char *key)
{
    uint32_t hash = 0;
    struct hash_item *hi = hash_lookup(h, key, &hash);
    if (hi) {
        hlist_del((struct hlist_node *)hi);
        free(hi->key);
        free(hi);
        return 0;
    }

    return -1;
}

void *hash_get_and_del(struct hash *h, const char *key)
{
    uint32_t hash = 0;
    struct hash_item *hi = hash_lookup(h, key, &hash);
    if (hi) {
        hlist_del((struct hlist_node *)hi);
        free(hi->key);
        free(hi);
        return NULL;
    }
    return NULL;
}
