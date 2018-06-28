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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "libosal.h"

int system_noblock(char **argv)
{
    pid_t pid;
    struct sigaction ignore, saveintr, savequit;
    sigset_t chldmask, savemask;

    if (argv == NULL)
        return -1;

    ignore.sa_handler = SIG_IGN;
    sigemptyset(&ignore.sa_mask);
    ignore.sa_flags = 0;
    if (sigaction(SIGINT, &ignore, &saveintr) < 0) return -1;
    if (sigaction(SIGQUIT, &ignore, &savequit) < 0) return -1;
    if (sigaction(SIGCHLD, &ignore, &savequit) < 0) return -1;

    sigemptyset(&chldmask);
    sigaddset(&chldmask, SIGCHLD);
    if (sigprocmask(SIG_BLOCK, &chldmask, &savemask) < 0) return -1;

    if ((pid = fork()) < 0) {
        perror("fork");
        return -1;
    } else if (pid == 0) {
        sigaction(SIGINT, &saveintr, NULL);
        sigaction(SIGQUIT, &savequit, NULL);
        sigaction(SIGCHLD, &savequit, NULL);
        sigprocmask(SIG_SETMASK, &savemask, NULL);
        if (-1 == execvp(argv[0], argv)) {
            perror("exec");
        }
        _exit(127);
    } else {
    }

    if (sigaction(SIGINT, &saveintr, NULL) < 0) return -1;
    if (sigaction(SIGQUIT, &savequit, NULL) < 0) return -1;
    if (sigaction(SIGCHLD, &ignore, &savequit) < 0) return -1;
    if (sigprocmask(SIG_SETMASK, &savemask, NULL) < 0) return -1;
    return pid;
}

ssize_t system_noblock_with_result(char **argv, void *buf, size_t count)
{
    int len = -1;
    int old_fd, new_fd;
    int fd[2];
    if (pipe(fd)) {
        printf("pipe failed\n");
        return -1;
    }
    int rfd = fd[0];
    int wfd = fd[1];

    if (EOF == fflush(stdout)) {
        printf("fflush failed: %s\n", strerror(errno));
        return -1;
    }
    if (-1 == (old_fd = dup(STDOUT_FILENO))) {
        printf("dup STDOUT_FILENO failed: %s\n", strerror(errno));
        return -1;
    }
    if (-1 == (new_fd = dup2(wfd, STDOUT_FILENO))) {
        //no need to check failed??
        //printf("dup2 STDOUT_FILENO failed: %s\n", strerror(errno));
        //return -1;
    }
    if (-1 == system_noblock(argv)) {
        printf("system call failed!\n");
        return -1;
    }
    if (-1 == read(rfd, buf, count-1)) {
        printf("read buffer failed!\n");
        return -1;
    }
    len = strlen(buf);
    *((char *)buf + len - 1) = '\0';
    if (-1 == dup2(old_fd, new_fd)) {
        printf("dup2 failed: %s\n", strerror(errno));
        return -1;
    }
    return len;
}

ssize_t system_with_result(const char *cmd, void *buf, size_t count)
{
    int len = -1;
    int old_fd, new_fd;
    int fd[2];
    if (pipe(fd)) {
        printf("pipe failed\n");
        return -1;
    }
    int rfd = fd[0];
    int wfd = fd[1];
    if (EOF == fflush(stdout)) {
        printf("fflush failed: %s\n", strerror(errno));
        return -1;
    }
    if (-1 == (old_fd = dup(STDOUT_FILENO))) {
        printf("dup STDOUT_FILENO failed: %s\n", strerror(errno));
        return -1;
    }
    if (-1 == (new_fd = dup2(wfd, STDOUT_FILENO))) {
        //no need to check failed??
        //printf("dup2 STDOUT_FILENO failed: %s\n", strerror(errno));
        //return -1;
    }
    if (-1 == system(cmd)) {
        printf("system call failed!\n");
        return -1;
    }
    if (-1 == read(rfd, buf, count-1)) {
        printf("read buffer failed!\n");
        return -1;
    }
    len = strlen(buf);
    *((char *)buf + len - 1) = '\0';
    if (-1 == dup2(old_fd, new_fd)) {
        printf("dup2 failed: %s\n", strerror(errno));
        return -1;
    }
    return len;
}

int is_little_endian(void)
{
    unsigned int probe = 0xff;
    size_t sz = sizeof(unsigned int);
    unsigned char * probe_byte = (unsigned char *)&probe;
    if (!(probe_byte[0] == 0xff || probe_byte[sz - 1] == 0xff)) {
        printf("%s: something wrong!\n", __func__);
    }
    return probe_byte[0] == 0xff;
}

bool proc_exist(const char *proc)
{
#define MAX_CMD_BUFSZ               128
    FILE* fp;
    int count;
    char buf[MAX_CMD_BUFSZ];
    char cmd[MAX_CMD_BUFSZ];
    bool exist = false;
    snprintf(cmd, sizeof(cmd), "ps -ef | grep -w %s | grep -v grep | wc -l", proc);

    do {
        if ((fp = popen(cmd, "r")) == NULL) {
            printf("popen failed\n");
            break;
        }
        if ((fgets(buf, MAX_CMD_BUFSZ, fp)) != NULL) {
            count = atoi(buf);
            if (count <= 0) {
                exist = false;
            } else if (count == 1) {
                exist = true;
            } else {
                printf("process number may be wrong");
            }
        }
        pclose(fp);
    } while (0);
    return exist;
}
