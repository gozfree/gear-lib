/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libgzf.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-10-31 18:16
 * updated: 2015-10-31 18:16
 ******************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "libgzf.h"

uint32_t hash_murmur(char *key, size_t len)
{
    uint32_t h, k;
    uint8_t *data;
    uint32_t seed = 0x0badcafe;//magic number
    uint32_t m = 0x5bd1e995;//magic number
    uint32_t r = 24;//magic number

    if (!key || !len) {
        printf("%s: invalid paraments!\n", __func__);
        return 0;
    }

    h = seed ^ len;
    data = (uint8_t *)key;
    while (len >= 4) {
        k = *(uint32_t *)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }
    switch (len) {
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
