/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    patch.h
 * author:  gozfree <gozfree@163.com>
 * created: 2016-07-29 14:24
 * updated: 2016-07-29 14:24
 ******************************************************************************/
#ifndef __PATCH_H__
#define __pATCH_H__

#include <stdlib.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif



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
