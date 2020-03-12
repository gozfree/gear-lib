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

#include "media_source.h"
#include "sdp.h"
#include <gear-lib/libdict.h>
#include <gear-lib/libmacro.h>
#include <gear-lib/libatomic.h>
#include <strings.h>

#define REGISTER_MEDIA_SOURCE(x)                                               \
    {                                                                          \
        extern struct media_source media_source_##x;                           \
            rtsp_media_source_register(&media_source_##x);                     \
    }

static struct media_source *first_media_source = NULL;
static struct media_source **last_media_source = &first_media_source;
static int registered = 0;

void rtsp_media_source_register(struct media_source *ms)
{
    struct media_source **p = last_media_source;
    ms->next = NULL;
    while (*p || atomic_ptr_cas((void * volatile *)p, NULL, ms))
        p = &(*p)->next;
    last_media_source = &ms->next;
}

void media_source_register_all(void)
{
    if (registered)
        return;
    registered = 1;

    REGISTER_MEDIA_SOURCE(h264);
    REGISTER_MEDIA_SOURCE(uvc);
}

struct media_source *rtsp_media_source_lookup(char *name)
{
    struct media_source *p;
    for (p = first_media_source; p != NULL; p = p->next) {
        if (!strcasecmp(name, p->name))
            break;
    }
    return p;
}
