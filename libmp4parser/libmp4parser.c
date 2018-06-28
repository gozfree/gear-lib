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
#include "mp4parser_inner.h"
#include "patch.h"
#include "libmp4parser.h"

/* seconds from 1904-01-01 00:00:00 UTC to 1969-12-31 24:00:00 UTC */
#define TIME_OFFSET    2082844800ULL

struct mp4_parser *mp4_parser_create(const char *file)
{
    stream_t *stream = create_file_stream();
    if (stream_open(stream, file, MODE_READ) == 0) {
        printf("stream_open %s failed\n", file);
        return NULL;
    }
    struct mp4_parser *mp = (struct mp4_parser *)calloc(1, sizeof(struct mp4_parser));
    mp->opaque_root = (void *)MP4_BoxGetRoot(stream);
    mp->opaque_stream = (void *)stream;
    return mp;
}

void mp4_parser_destroy(struct mp4_parser *mp)
{
    if (!mp || !mp->opaque_root || !mp->opaque_stream) {
        return;
    }
    stream_t *stream = (stream_t *)mp->opaque_stream;
    MP4_Box_t *root = (MP4_Box_t *)mp->opaque_root;
    MP4_BoxFree(stream, root);
    stream_close(stream);
    destory_file_stream(stream);
    free(mp);
}

int mp4_get_duration(struct mp4_parser *mp, uint64_t *duration)
{
    MP4_Box_t *root = (MP4_Box_t *)mp->opaque_root;
    MP4_Box_t *p_box = MP4_BoxGet(root, "moov/mvhd");
    if (!p_box) {
        return -1;
    }
    *duration = (uint64_t)(p_box->data.p_mvhd->i_duration*1.0/p_box->data.p_mvhd->i_timescale*1.0);
    return 0;
}

int mp4_get_creation(struct mp4_parser *mp, uint64_t *time)
{
    MP4_Box_t *root = (MP4_Box_t *)mp->opaque_root;
    MP4_Box_t *p_box = MP4_BoxGet(root, "moov/mvhd");
    if (!p_box) {
        return -1;
    }
    *time = p_box->data.p_mvhd->i_creation_time - TIME_OFFSET;
    return 0;
}

int mp4_get_resolution(struct mp4_parser *mp, uint32_t *width, uint32_t *height)
{
    MP4_Box_t *root = (MP4_Box_t *)mp->opaque_root;
    MP4_Box_t *p_box = MP4_BoxGet(root, "moov/trak/tkhd");
    if (!p_box) {
        return -1;
    }
    *width = (uint32_t)(p_box->data.p_tkhd->i_width/BLOCK16x16);
    *height = (uint32_t)(p_box->data.p_tkhd->i_height/BLOCK16x16);
    return 0;
}
