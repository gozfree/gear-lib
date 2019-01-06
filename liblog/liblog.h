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
#ifndef LIBLOG_H
#define LIBLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#define	LOG_EMERG   0  /* system is unusable */
#define	LOG_ALERT   1  /* action must be taken immediately */
#define	LOG_CRIT    2  /* critical conditions */
#define	LOG_ERR     3  /* error conditions */
#define	LOG_WARNING 4  /* warning conditions */
#define	LOG_NOTICE  5  /* normal but significant condition */
#define	LOG_INFO    6  /* informational */
#define	LOG_DEBUG   7  /* debug-level messages */
#define	LOG_VERB    8  /* verbose messages */


typedef enum {
    LOG_STDIN   = 0, /*stdin*/
    LOG_STDOUT  = 1, /*stdout*/
    LOG_STDERR  = 2, /*stderr*/
    LOG_FILE    = 3,
    LOG_RSYSLOG = 4,

    LOG_MAX_OUTPUT = 255
} log_type_t;

int log_init(int type, const char *ident);
void log_deinit();

void log_set_level(int level);
void log_set_split_size(int size);
void log_set_rotate(int enable);
int log_set_path(const char *path);
int log_print(int lvl, const char *tag, const char *file, int line,
        const char *func, const char *fmt, ...);

#define LOG_LEVEL_ENV     "LIBLOG_LEVEL"
#define LOG_OUTPUT_ENV    "LIBLOG_OUTPUT"
#define LOG_TIMESTAMP_ENV "LIBLOG_TIMESTAMP"

#define LOG_TAG "tag"

#define loge(...) log_print(LOG_ERR, LOG_TAG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define logw(...) log_print(LOG_WARNING, LOG_TAG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define logi(...) log_print(LOG_INFO, LOG_TAG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define logd(...) log_print(LOG_DEBUG, LOG_TAG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define logv(...) log_print(LOG_VERB, LOG_TAG, __FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef __ANDROID__
#ifndef __ANDROID_APP__
//only for android apk debug
#include <jni.h>
#include <android/log.h>
#undef loge
#define loge(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#undef logw
#define logw(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#undef logi
#define logi(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#undef logd
#define logd(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#undef logv
#define logv(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#endif
#endif

#ifdef __cplusplus
}
#endif
#endif
