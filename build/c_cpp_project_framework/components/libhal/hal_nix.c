/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#include "libhal.h"
#include <libdstring.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <linux/if.h>
#include <linux/wireless.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define is_str_equal(a,b) \
    ((strlen(a) == strlen(b)) && (0 == strcasecmp(a,b)))

#define MAX_NETIF_NUM           8
#define SYS_CLASS_NET           "/sys/class/net"
#define SYS_BLK_MMCBLK0        "/sys/block/mmcblk0/device"
#define DEV_MMCBLK0             "/dev/mmcblk0"
#define DEV_MMCBLK0P1           "/dev/mmcblk0p1"
#define SYSTEM_MOUNTS           "/proc/self/mounts"

enum sdcard_key_index {
    SD_NAME = 0,
    SD_CID,
    SD_CSD,
    SD_SCR,
    SD_SERIAL,
    SD_MANFID,
    SD_OEMID,
    SD_DATE,
};

struct sdcard_desc {
    enum sdcard_key_index index;
    char dev[32];
    char desc[64];
};

static struct sdcard_desc sdcard_desc_list[] = {
    {SD_NAME,   "name"},
    {SD_CID,    "cid"},
    {SD_CSD,    "csd"},
    {SD_SCR,    "scr"},
    {SD_SERIAL, "serial"},
    {SD_MANFID, "manfid"},
    {SD_OEMID,  "oemid"},
    {SD_DATE,   "date"},
};

static ssize_t file_read_path(const char *path, void *data, size_t len)
{
    int fd = open(path, O_RDONLY, 0666);
    if (fd == -1) {
        return -1;
    }
    int n;
    char *p = (char *)data;
    size_t left = len;
    size_t step = 1024*1024;
    int cnt = 0;

    while (left > 0) {
        if (left < step)
            step = left;
        n = read(fd, (void *)p, step);
        if (n > 0) {
            p += n;
            left -= n;
            continue;
        } else {
            if (n == 0) {
                break;
            } else {
                if (errno == EINTR || errno == EAGAIN) {
                    if (++cnt > 3)
                        break;
                    continue;
                }
            }
        }
    }
    close(fd);
    return (len - left);
}


