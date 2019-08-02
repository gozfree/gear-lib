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
#include "mp4parser_inner.h"
#include "patch.h"
#include "libmp4parser.h"

/* seconds from 1904-01-01 00:00:00 UTC to 1969-12-31 24:00:00 UTC */
#define TIME_OFFSET    2082844800ULL

struct mp4_parser *mp4_parser_create(const char *file)
{
    stream_t *stream = create_file_stream(file);
    if (!stream) {
        printf("create_file_stream failed!\n");
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
