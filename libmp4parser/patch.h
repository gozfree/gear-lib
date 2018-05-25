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
#ifndef PATCH_H
#define PATCH_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define typeof __typeof__

typedef uint32_t vlc_fourcc_t;
#ifdef __GNUC__
#   define likely(p)   __builtin_expect(!!(p), 1)
#   define unlikely(p) __builtin_expect(!!(p), 0)
#else
#   define likely(p)   (!!(p))
#   define unlikely(p) (!!(p))
#endif

#define min(a, b)           ((a) > (b) ? (b) : (a))
#define max(a, b)           ((a) > (b) ? (a) : (b))

#define FREENULL(a) do { if (a) {free( a ); a = NULL;} } while(0)

void msg_log(int log_lvl, const char *fmt, ...);
#define MSG_DGB  1
#define MSG_WARN 2
#define MSG_ERR  3

#define msg_Dbg(fmt, ...) msg_log(MSG_DGB, __VA_ARGS__)
#define msg_Warn(fmt, ...) msg_log(MSG_WARN, __VA_ARGS__)
#define msg_Err(fmt, ...) msg_log(MSG_ERR, __VA_ARGS__)

#define VLC_UNUSED(x) (void)(x)
#   define __MIN(a, b)   ( ((a) < (b)) ? (a) : (b) )
#   define __MAX(a, b)   ( ((a) > (b)) ? (a) : (b) )


#ifdef __GNUC__
# define VLC_GCC_VERSION(maj,min) \
    ((__GNUC__ > (maj)) || (__GNUC__ == (maj) && __GNUC_MINOR__ >= (min)))
#else
# define VLC_GCC_VERSION(maj,min) (0)
#endif

static inline uint16_t bswap16 (uint16_t x)
{
    return (x << 8) | (x >> 8);
}

/** Byte swap (32 bits) */
static inline uint32_t bswap32 (uint32_t x)
{
#if VLC_GCC_VERSION(4,3) || defined(__clang__)
    return __builtin_bswap32 (x);
#else
    return ((x & 0x000000FF) << 24)
         | ((x & 0x0000FF00) <<  8)
         | ((x & 0x00FF0000) >>  8)
         | ((x & 0xFF000000) >> 24);
#endif
}

/** Byte swap (64 bits) */
static inline uint64_t bswap64 (uint64_t x)
{
#if VLC_GCC_VERSION(4,3) || defined(__clang__)
    return __builtin_bswap64 (x);
#elif !defined (__cplusplus)
    return ((x & 0x00000000000000FF) << 56)
         | ((x & 0x000000000000FF00) << 40)
         | ((x & 0x0000000000FF0000) << 24)
         | ((x & 0x00000000FF000000) <<  8)
         | ((x & 0x000000FF00000000) >>  8)
         | ((x & 0x0000FF0000000000) >> 24)
         | ((x & 0x00FF000000000000) >> 40)
         | ((x & 0xFF00000000000000) >> 56);
#else
    return ((x & 0x00000000000000FFULL) << 56)
         | ((x & 0x000000000000FF00ULL) << 40)
         | ((x & 0x0000000000FF0000ULL) << 24)
         | ((x & 0x00000000FF000000ULL) <<  8)
         | ((x & 0x000000FF00000000ULL) >>  8)
         | ((x & 0x0000FF0000000000ULL) >> 24)
         | ((x & 0x00FF000000000000ULL) >> 40)
         | ((x & 0xFF00000000000000ULL) >> 56);
#endif
}

#ifdef WORDS_BIGENDIAN
# define hton16(i) ((uint16_t)(i))
# define hton32(i) ((uint32_t)(i))
# define hton64(i) ((uint64_t)(i))
#else
# define hton16(i) bswap16(i)
# define hton32(i) bswap32(i)
# define hton64(i) bswap64(i)
#endif
#define ntoh16(i) hton16(i)
#define ntoh32(i) hton32(i)
#define ntoh64(i) hton64(i)

/* define from vlc */
#ifdef WORDS_BIGENDIAN
#   define VLC_FOURCC( a, b, c, d ) \
        ( ((uint32_t)d) | ( ((uint32_t)c) << 8 ) \
           | ( ((uint32_t)b) << 16 ) | ( ((uint32_t)a) << 24 ) )
#   define VLC_TWOCC( a, b ) \
        ( (uint16_t)(b) | ( (uint16_t)(a) << 8 ) )

#else
#   define VLC_FOURCC( a, b, c, d ) \
        ( ((uint32_t)a) | ( ((uint32_t)b) << 8 ) \
           | ( ((uint32_t)c) << 16 ) | ( ((uint32_t)d) << 24 ) )
#   define VLC_TWOCC( a, b ) \
        ( (uint16_t)(a) | ( (uint16_t)(b) << 8 ) )

#endif



/** Reads 16 bits in network byte order */
static inline uint16_t U16_AT (const void *p)
{
    uint16_t x;

    memcpy (&x, p, sizeof (x));
    return ntoh16 (x);
}

/** Reads 32 bits in network byte order */
static inline uint32_t U32_AT (const void *p)
{
    uint32_t x;

    memcpy (&x, p, sizeof (x));
    return ntoh32 (x);
}

/** Reads 64 bits in network byte order */
static inline uint64_t U64_AT (const void *p)
{
    uint64_t x;

    memcpy (&x, p, sizeof (x));
    return ntoh64 (x);
}

#define GetWBE(p)  U16_AT(p)
#define GetDWBE(p) U32_AT(p)
#define GetQWBE(p) U64_AT(p)

/* implement stream  */

#define MODE_READ             (1)
#define MODE_WRITE            (2)
#define MODE_READWRITEFILTER  (3)
#define MODE_EXISTING         (4)
#define MODE_CREATE           (8)


typedef struct stream {
    void *(*open)(struct stream *stream_s, const char *filename, int mode);
    int (*read)(struct stream *stream_s, void *buf, int size);
    int (*write)(struct stream *stream_s, void *buf, int size);
    int (*peek)(struct stream *stream_s, const uint8_t **buf, int size);
    uint64_t (*seek)(struct stream *stream_s, int64_t offset, int whence);
    int64_t (*tell)(struct stream *stream_s);
    int64_t (*size)(struct stream *stream_s);
    int (*close)(struct stream *stream_s);
    void *opaque;
    void **priv_buf;//store peek malloc buffer
    int priv_buf_num;
} stream_t;

stream_t* create_file_stream();
void destory_file_stream(stream_t* stream_s);

#define stream_open(s, filename, mode) ((stream_t*)s)->open(((stream_t*)s), filename, mode)
#define stream_close(s) ((stream_t*)s)->close(((stream_t*)s))
#define stream_Read(s, buf, size) ((stream_t*)s)->read(((stream_t*)s), buf, size)
#define stream_write(s, buf, size) ((stream_t*)s)->write(((stream_t*)s), buf, size)
#define stream_Peek(s, buf, size) ((stream_t*)s)->peek(((stream_t*)s), buf, size)
#define stream_Seek(s, offset) ((stream_t*)s)->seek(((stream_t*)s), offset, SEEK_SET)
#define stream_Tell(s) ((stream_t*)s)->tell(((stream_t*)s))
#define stream_Size(s) ((stream_t*)s)->size(((stream_t*)s))

bool decodeQtLanguageCode( uint16_t i_language_code, char *psz_iso,
                                  bool *b_mactables );

#ifdef __cplusplus
}
#endif
#endif
