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

#ifndef CONFIG_UTIL_H
#define CONFIG_UTIL_H


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
 * firstly, try to match "char *", which must be pointer of memory in higher address
 * if the type is "int", the value force to "char *" will be the real value
 * because the int value is the index of table in conf file, and should be limited.
 * so we use MAX_CONF_ENTRY to divide the type of "int" or "char *"
 */
#define MAX_CONF_ENTRY 4096
#define va_arg_type(ap, mix)                        \
    do {                                            \
        char *__tmp = va_arg(ap, char *);           \
        if (!__tmp) {                               \
            mix.type = TYPE_EMPTY;                  \
            mix.cval = NULL;                        \
        } else if (__tmp < (char *)MAX_CONF_ENTRY) {\
            mix.type = TYPE_INT;                    \
            mix.ival = *(int *)&__tmp;              \
        } else {                                    \
            mix.type = TYPE_CHARP;                  \
            mix.cval = __tmp;                       \
        }                                           \
    } while (0)


#endif
