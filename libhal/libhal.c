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
#include "libhal.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
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
    char *p;
    char buf[512];

    info->cores = get_nprocs_conf();
    info->cores_available = get_nprocs();

    if (NULL == (fp = fopen("/proc/cpuinfo", "r"))) {
        printf("read cpuinfo failed!\n");
        return -1;
    }
    while (fgets(buf, 511, fp) != NULL) {
        if (memcmp(buf, "flags", 5) == 0 ||//x86
            memcmp(buf, "Features", 8) == 0) {//arm
            if (NULL != (p = strstr(buf, ": "))) {
                strcpy(info->features, p+2);
            }
        }
        if (memcmp(buf, "model name", 10) == 0) {
            if (NULL != (p = strstr(buf, ": "))) {
                strcpy(info->name, p+2);
            }
        }
        memset(buf, 0, sizeof(buf));
    }
    fclose(fp);
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