int network_get_info(const char *interface, struct network_info *info)
{
    int ret = -1;
    int fd = 0;
    do {
        if (!interface || !info) {
            printf("invalid paraments!\n");
            ret = -1;
            break;
        }
        memset(info, 0, sizeof(*info));

        //is_probed
        char net_dev[128] = {0};
        snprintf(net_dev, sizeof(net_dev), "%s/%s", SYS_CLASS_NET, interface);
        if (-1 == access(net_dev, F_OK|W_OK|R_OK)) {
            printf("access %s failed: %d\n", net_dev, errno);
            info->is_probed = false;
            break;
        } else {
            info->is_probed = true;
        }

        if (-1 == (fd = socket(AF_INET, SOCK_DGRAM, 0))) {
            printf("socket failed: %d\n", errno);
            break;
        }

        //is_running
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strcpy(ifr.ifr_name, interface);
        if (-1 == ioctl(fd, SIOCGIFFLAGS, &ifr)) {
            //printf("ioctl SIOCGIFFLAGS failed: %s\n", strerror(errno));
            break;
        }
        if (ifr.ifr_flags & IFF_RUNNING) {
            info->is_running = true;
        } else {
            info->is_running = false;
        }

        //ssid
        struct iwreq wreq;
        memset(&wreq, 0, sizeof(wreq));
        strcpy(wreq.ifr_name, interface);
        wreq.u.essid.pointer = info->ssid;
        wreq.u.essid.length = sizeof(info->ssid);
        if (-1 == ioctl(fd, SIOCGIWESSID, &wreq)) {
            //printf("ioctl SIOCGIWESSID failed: %s\n", strerror(errno));
        }
        printf("ssid = %s\n", (char *)wreq.u.essid.pointer);

        //ipaddr macaddr
        struct ifconf ifc;
        struct ifreq buf[MAX_NETIF_NUM];
        int if_num = 0;
        memset(&ifc, 0, sizeof(ifc));
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = (caddr_t)buf;
        if (-1 == ioctl(fd, SIOCGIFCONF, &ifc)) {
            //printf("ioctl SIOCGIFCONF failed: %s\n", strerror(errno));
            break;
        }
        if_num = ifc.ifc_len / sizeof(struct ifreq);
        if (if_num == 0) {
            printf("no network interface\n");
            break;
        }
        while (if_num-- > 0) {
            if (-1 == ioctl(fd, SIOCGIFADDR, (char *)&buf[if_num])) {
                printf("ioctl SIOCGIFADDR failed: %s\n", strerror(errno));
                break;
            }
            if (is_str_equal(interface, buf[if_num].ifr_name)) {
                snprintf(info->ipaddr, sizeof(info->ipaddr), "%s",
                    inet_ntoa(((struct sockaddr_in *)&(buf[if_num].ifr_addr))->sin_addr));
                if (!ioctl(fd, SIOCGIFHWADDR, (char *)(&buf[if_num]))) {
                    snprintf(info->macaddr, sizeof(info->macaddr),
                            "%02X:%02X:%02X:%02X:%02X:%02X",
                            (unsigned char)buf[if_num].ifr_hwaddr.sa_data[0],
                            (unsigned char)buf[if_num].ifr_hwaddr.sa_data[1],
                            (unsigned char)buf[if_num].ifr_hwaddr.sa_data[2],
                            (unsigned char)buf[if_num].ifr_hwaddr.sa_data[3],
                            (unsigned char)buf[if_num].ifr_hwaddr.sa_data[4],
                            (unsigned char)buf[if_num].ifr_hwaddr.sa_data[5]);
                }
                ret = 0;
                break;
            }
        }
    } while (0);
    if (fd) {
        close(fd);
    }
    return ret;
}

int sdcard_get_info(const char *mount_point, struct sdcard_info *info)
{
    int i = 0;
    uint64_t free_space = 0;
    char path[128];
    char buf[2048];
    int ret = 0;

    do {
        if (!mount_point || !info) {
            printf("invalid paraments!\n");
            ret = -1;
            break;
        }
        memset(info, 0, sizeof(*info));

        if (-1 == access(SYS_BLK_MMCBLK0, F_OK|W_OK|R_OK)) {
            printf("SDCard does not insert\n");
            info->is_insert = false;
            break;
        } else {
            info->is_insert = true;
        }
        for (i = 0; i < (int)ARRAY_SIZE(sdcard_desc_list); i++) {
            snprintf(path, sizeof(path), "%s/%s", SYS_BLK_MMCBLK0, sdcard_desc_list[i].dev);
            if (-1 == file_read_path(path, sdcard_desc_list[i].desc, sizeof(sdcard_desc_list[i].desc))) {
                continue;
            }
            size_t str_end = strlen(sdcard_desc_list[i].desc);
            if (sdcard_desc_list[i].desc[str_end-1] == '\n') {
                sdcard_desc_list[i].desc[str_end-1] = '\0';
            }
        }

        file_read_path(SYSTEM_MOUNTS, buf, sizeof(buf));
        if (!strstr(buf, DEV_MMCBLK0) && !strstr(buf, DEV_MMCBLK0P1)) {
            printf("sdcard insert, but mount failed, please format sdcard!");
            info->is_mounted = false;
            break;
        }
        info->is_mounted = true;

        struct statfs disk_statfs;
        if (-1 == statfs(mount_point, &disk_statfs)) {
            printf("get disk statfs failed: %d\n", errno);
            break;
        }
        free_space = ((uint64_t)disk_statfs.f_bsize * (uint64_t)disk_statfs.f_bfree);
        info->capacity = ((uint64_t)disk_statfs.f_bsize * (uint64_t)disk_statfs.f_blocks);
        strcpy(info->name,   sdcard_desc_list[SD_NAME].desc);
        strcpy(info->serial, sdcard_desc_list[SD_SERIAL].desc);
        strcpy(info->manfid, sdcard_desc_list[SD_MANFID].desc);
        strcpy(info->oemid,  sdcard_desc_list[SD_OEMID].desc);
        strcpy(info->date,   sdcard_desc_list[SD_DATE].desc);
        info->used_size = info->capacity-free_space;
    } while (0);
    return ret;
}

