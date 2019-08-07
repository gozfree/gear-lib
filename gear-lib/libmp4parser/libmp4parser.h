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
#ifndef LIBMP4PARSER_H
#define LIBMP4PARSER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mp4_parser {
    void *opaque_stream;
    void *opaque_root;
};

struct mp4_parser *mp4_parser_create(const char *file);
int mp4_get_duration(struct mp4_parser *mp, uint64_t *duration);
int mp4_get_creation(struct mp4_parser *mp, uint64_t *time);
int mp4_get_resolution(struct mp4_parser *mp, uint32_t *width, uint32_t *height);
void mp4_parser_destroy(struct mp4_parser *mp);

#ifdef __cplusplus
}
#endif
#endif
