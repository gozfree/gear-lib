/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libdebug.h
 * author:  gozfree <gozfree@163.com>
 * created: 2016-06-17 16:53:13
 * updated: 2016-06-17 16:53:13
 *****************************************************************************/
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


#ifdef __cplusplus
}
#endif
#endif
