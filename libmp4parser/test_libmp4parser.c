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
#include "libmp4parser.h"

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Invalid argument, useage: \n mp4parser /path/to/mp4file \n");
        return -1;
    }

    stream_t *s = create_file_stream();
    if (stream_open(s, argv[1], MODE_READ) == 0) {
        printf("Can not open file\n");
        return -1;
    }

    MP4_Box_t *SearchResult = MP4_BoxGetRoot(s);
    printf("search result box is %c%c%c%c\n", SearchResult->i_type&0x000000ff,
                                             (SearchResult->i_type&0x0000ff00)>>8,
                                             (SearchResult->i_type&0x00ff0000)>>16,
                                             (SearchResult->i_type&0xff000000)>>24);
    MP4_Box_t *p_ftyp = MP4_BoxGet(SearchResult, "ftyp");
    printf("ftyp.major_brand is %s\n",(char *)&p_ftyp->data.p_ftyp->i_major_brand);
    MP4_BoxFree(s, SearchResult);
    stream_close(s);
    destory_file_stream(s);
    return 0;
}


