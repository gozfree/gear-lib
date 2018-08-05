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

#include "media_source.h"
#include "sdp.h"
#include <libdict.h>
#include <libmacro.h>
#include <libatomic.h>
#include <strings.h>

#define REGISTER_MEDIA_SOURCE(x)                                               \
    {                                                                          \
        extern struct media_source media_source_##x;                           \
            media_source_register(&media_source_##x);                          \
    }

static struct media_source *first_media_source = NULL;
static struct media_source **last_media_source = &first_media_source;
static int registered = 0;

static void media_source_register(struct media_source *ms)
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

struct media_source *media_source_lookup(char *name)
{
    struct media_source *p;
    for (p = first_media_source; p != NULL; p = p->next) {
        if (!strcasecmp(name, p->name))
            break;
    }
    return p;
}
