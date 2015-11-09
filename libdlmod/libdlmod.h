/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libdlmod.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-11-09 18:51
 * updated: 2015-11-09 18:51
 ******************************************************************************/
#ifndef _LIBDLMOD_H_
#define _LIBDLMOD_H_

#ifdef __cplusplus
extern "C" {
#endif

void *dl_load(const char *path_name);
void dl_unload(void *handle);



#ifdef __cplusplus
}
#endif
#endif
