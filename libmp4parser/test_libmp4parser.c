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
    struct mp4_parser *mp = mp4_parser_create(argv[1]);
    uint64_t duration = 0;
    uint32_t w, h;
    mp4_get_duration(mp, &duration);
    printf("duration = %lu\n", duration);
    mp4_get_resolution(mp, &w, &h);
    printf("resolution = %dx%d\n", (int)w, (int)h);
    mp4_parser_destroy(mp);
    return 0;
}