int cpu_get_info(struct cpu_info *info)
{
    FILE *fp;
    char *line = NULL;
    size_t linecap = 0;
    struct dstr proc_name;
    struct dstr proc_speed;
    int physical_id = -1;
    int last_physical_id = -1;

    info->cores_physical = get_nprocs_conf();
    info->cores_logical = get_nprocs();

    if (NULL == (fp = fopen("/proc/cpuinfo", "r"))) {
        printf("read cpuinfo failed!\n");
        return -1;
    }

    dstr_init(&proc_name);
    dstr_init(&proc_speed);

    while (getline(&line, &linecap, fp) != -1) {
        if (!strncmp(line, "model name", 10)) {
            char *start = strchr(line, ':');
            if (!start || *(++start) == '\0')
                continue;
            dstr_copy(&proc_name, start);
            dstr_resize(&proc_name, proc_name.len - 1);
            dstr_depad(&proc_name);
        }
        if (!strncmp(line, "physical id", 11)) {
            char *start = strchr(line, ':');
            if (!start || *(++start) == '\0')
                continue;
            physical_id = atoi(start);
        }
        if (!strncmp(line, "cpu MHz", 7)) {
            char *start = strchr(line, ':');
            if (!start || *(++start) == '\0')
                continue;
            dstr_copy(&proc_speed, start);
            dstr_resize(&proc_speed, proc_speed.len - 1);
            dstr_depad(&proc_speed);
        }

        if (*line == '\n' && physical_id != last_physical_id) {
            last_physical_id = physical_id;
            strncpy(info->name, proc_name.array, sizeof(info->name));
            strncpy(info->speed, proc_speed.array, sizeof(info->speed));
        }
    }

    fclose(fp);
    dstr_free(&proc_name);
    dstr_free(&proc_speed);
    free(line);
    return 0;
}

int memory_get_info(struct memory_info *mem)
{
    struct sysinfo info;
    if (sysinfo(&info) < 0)
        return -1;

    mem->total = (uint64_t)info.totalram * info.mem_unit;
    mem->free = ((uint64_t)info.freeram + (uint64_t)info.bufferram) * info.mem_unit;
    return 0;
}

int os_get_version(struct os_info *os)
{
    struct utsname info;
    if (uname(&info) < 0)
        return -1;

    strncpy(os->sysname, info.sysname, sizeof(os->sysname));
    strncpy(os->release, info.release, sizeof(os->release));
	printf("Kernel Version: %s %s\n", os->sysname, os->release);
    return 0;
}

int network_get_port_occupied(struct network_ports *np)
{
    FILE* fp;
    char line[256];
    unsigned int port;

    memset(np, 0, sizeof(*np));

    if (NULL != (fp = fopen("/proc/self/net/tcp", "r"))) {
        while (fgets(line, sizeof(line), fp)) {
            if (1 == sscanf(line, " %*d: %*p:%x", &port)) {
                np->tcp[np->tcp_cnt] = port;
                np->tcp_cnt++;
            }
        }
        fclose(fp);
    }

    if (NULL != (fp = fopen("/proc/self/net/udp", "r"))) {
        while (fgets(line, sizeof(line), fp)) {
            if (1 == sscanf(line, " %*d: %*p:%x", &port)) {
                np->udp[np->udp_cnt] = port;
                np->udp_cnt++;
            }
        }
        fclose(fp);
    }
    return 0;
}

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
