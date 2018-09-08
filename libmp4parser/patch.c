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
#ifdef MP4_VERBOSE
    case MSG_DGB:
        printf("debug: %s\n", buf);
        break;
    case MSG_WARN:
        printf("warn: %s\n", buf);
        break;
#endif
    case MSG_ERR:
        printf("err: %s\n", buf);
        break;
    default:
        break;
    }
}

int stream_Read(stream_t *s, void* buf, int size)
{
    return fread(buf, 1, size, s->fp);
}

uint64_t stream_Seek(stream_t *s, int64_t offset)
{
    return fseek(s->fp, offset, SEEK_SET);
}

int64_t stream_Tell(stream_t *s)
{
    return ftell(s->fp);
}

int stream_Peek(stream_t *s, const uint8_t **buf, int size)
{
    uint32_t offset = stream_Tell(s);
    *buf = (uint8_t *)calloc(1, size);
    s->priv_buf_num++;
    s->priv_buf = (void **)realloc(s->priv_buf, s->priv_buf_num * sizeof(uint8_t*));
    s->priv_buf[s->priv_buf_num-1] = (void *)*buf;
    int ret = stream_Read(s, (void *)*buf, size);
    stream_Seek(s, offset);
    return ret;
}

int64_t stream_Size(stream_t *s)
{
    long size;
    long tmp = ftell(s->fp);
    fseek(s->fp, 0L, SEEK_END);
    size = ftell(s->fp);
    fseek(s->fp, tmp, SEEK_SET);
    return (int64_t)size;
}

stream_t* create_file_stream(const char *filename)
{
    stream_t* s = (stream_t*)calloc(1, sizeof(stream_t));
    s->priv_buf_num = 0;
    s->priv_buf = (void **)calloc(1, sizeof(uint32_t));
    s->fp = fopen(filename, "rb");
    if (!s->fp) {
        printf("fopen %s failed!\n", filename);
        free(s);
        return NULL;
    }
    return s;
}

void destory_file_stream(stream_t* s)
{
    int i;
    fclose(s->fp);
    for (i = 0; i < s->priv_buf_num; i++) {
        free(s->priv_buf[i]);
    }
    free(s->priv_buf);
    free(s);
}
