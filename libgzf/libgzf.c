/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    libgzf.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-10-31 18:16
 * updated: 2015-10-31 18:16
 ******************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "libgzf.h"

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
