/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libhash.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-05-10 00:42
 * updated: 2015-05-10 00:42
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "libhash.h"

//#define key_fn  key_fn_dobbs
#define key_fn  key_fn_murmur

#if 0
static uint32_t key_fn_dobbs(const char *key, size_t len)
{
    uint32_t hash, i;
    for(hash = i = 0; i < len; ++i)
    {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}
#endif

static uint32_t key_fn_murmur(const char *key, size_t len)
{
    uint32_t h, k, seed;
    uint32_t m = 0x5bd1e995;
    int         r = 24;
    unsigned char * data;
    seed = 0x0badcafe;
    h = seed ^ len;
    data = (unsigned char *)key;
    while(len >= 4) {
        k = *(unsigned int *)data;
        k *= m;
        k ^= k >> r;
        k *= m;
        h *= m;
        h ^= k;
        data += 4;
        len -= 4;
    }
    switch(len) {
        case 3: h ^= data[2] << 16;
        case 2: h ^= data[1] << 8;
        case 1: h ^= data[0];
                h *= m;
    };
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
    return h;
}

struct hash *hash_create(int bucket_size)
{
    int i;
    struct hash *dict = (struct hash *)calloc(1, sizeof(*dict));
    if (!dict) {
        return NULL;
    }
    //dict->list = (struct hlist_head *)malloc(bucket_size * sizeof(struct hlist_head));
    dict->list = (struct hlist_head *)calloc(bucket_size, sizeof(struct hlist_head));
    if (!dict->list) {
        return NULL;
    }
    dict->bucket_size = bucket_size;
    for (i = 0; i < bucket_size; i++) {
        INIT_HLIST_HEAD(&dict->list[i]);
    }
    return dict;
}

void hash_destroy(struct hash *dict)
{
    int i;
    struct hash_entry *entry;
    struct hlist_node *next;
    if (dict) {
        for (i = 0; i < dict->bucket_size; i++) {
            hlist_for_each_entry_safe(entry, next, &dict->list[i], entry) {
                hlist_del((struct hlist_node *)entry);
                free(entry->key);
                if (dict->destory) {
                    dict->destory(entry->value);
                }
                free(entry);
            }
        }
        free(dict->list);
        free(dict);
    }
}

void hash_set_destory(struct hash *dict, void (*destory)(void *value))
{
    dict->destory = destory;
}

void *hash_get(struct hash *dict, const char *key)
{
    int bucket = key_fn(key, strlen(key)) % dict->bucket_size;
    struct hash_entry *entry;
    hlist_for_each_entry(entry, &dict->list[bucket], entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
    }

    return NULL;
}

void hash_set(struct hash *dict, const char *key, void *value)
{
    int bucket = key_fn(key, strlen(key)) % dict->bucket_size;
    struct hash_entry *entry;

    hlist_for_each_entry(entry, &dict->list[bucket], entry) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = value;
            return;
        }
    }
    entry = (struct hash_entry *)calloc(1, sizeof(*entry));

    entry->key = strdup(key);
    entry->value = value;
    hlist_add_head(&entry->entry, &dict->list[bucket]);
    return;
}

void hash_del(struct hash *dict, const char *key)
{
    int bucket = key_fn(key, strlen(key)) % dict->bucket_size;
    struct hash_entry *entry;

    hlist_for_each_entry(entry, &dict->list[bucket], entry) {
        if (strcmp(entry->key, key) == 0) {
            hlist_del((struct hlist_node *)entry);
            free(entry->key);
            free(entry);
            return;
        }
    }
}

void *hash_get_and_del(struct hash *dict, const char *key)
{
    int bucket = key_fn(key, strlen(key)) % dict->bucket_size;
    struct hash_entry *entry;
    struct hlist_node *next;
    void *ret = NULL;
    hlist_for_each_entry_safe(entry, next, &dict->list[bucket], entry) {
        if (strcmp(entry->key, key) == 0) {
            hlist_del((struct hlist_node *)entry);
            free(entry->key);
            ret = entry->value;
            free(entry);
            return ret;
        }
    }

    return NULL;
}

