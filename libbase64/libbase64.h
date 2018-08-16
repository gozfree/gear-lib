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
#ifndef LIBBASE64_H
#define LIBBASE64_H

#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif

size_t base64_encode(char* target, const void *source, size_t bytes);
size_t base64_encode_url(char* target, const void *source, size_t bytes);
size_t base64_decode(void* target, const char *source, size_t bytes);

size_t base16_encode(char* target, const void *source, size_t bytes);
size_t base16_decode(void* target, const char *source, size_t bytes);



#ifdef __cplusplus
}
#endif
#endif
