/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#include "libmedia-io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct media_packet *media_packet_create(enum media_type type, enum media_mem_type mem_type, void *data, size_t len)
{
    struct media_packet *mp = calloc(1, sizeof(struct media_packet));
    if (!mp) {
        return NULL;
    }
    mp->type = type;
    switch (mp->type) {
    case MEDIA_TYPE_AUDIO:
        mp->audio = audio_packet_create(mem_type, data, len);
        break;
    case MEDIA_TYPE_VIDEO:
        mp->video = video_packet_create(mem_type, data, len);
        break;
    default:
        printf("unsupport create %d media packet\n", mp->type);
        break;
    }
    return mp;
}

void media_packet_destroy(struct media_packet *mp)
{
    if (!mp) {
        return;
    }
    switch (mp->type) {
    case MEDIA_TYPE_AUDIO:
        audio_packet_destroy(mp->audio);
        break;
    case MEDIA_TYPE_VIDEO:
        video_packet_destroy(mp->video);
        break;
    default:
        printf("unsupport destroy %d media packet\n", mp->type);
        break;
    }
    free(mp);
}

struct media_packet *media_packet_copy(const struct media_packet *src, enum media_mem_type mem_type)
{
    struct media_packet *dst = NULL;
    if (!src)
        return NULL;

    switch (src->type) {
    case MEDIA_TYPE_VIDEO:
        dst = media_packet_create(MEDIA_TYPE_VIDEO, mem_type, NULL, 0);
        video_packet_copy(dst->video, src->video, mem_type);
        break;
    case MEDIA_TYPE_AUDIO:
        dst = media_packet_create(MEDIA_TYPE_AUDIO, mem_type, NULL, 0);
        audio_packet_copy(dst->audio, src->audio, mem_type);
        break;
    default:
        printf("unsupport copy %d media packet\n", src->type);
        break;
    }
    if (dst)
        dst->type = src->type;
    return dst;
}

size_t media_packet_get_size(struct media_packet *mp)
{
    switch (mp->type) {
    case MEDIA_TYPE_AUDIO:
        return mp->audio->size;
    case MEDIA_TYPE_VIDEO:
        return mp->video->size;
    default:
        return 0;
    }
    return 0;
}

void media_producer_dump_info(struct media_producer *mp)
{
    if (!mp) {
        printf("media producer is empty!\n");
        return;
    }
    switch (mp->type) {
    case MEDIA_TYPE_AUDIO:
        audio_producer_dump(&mp->audio);
        printf("audio unsupport type!\n");
        break;
    case MEDIA_TYPE_VIDEO:
        video_producer_dump(&mp->video);
        break;
    default:
        printf("dump unsupported media producer type!\n");
        break;
    }
}
void media_encoder_dump_info(struct media_encoder *me)
{
    if (!me) {
        printf("media encoder is empty!\n");
        return;
    }
    switch (me->type) {
    case MEDIA_TYPE_AUDIO:
        audio_encoder_dump(&me->audio);
        printf("audio unsupport type!\n");
        break;
    case MEDIA_TYPE_VIDEO:
        video_encoder_dump(&me->video);
        break;
    default:
        printf("dump unsupported media encoder type!\n");
        break;
    }
}
