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
#ifndef LIB_WIN_H
#define LIB_WIN_H
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <winsock2.h>

#define bool                int
#define false               0
#define true                1
#define inline              __inline
#define ssize_t             SSIZE_T
#define __func__            __FUNCTION__

#undef USE_SYSLOG

#undef PATH_SPLIT
#define PATH_SPLIT                '\\'
#define getcwd(buf, size)         GetModuleFileName(NULL, buf, size)
#define mkdir(path,mode)          _mkdir(path)
#define localtime_r(timep,result) localtime_s(result, timep)
#define _gettid(void)             GetCurrentProcessId()
#define iovec                     _WSABUF
#define iov_len                   len
#define iov_base                  buf

static ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
    int i;
    ssize_t len = 0;
    for (i = 0; i < iovcnt; ++i) {
        len += write(fd, iov[i].iov_base, iov[i].iov_len);
    }
    return len;
}

#endif
