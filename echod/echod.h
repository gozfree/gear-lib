/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    echod.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-07-22 01:11
 * updated: 2015-07-22 01:11
 *****************************************************************************/
#ifndef _RPCD_H_
#define _RPCD_H_

#include <libdict.h>
#include <libgevent.h>
#include <libworkq.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct echod {
    int listen_fd;
    struct gevent_base *evbase;
    dict *dict_uuid2fd;
    dict *dict_fd2rpc;
    struct workq *wq;

} echod_t;

#ifdef __cplusplus
}
#endif
#endif
