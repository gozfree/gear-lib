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

#ifndef CONFIG_UTIL_H
#define CONFIG_UTIL_H

#define POINTER_ADDR 0x10000000

#define TYPE_EMPTY   0
#define TYPE_INT     1
#define TYPE_CHARP   2

struct int_charp {
    int type;
    union {
        int ival;
        char *cval;
    };
};

/*
 * va_arg_type can get value from ap ignore type
 * if the type is char *, it must be pointer of memory in higher address
 * when the type is int, it always smaller than pointer addr
 */
#define va_arg_type(ap, mix)                        \
    do {                                            \
        char *__tmp = va_arg(ap, char *);           \
        if (!__tmp) {                               \
            mix.type = TYPE_EMPTY;                  \
            mix.cval = NULL;                        \
        } else if (__tmp < (char *)POINTER_ADDR) {  \
            mix.type = TYPE_INT;                    \
            mix.ival = *(int *)&__tmp;              \
        } else {                                    \
            mix.type = TYPE_CHARP;                  \
            mix.cval = __tmp;                       \
        }                                           \
    } while (0)


#endif
