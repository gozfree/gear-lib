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
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <assert.h>
#include "patch.h"

bool decodeQtLanguageCode( uint16_t i_language_code, char *psz_iso,
                                  bool *b_mactables )
{
    static const char * psz_qt_to_iso639_2T_lower =
            "eng"    "fra"    "deu"    "ita"    "nld"
            "swe"    "spa"    "dan"    "por"    "nor" /* 5-9 */
            "heb"    "jpn"    "ara"    "fin"    "gre"
            "isl"    "mlt"    "tur"    "hrv"    "zho" /* 15-19 */
            "urd"    "hin"    "tha"    "kor"    "lit"
            "pol"    "hun"    "est"    "lav"    "sme" /* 25-29 */
            "fao"    "fas"    "rus"    "zho"    "nld" /* nld==flemish */
            "gle"    "sqi"    "ron"    "ces"    "slk" /* 35-39 */
            "slv"    "yid"    "srp"    "mkd"    "bul"
            "ukr"    "bel"    "uzb"    "kaz"    "aze" /* 45-49 */
            "aze"    "hye"    "kat"    "mol"    "kir"
            "tgk"    "tuk"    "mon"    "mon"    "pus" /* 55-59 */
            "kur"    "kas"    "snd"    "bod"    "nep"
            "san"    "mar"    "ben"    "asm"    "guj" /* 65-69 */
            "pan"    "ori"    "mal"    "kan"    "tam"
            "tel"    "sin"    "mya"    "khm"    "lao" /* 75-79 */
            "vie"    "ind"    "tgl"    "msa"    "msa"
            "amh"    "tir"    "orm"    "som"    "swa" /* 85-89 */
            "kin"    "run"    "nya"    "mlg"    "epo" /* 90-94 */
            ;

    static const char * psz_qt_to_iso639_2T_upper =
            "cym"    "eus"    "cat"    "lat"    "que" /* 128-132 */
            "grn"    "aym"    "tat"    "uig"    "dzo"
            "jaw"    "sun"    "glg"    "afr"    "bre" /* 138-142 */
            "iku"    "gla"    "glv"    "gle"    "ton"
            "gre"                                     /* 148 */
            ;
    unsigned i;
    if ( i_language_code < 0x400 || i_language_code == 0x7FFF )
    {
        const void *p_data;
        *b_mactables = true;
        if ( i_language_code <= 94 )
        {
            p_data = psz_qt_to_iso639_2T_lower + i_language_code * 3;
        }
        else if ( i_language_code >= 128 && i_language_code <= 148 )
        {
            i_language_code -= 128;
            p_data = psz_qt_to_iso639_2T_upper + i_language_code * 3;
        }
        else
            return false;
        memcpy( psz_iso, p_data, 3 );
    }
    else
    {
        *b_mactables = false;
        /*
         * to build code: ( ( 'f' - 0x60 ) << 10 ) | ( ('r' - 0x60) << 5 ) | ('a' - 0x60)
         */
        if( i_language_code == 0x55C4 ) /* "und" */
        {
            memset( psz_iso, 0, 3 );
            return false;
        }

        for( i = 0; i < 3; i++ )
            psz_iso[i] = ( ( i_language_code >> ( (2-i)*5 ) )&0x1f ) + 0x60;
    }
    return true;
}

void msg_log(int log_lvl, const char *fmt, ...)
{
    va_list ap;
    char buf[512] = {0};

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    switch (log_lvl) {
    case MSG_DGB:
        //printf("debug: %s\n", buf);
        break;
    case MSG_WARN:
        //printf("warn: %s\n", buf);
        break;
    case MSG_ERR:
        printf("err: %s\n", buf);
        break;
    default:
        break;
    }
}

static void* file_open(stream_t *stream_s, const char* filename, int mode)
{
    FILE* file = NULL;
    const char* mode_fopen = NULL;
    if ((mode & MODE_READWRITEFILTER) == MODE_READ) {
        mode_fopen = "rb";
    } else if (mode & MODE_EXISTING) {
        mode_fopen = "r+b";
    } else if (mode & MODE_CREATE) {
        mode_fopen = "wb";
    }
    if ((filename != NULL) && (mode_fopen != NULL)) {
        file = fopen(filename, mode_fopen);
    }
    stream_s->opaque = (void*)file;
    return file;
}

static int file_read(stream_t *stream_s, void* buf, int size)
{
    FILE* file = (FILE*)stream_s->opaque;
    return fread(buf, 1, size, file);
}

static int file_write(stream_t *stream_s, void *buf, int size)
{
    FILE* file = (FILE*)stream_s->opaque;
    return fwrite(buf, 1, size, file);
}

static uint64_t file_seek(stream_t *stream_s, int64_t offset, int whence)
{
    FILE* file = (FILE*)stream_s->opaque;
    return fseek(file, offset, whence);
}

static int64_t file_tell(stream_t *stream_s)
{
    FILE* file = (FILE*)stream_s->opaque;
    return ftell(file);
}

static int file_peek(stream_t *stream_s, const uint8_t **buf, int size)
{
    uint32_t offset = file_tell(stream_s);
    *buf = (uint8_t *)calloc(1, size);
    stream_s->priv_buf_num++;
    stream_s->priv_buf = (void **)realloc(stream_s->priv_buf, stream_s->priv_buf_num * sizeof(uint8_t*));
    stream_s->priv_buf[stream_s->priv_buf_num-1] = (void *)*buf;
    int ret = file_read(stream_s, (void *)*buf, size);
    file_seek(stream_s, offset, SEEK_SET);
    return ret;
}

static int64_t file_size(stream_t *stream_s)
{
    FILE* file = (FILE*)stream_s->opaque;
    long size;
    long tmp = ftell(file);
    fseek(file, 0L, SEEK_END);
    size = ftell(file);
    fseek(file, tmp, SEEK_SET);
    return (size_t)size;
}

static int file_close(stream_t *stream_s)
{
    FILE* file = (FILE*)stream_s->opaque;
    return fclose(file);
}

stream_t* create_file_stream()
{
    stream_t* s = (stream_t*)calloc(1, sizeof(stream_t));
    s->open = file_open;
    s->read = file_read;
    s->write = file_write;
    s->peek = file_peek;
    s->seek = file_seek;
    s->tell = file_tell;
    s->size = file_size;
    s->close = file_close;
    s->priv_buf_num = 0;
    s->priv_buf = (void **)calloc(1, sizeof(uint32_t));
    return s;
}

void destory_file_stream(stream_t* stream_s)
{
    int i;
    for (i = 0; i < stream_s->priv_buf_num; i++) {
        free(stream_s->priv_buf[i]);
    }
    free(stream_s->priv_buf);
    free(stream_s);
}



