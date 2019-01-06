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
#ifndef LIBDEBUG_H
#define LIBDEBUG_H

#ifdef __cplusplus
extern "C" {
#endif


/*! Initialize backtrace handler, once "Segmentation fault" occured,
 * the backtrace info will be show like "(gdb) bt"
 *
 */
int debug_backtrace_init();


/*! backtrace dump, can be called everywhere in your code
 *
 */
void debug_backtrace_dump();


/*! help analysis signals
 *
 */
int debug_signals_init();

#ifdef __cplusplus
}
#endif
#endif
