/******************************************************************************
 * Copyright (C) 2014-2017 Zhifeng Gong <gozfree@163.com>
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
#ifndef LIBLOG_WIN_H
#define LIBLOG_WIN_H

#include <direct.h>
#include <winsock2.h>

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
    ssize_t len = 0;
    for (int i = 0; i < iovcnt; ++i) {
        len += write(fd, iov[i].iov_base, iov[i].iov_len);
    }
    return len;
}

#endif
