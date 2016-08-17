/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libcmd.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-10-31 16:58
 * updated: 2015-10-31 16:58
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <libmacro.h>

#include "libcmd.h"

#define CMD_NAME            "libcmd_shell"
#define CMD_PROMPT          "libcmd_shell > "
#define CMD_MAX_ARGS        16      /* max number of command args */
#define CMD_MAX_NUM         1024

static struct cmd *g_cmd = NULL;
static char *g_line_read = NULL;

static int parse_line(const char *name, char *argv[])
{
    int nargs = 0;
    char last = 0;
    char *line = (char *)name;

    while (nargs < CMD_MAX_ARGS) {
        /* skip any white space */
        while ((*line == ' ') || (*line == '\t') || (*line == '-')) {
            last = *line;
            ++line;
        }
        if (*line == '\0') {    /* end of line, no more args    */
            argv[nargs] = NULL;
            return (nargs);
        }
        argv[nargs++] = line;   /* begin of argument string    */
        /* find end of string */
        while (*line && (*line != ' ') && (*line != '\t')) {
            if (*line == '-') {
                if (last != ' ') {
                    last = *line;
                    ++line;
                } else {
                    break;
                }
            } else {
                last = *line;
                ++line;
            }
        }
        if (*line == '\0') {    /* end of line, no more args    */
            argv[nargs] = NULL;
            return (nargs);
        }
        *line++ = '\0';         /* terminate current arg          */
    }
    printf("** Too many args (max. %d) **\n", CMD_MAX_ARGS);
    return (nargs);
}

static char *cmd_gen(const char *text, int state)
{
    static int len = 0;
    static int rank = 0;//must be static, search dict continuously
    char *key, *val;

    if (!state) {
        rank = 0;
        len = strlen(text);
    }
    while (1) {
        rank = dict_enumerate(g_cmd->dict, rank, &key, &val);
        if (rank < 0) {
            break;
        }
        if (strncmp(key, text, len) == 0) {
            return strdup(key);
        }
    }
    return NULL;
}

static char **cmd_completion(const char *text, int start, int end)
{
    char **matches = NULL;
    if (start == 0)
        matches = rl_completion_matches(text, cmd_gen);
    return matches;
}

static char *cmd_get_input(void)
{
    if (g_line_read) {
        free(g_line_read);
        g_line_read = NULL;
    }
    g_line_read = readline(CMD_PROMPT);//alloc string, need free
    if (g_line_read == NULL) {
        printf("readline null\n");
        return NULL;
    }
    add_history(g_line_read);
    return g_line_read;
}

struct cmd *cmd_init(int cnt)
{
    int res = 0;
    struct cmd *cmd = NULL;
    do {
        if (cnt < 0 || cnt > CMD_MAX_NUM) {
            printf("cmd num can't more than %d\n", CMD_MAX_NUM);
            res = -1;
            break;
        }
        cmd = CALLOC(1, struct cmd);
        if (!cmd) {
            printf("malloc cmd failed!\n");
            res = -1;
            break;
        }
        cmd->cnt = cnt;
        cmd->dict = dict_new();
        if (!cmd->dict) {
            printf("dict_new failed!\n");
            res = -1;
            break;
        }
        rl_readline_name = CMD_NAME;
        rl_attempted_completion_function = cmd_completion;
    } while (0);

    if (UNLIKELY(res != 0)) {
        if (cmd->dict) dict_free(cmd->dict);
        if (cmd) free(cmd);
    }
    g_cmd = cmd;
    return cmd;
}

void cmd_deinit(struct cmd *cmd)
{
    if (!cmd) return;
    if (cmd->dict) dict_free(cmd->dict);
    if (cmd) free(cmd);
}

int cmd_register(struct cmd *cmd, const char *name, cmd_cb func)
{
    if (!cmd || !name || !func) {
        printf("%s: invalid paraments!", __func__);
        return -1;
    }
    if (dict_add(cmd->dict, (char *)name, (char *)func) < 0) {
        printf("dict_add %s failed!\n", name);
        return -1;
    }
    return 0;
}

int cmd_execute(struct cmd *cmd, const char *name)
{
    char *argv[CMD_MAX_ARGS + 1];
    int argc;

    if (!cmd || !name) {
        printf("%s: invalid paraments!", __func__);
        return -1;
    }

    if ((argc = parse_line(name, argv)) == 0) {
        //printf("parse_line failed!\n");
        return -1;
    }
    cmd_cb func = (cmd_cb)dict_get(cmd->dict, argv[0], NULL);
    if (!func) {
        printf("dict_get %s failed!\n", argv[0]);
        return -1;
    }
    if ((func)(argc, argv) != 0) {
        printf("run do_cmd error\n");
    }
    return 0;
}

int cmd_get_registered(struct cmd *cmd)
{
    if (!cmd) {
        printf("%s: invalid paraments!", __func__);
        return -1;
    }
    int i = 0;
    int rank = 0;
    char *key, *val;
    while (1) {
        rank = dict_enumerate(cmd->dict, rank, &key, &val);
        if (rank < 0) {
            break;
        }
        printf("key = %s\n", key);
        ++i;
    }
    return i;
}

void cmd_loop(struct cmd *cmd)
{
    char *s;
    while (1) {
        s = cmd_get_input();
        if (!s) {
            continue;
        }
        cmd_execute(cmd, s);
    }
}
