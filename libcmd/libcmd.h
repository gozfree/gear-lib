/******************************************************************************
 * Copyright (C) 2014-2016
 * file:    libcmd.h
 * author:  gozfree <gozfree@163.com>
 * created: 2016-09-16 18:00
 * updated: 2016-09-16 18:00
 ******************************************************************************/
#ifndef LIBCMD_H
#define LIBCMD_H

#include <libdict.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cmd {
    int cnt;
    dict *dict;
} cmd_t;

typedef int (*cmd_cb)(int, char **);

struct cmd *cmd_init(int num);
void cmd_deinit(struct cmd *cmd);
int cmd_register(struct cmd *cmd, const char *name, cmd_cb func);
int cmd_execute(struct cmd *cmd, const char *name);
int cmd_get_registered(struct cmd *cmd);
void cmd_loop(struct cmd *cmd);

#ifdef __cplusplus
}
#endif
#endif
