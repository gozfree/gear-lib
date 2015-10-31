#ifndef _LIBCMD_H_
#define _LIBCMD_H_

#include <libdict.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cmd_entry {
    const char *name;
    int (*func)(int argc, char **argv);
} cmd_entry_t;

typedef struct cmd {
    int cnt;
    dict *dict;
    struct cmd_entry **entry;
} cmd_t;

typedef int (*cmd_cb)(int, char **);

struct cmd *cmd_init(int num);
int cmd_register(struct cmd *cmd, const char *name, cmd_cb func);
int cmd_execute(struct cmd *cmd, const char *name);
int cmd_get_registered(struct cmd *cmd);
void cmd_loop(struct cmd *cmd);

#ifdef __cplusplus
}
#endif
#endif
