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
#include "libsock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#if defined (OS_LINUX)
#include <unistd.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#elif defined (OS_RTOS)
#include <net/if.h>
#elif defined (OS_RTTHREAD)
#include <sys/socket.h>
#include <netdb.h>
#define SOMAXCONN	128
#endif

#if defined (OS_WINDOWS)
#pragma warning(disable:4996)
#endif
#define LISTEN_MAX_BACKLOG  (128)
#define MTU                 (1500 - 42 - 200)
#define MAX_RETRY_CNT       (3)

#define USE_IPV6    0

#if defined (OS_WINDOWS)
static int sock_pre_init_win()
{
    WSADATA ws;
    if (WSAStartup(MAKEWORD(2, 2), &ws) != 0) {
        printf("Init Windows Socket Failed\n");
        return -1;
    }
    return 0;
}

static void sock_post_deinit_win()
{
    WSACleanup();
}
#else
typedef int SOCKET;
#endif

static int host_to_sockaddr(const char *host, struct in_addr *addr)
{
    struct addrinfo *res = NULL;
    struct addrinfo *ap;
    struct addrinfo hints;
    int ret;
    memset(&hints, 0, sizeof(struct addrinfo));
#if USE_IPV6
    hints.ai_family = AF_UNSPEC;
#else
    hints.ai_family = AF_INET;
#endif
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_CANONNAME;

    ret = getaddrinfo(host, NULL, &hints, &res);
    if (ret != 0) {
#if !defined (OS_RTOS)
        printf("getaddrinfo: %s\n", gai_strerror(ret));
#endif
        return -1;
    }
    for (ap = res; ap != NULL; ap = ap->ai_next) {
        if (ap->ai_family == hints.ai_family) {
            break;
        }
    }
    if (ap == NULL) {
        printf("addrinfo is invalid!\n");
        freeaddrinfo(res);
        return -1;
    }

    *addr = ((struct sockaddr_in*)(ap->ai_addr))->sin_addr;
    freeaddrinfo(res);
    return 0;
}

static struct sock_connection *_sock_connect(int type, const char *host, uint16_t port)
{
    int domain = -1;
    int protocol = 0;
    struct sockaddr_in si;
#if defined (OS_LINUX)
    struct sockaddr_un su;
#endif
    struct sockaddr *sa;
    size_t sa_len = 0;
    struct sock_connection *sc = NULL;
#if defined (OS_WINDOWS)
    sock_pre_init_win();
#endif
    if (type < 0 || strlen(host) == 0 || port >= 0xFFFF) {
        printf("invalid paraments\n");
        return NULL;
    }

    switch (type) {
    case SOCK_STREAM:
    case SOCK_DGRAM:
#if USE_IPV6
        domain = AF_UNSPEC;
#else
        domain = AF_INET;
#endif
        si.sin_family = domain;
        //si.sin_addr.s_addr = inet_addr(host);
        host_to_sockaddr(host, &si.sin_addr);
        si.sin_port = htons(port);

        sa = (struct sockaddr*)&si;
        sa_len = sizeof(si);
        break;
#if defined (OS_LINUX)
    case SOCK_SEQPACKET:
        snprintf(su.sun_path, sizeof(su.sun_path), "%s", host);
        su.sun_family = PF_UNIX;
        domain = PF_UNIX;
        sa = (struct sockaddr*)&su;
        sa_len = sizeof(su);
        break;
#endif
    default:
        printf("unknown socket type!\n");
        return NULL;
        break;
    }

    sc = (struct sock_connection* )calloc(1, sizeof(struct sock_connection));
    if (sc == NULL) {
        printf("malloc failed!\n");
        return NULL;
    }

    sc->fd = socket(domain, type, protocol);
    if (-1 == sc->fd) {
        printf("socket: %s\n", strerror(errno));
        goto fail;
    }

    if (-1 == connect(sc->fd, sa, sa_len)) {
        printf("connect failed: %s\n", strerror(errno));
        goto fail;
    }
    sc->remote.ip = inet_addr(host);
    sc->remote.port = port;
    sc->type = type;
    if (-1 == sock_getaddr_by_fd(sc->fd, &sc->local)) {
        printf("sock_getaddr_by_fd failed: %s\n", strerror(errno));
        goto fail;

    }

    return sc;
fail:
    if (sc) {
        if (-1 != sc->fd) {
            close(sc->fd);
        }
        free(sc);
    }
    return NULL;
}

struct sock_connection *sock_tcp_connect(const char *host, uint16_t port)
{
    return _sock_connect(SOCK_STREAM, host, port);
}

struct sock_connection *sock_udp_connect(const char *host, uint16_t port)
{
    return _sock_connect(SOCK_DGRAM, host, port);
}

struct sock_connection *sock_unix_connect(const char *host, uint16_t port)
{
#if defined (OS_LINUX)
    return _sock_connect(SOCK_SEQPACKET, host, port);
#else
    return NULL;
#endif
}

int sock_tcp_bind_listen(const char *host, uint16_t port)
{
    SOCKET fd;
    struct sockaddr_in si;
#if defined (OS_WINDOWS)
    sock_pre_init_win();
#endif
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == fd) {
        printf("socket failed: %s\n", strerror(errno));
        goto fail;
    }
    if (-1 == sock_set_reuse(fd, 1)) {
        printf("sock_set_reuse failed: %s\n", strerror(errno));
        goto fail;
    }
    si.sin_family = AF_INET;
    if (!host || strlen(host) == 0) {
        si.sin_addr.s_addr = INADDR_ANY;
    } else {
        si.sin_addr.s_addr = inet_addr(host);
    }
    si.sin_port = htons(port);
    if (-1 == bind(fd, (struct sockaddr*)&si, sizeof(si))) {
        printf("bind failed: %s\n", strerror(errno));
        goto fail;
    }
    if (-1 == listen(fd, SOMAXCONN)) {
        printf("listen failed: %s\n", strerror(errno));
        goto fail;
    }
    return fd;
fail:
    if (-1 != fd) {
        sock_close(fd);
    }
    return -1;
}

int sock_udp_bind(const char *host, uint16_t port)
{
    int fd;
    struct sockaddr_in si;
#if defined (OS_WINDOWS)
    sock_pre_init_win();
#endif
    si.sin_family = AF_INET;
    si.sin_addr.s_addr = host ? inet_addr(host) : INADDR_ANY;
    si.sin_port = htons(port);
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (-1 == fd) {
        printf("socket: %s\n", strerror(errno));
        return -1;
    }
    if (-1 == sock_set_reuse(fd, 1)) {
        printf("sock_set_reuse failed!\n");
        close(fd);
        return -1;
    }
    if (-1 == bind(fd, (struct sockaddr*)&si, sizeof(si))) {
        printf("bind: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    return fd;
}

//#if defined (__WIN32__) || defined (WIN32) || defined (_MSC_VER)
#if defined (OS_LINUX)
int sock_unix_bind_listen(const char *host, uint16_t port)
{
    int fd;
    struct sockaddr_un su;
   // WSADATA Ws;
   // if (WSAStartup(MAKEWORD(2,2), &Ws) != 0) {
   //     printf("Init Windows Socket Failed\n");
   //     return NULL;
   // }
    su.sun_family = PF_UNIX;
    snprintf(su.sun_path, sizeof(su.sun_path), "%s", host);
    fd = socket(PF_UNIX, SOCK_SEQPACKET, 0);
    if (-1 == fd) {
        printf("socket failed: %d\n", errno);
        return -1;
    }

    if (-1 == bind(fd, (struct sockaddr*)&su, sizeof(struct sockaddr_un))) {
        printf("bind %s failed: %d\n", host, errno);
        goto failed;
    }
    if (-1 == listen(fd, SOMAXCONN)) {
        printf("listen failed: %d\n", errno);
        goto failed;
    }
    return fd;

failed:
    close(fd);
    return -1;
}
#endif

struct sock_connection *sock_accept_connect(int fd)
{
    uint32_t ip;
    uint16_t port;
    struct sockaddr_in si;
    struct sock_connection *sc = NULL;
    socklen_t len = sizeof(si);
    int afd = accept(fd, (struct sockaddr *)&si, &len);
    if (afd == -1) {
        printf("accept: %s\n", strerror(errno));
        return NULL;
    } else {
        ip = si.sin_addr.s_addr;
        port = ntohs(si.sin_port);
    }
    sc = (struct sock_connection* )calloc(1, sizeof(struct sock_connection));
    if (sc != NULL) {
        sc->fd = afd;
        sc->type = SOCK_STREAM;
        if (-1 == sock_getaddr_by_fd(sc->fd, &sc->local)) {
            printf("sock_getaddr_by_fd failed: %s\n", strerror(errno));
        }
        sc->remote.ip = ip;
        sc->remote.port = port;
        sock_addr_ntop(sc->remote.ip_str, ip);
    }
    return sc;
}

int sock_accept(uint64_t fd, uint32_t *ip, uint16_t *port)
{
    struct sockaddr_in si;
    socklen_t len = sizeof(si);
#ifdef ENABLE_PTCP
    int afd = ptcp_accept(*(ptcp_socket_t *)&fd, (struct sockaddr *)&si, &len);
#else
    int afd = accept(fd, (struct sockaddr *)&si, &len);
#endif
    if (afd == -1) {
        printf("accept: %s\n", strerror(errno));
        return -1;
    } else {
        *ip = si.sin_addr.s_addr;
        *port = ntohs(si.sin_port);
    }
    return afd;
}

void sock_close(int fd)
{
#if defined (OS_LINUX)
    close(fd);
#elif defined (OS_WINDOWS)
  	closesocket(fd);
    sock_post_deinit_win();
#endif
}

#if defined (OS_LINUX)
int sock_get_tcp_info(int fd, struct tcp_info *tcpi)
{
    socklen_t len = sizeof(*tcpi);
    return getsockopt(fd, SOL_TCP, TCP_INFO, tcpi, &len);
}
#endif

#if defined (OS_LINUX)
int sock_get_local_list(sock_addr_list_t **al, int loopback)
{
#ifdef __ANDROID__
#else
    struct ifaddrs * ifs = NULL;
    struct ifaddrs * ifa = NULL;
    sock_addr_list_t *ap, *an;

    if (-1 == getifaddrs(&ifs)) {
        printf("getifaddrs: %s\n", strerror(errno));
        return -1;
    }

    ap = NULL;
    *al = NULL;
    for (ifa = ifs; ifa != NULL; ifa = ifa->ifa_next) {

        char saddr[SOCK_ADDR_LEN] = "";
        if (!(ifa->ifa_flags & IFF_UP))
            continue;
        if (!(ifa->ifa_addr))
            continue;
        if (ifa ->ifa_addr->sa_family == AF_INET) {
            if (!inet_ntop(AF_INET,
                           &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr,
                           saddr, SOCK_ADDR_LEN))
                continue;
            if (strstr(saddr,"169.254.") == saddr)
                continue;
            if (!strcmp(saddr,"0.0.0.0"))
                continue;
        } else if (ifa->ifa_addr->sa_family == AF_INET6) {
            if (!inet_ntop(AF_INET6,
                           &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr,
                           saddr, INET6_ADDRSTRLEN))
                continue;
            if (strstr(saddr,"fe80") == saddr)
                continue;
            if (!strcmp(saddr,"::"))
                continue;
        } else {
            continue;
        }
        if ((ifa->ifa_flags & IFF_LOOPBACK) && !loopback)
            continue;

        an = (sock_addr_list_t *)calloc(sizeof(sock_addr_list_t), 1);
        if (an == NULL) {
            continue;
        }
        an->addr.ip = ((struct sockaddr_in *) ifa->ifa_addr)->sin_addr.s_addr;
        an->addr.port = 0;
        an->next = NULL;
        if (*al == NULL) {
            *al = an;
            ap = *al;
        } else {
            ap->next = an;
            ap = ap->next;
        }
    }
    freeifaddrs(ifs);
#endif
    return 0;
}
#endif

int sock_get_remote_addr_by_fd(int fd, struct sock_addr *addr)
{
    struct sockaddr_in si;
    socklen_t len = sizeof(si);

    if (-1 == getpeername(fd, (struct sockaddr *)&(si.sin_addr), &len)) {
        printf("getpeername: %s\n", strerror(errno));
        return -1;
    }
    addr->ip = si.sin_addr.s_addr;
    addr->port = ntohs(si.sin_port);
    sock_addr_ntop(addr->ip_str, addr->ip);
    return 0;
}

int sock_getaddr_by_fd(int fd, struct sock_addr *addr)
{
    struct sockaddr_in si;
    socklen_t len = sizeof(si);
    memset(&si, 0, len);

    if (-1 == getsockname(fd, (struct sockaddr *)&si, &len)) {
        printf("getsockname: %s\n", strerror(errno));
        return -1;
    }

    addr->ip = si.sin_addr.s_addr;
    addr->port = ntohs(si.sin_port);
    sock_addr_ntop(addr->ip_str, addr->ip);

    return 0;
}

uint32_t sock_addr_pton(const char *ip)
{
    struct in_addr ia;
    int ret;

    ret = inet_pton(AF_INET, ip, &ia);
    if (ret == -1) {
        printf("inet_pton: %s\n", strerror(errno));
        return -1;
    } else if (ret == 0) {
        printf("inet_pton not in presentation format\n");
        return -1;
    }
    return ia.s_addr;
}

int sock_addr_ntop(char *str, uint32_t ip)
{
    struct in_addr ia;
    char tmp[SOCK_ADDR_LEN];

    ia.s_addr = ip;
    if (NULL == inet_ntop(AF_INET, &ia, tmp, SOCK_ADDR_LEN)) {
        printf("inet_ntop: %s\n", strerror(errno));
        return -1;
    }
    strncpy(str, tmp, SOCK_ADDR_LEN);
    return 0;
}

int sock_getaddrinfo(sock_addr_list_t **al, const char *domain, const char *port)
{
    int ret;
    struct addrinfo hints, *ai_list, *rp;
    sock_addr_list_t *ap, *an;

    memset(&hints, 0, sizeof(struct addrinfo));
#if USE_IPV6
    hints.ai_family = AF_UNSPEC;   /* Allows IPv4 or IPv6 */
#else
    hints.ai_family = AF_INET;   /* Allows IPv4 or IPv6 */
#endif
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_CANONNAME;

    ret = getaddrinfo(domain, port, &hints, &ai_list);
    if (ret != 0) {
        //printf("getaddrinfo: %s\n", gai_strerror(ret));
        return -1;
    }

    ap = NULL;
    *al = NULL;
    for (rp = ai_list; rp != NULL; rp = rp->ai_next ) {
        an = (sock_addr_list_t *)calloc(sizeof(sock_addr_list_t), 1);
        if (an == NULL) {
            continue;
        }
        an->addr.ip = ((struct sockaddr_in *)rp->ai_addr)->sin_addr.s_addr;
        an->addr.port = ntohs(((struct sockaddr_in *)rp->ai_addr)->sin_port);
        an->next = NULL;
        if (*al == NULL) {
            *al = an;
            ap = *al;
        } else {
            ap->next = an;
            ap = ap->next;
        }
    }
    if (al == NULL) {
        printf("Could not get ip address of %s!\n", domain);
        return -1;
    }
    freeaddrinfo(ai_list);
    return 0;
}

int sock_gethostbyname(sock_addr_list_t **al, const char *name)
{
    sock_addr_list_t *ap, *an;
    struct hostent *host;
    char **p;

    host = gethostbyname(name);
    if (NULL == host) {
        printf("gethostbyname failed!\n");
        return -1;
    }
    if (host->h_addrtype != AF_INET && host->h_addrtype != AF_INET6) {
        printf("addrtype error!\n");
        return -1;
    }

    ap = NULL;
    *al = NULL;
    for (p = host->h_addr_list; *p != NULL; p++) {
        an = (sock_addr_list_t *)calloc(sizeof(sock_addr_list_t), 1);
        if (an == NULL) {
            continue;
        }
        an->addr.ip = ((struct in_addr*)(*p))->s_addr;
        an->addr.port = 0;
        an->next = NULL;
        if (*al == NULL) {
            *al = an;
            ap = *al;
        } else {
            ap->next = an;
            ap = ap->next;
        }
        if (al == NULL) {
            printf("Could not get ip address of %s!\n", name);
            return -1;
        }
    }
    printf("hostname: %s\n", host->h_name);

    for (p = host->h_aliases; *p != NULL; p++) {
        printf("alias: %s\n", *p);
    }
    return 0;
}

#if defined (OS_LINUX)
int sock_get_local_info(void)
{
    int fd;
    int interfaceNum = 0;
    struct ifreq buf[16];
    struct ifconf ifc;
    struct ifreq ifrcopy;
    char mac[16] = {0};
    char ip[32] = {0};
    char broadAddr[32] = {0};
    char subnetMask[32] = {0};

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        close(fd);
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;
    if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc)) {
        interfaceNum = ifc.ifc_len / sizeof(struct ifreq);
        printf("interface num = %d\n", interfaceNum);
        while (interfaceNum-- > 0) {
            printf("\ndevice name: %s\n", buf[interfaceNum].ifr_name);

            //ignore the interface that not up or not runing
            ifrcopy = buf[interfaceNum];
            if (ioctl(fd, SIOCGIFFLAGS, &ifrcopy)) {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the mac of this interface
            if (!ioctl(fd, SIOCGIFHWADDR, (char *)(&buf[interfaceNum]))) {
                memset(mac, 0, sizeof(mac));
                snprintf(mac, sizeof(mac), "%02x%02x%02x%02x%02x%02x",
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[0],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[1],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[2],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[3],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[4],
                    (unsigned char)buf[interfaceNum].ifr_hwaddr.sa_data[5]);
                printf("device mac: %s\n", mac);
            } else {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the IP of this interface
            if (!ioctl(fd, SIOCGIFADDR, (char *)&buf[interfaceNum])) {
                snprintf(ip, sizeof(ip), "%s",
                    (char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_addr))->sin_addr));
                printf("device ip: %s\n", ip);
            } else {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the broad address of this interface
            if (!ioctl(fd, SIOCGIFBRDADDR, &buf[interfaceNum])) {
                snprintf(broadAddr, sizeof(broadAddr), "%s",
                    (char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_broadaddr))->sin_addr));
                printf("device broadAddr: %s\n", broadAddr);
            } else {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }

            //get the subnet mask of this interface
            if (!ioctl(fd, SIOCGIFNETMASK, &buf[interfaceNum])) {
                snprintf(subnetMask, sizeof(subnetMask), "%s",
                    (char *)inet_ntoa(((struct sockaddr_in *)&(buf[interfaceNum].ifr_netmask))->sin_addr));
                printf("device subnetMask: %s\n", subnetMask);
            } else {
                printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
                close(fd);
                return -1;
            }
        }
    } else {
        printf("ioctl: %s [%s:%d]\n", strerror(errno), __FILE__, __LINE__);
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}
#endif

int sock_set_noblk(int fd, int enable)
{
#if defined (OS_LINUX)
    int flag;
    flag = fcntl(fd, F_GETFL);
    if (flag == -1) {
        printf("fcntl: %s\n", strerror(errno));
        return -1;
    }
    if (enable) {
        flag |= O_NONBLOCK;
    } else {
        flag &= ~O_NONBLOCK;
    }
    if (-1 == fcntl(fd, F_SETFL, flag)) {
        printf("fcntl: %s\n", strerror(errno));
        return -1;
    }
#endif
    return 0;
}

int sock_set_block(int fd)
{
#if defined (OS_LINUX)
    int flag;
    flag = fcntl(fd, F_GETFL);
    if (flag == -1) {
        printf("fcntl: %s\n", strerror(errno));
        return -1;
    }
    flag &= ~O_NONBLOCK;
    if (-1 == fcntl(fd, F_SETFL, flag)) {
        printf("fcntl: %s\n", strerror(errno));
        return -1;
    }
#endif
    return 0;
}

int sock_set_nonblock(int fd)
{
#if defined (OS_LINUX)
    int flag;
    flag = fcntl(fd, F_GETFL);
    if (flag == -1) {
        printf("fcntl: %s\n", strerror(errno));
        return -1;
    }
    flag |= O_NONBLOCK;
    if (-1 == fcntl(fd, F_SETFL, flag)) {
        printf("fcntl: %s\n", strerror(errno));
        return -1;
    }
#endif
    return 0;
}
int sock_set_reuse(int fd, int enable)
{
    int on = !!enable;

#ifdef SO_REUSEPORT
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (unsigned char *)&on, sizeof(on))) {
        printf("setsockopt SO_REUSEPORT: %s\n", strerror(errno));
        return -1;
    }
#endif
#ifdef SO_REUSEADDR
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (unsigned char *)&on, sizeof(on))) {
        printf("setsockopt SO_REUSEADDR: %s\n", strerror(errno));
        return -1;
    }
#endif
    return 0;
}

int sock_set_tcp_keepalive(int fd, int enable)
{
    int on = !!enable;

#ifdef SO_KEEPALIVE
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
                         (const void*)&on, (socklen_t) sizeof(on))) {
        printf("setsockopt SO_KEEPALIVE: %s\n", strerror(errno));
        return -1;
    }
#endif
    return 0;
}

int sock_set_buflen(int fd, int size)
{
    int sz;

    sz = size;
    while (sz > 0) {
        if (-1 == setsockopt(fd, SOL_SOCKET, SO_RCVBUF,
                             (const void*) (&sz), (socklen_t) sizeof(sz))) {
            sz = sz / 2;
        } else {
            break;
        }
    }

    if (sz < 1) {
        printf("setsockopt SO_RCVBUF: %s\n", strerror(errno));
    }

    sz = size;
    while (sz > 0) {
        if (-1 == setsockopt(fd, SOL_SOCKET, SO_SNDBUF,
                             (const void*) (&sz), (socklen_t) sizeof(sz))) {
            sz = sz / 2;
        } else {
            break;
        }
    }

    if (sz < 1) {
        printf("setsockopt SO_SNDBUF: %s\n", strerror(errno));
    }
    return 0;
}

int sock_send(uint64_t fd, const void *buf, size_t len)
{
    ssize_t n;
    char *p = (char *)buf;
    size_t left = len;
    size_t step = len;
    int cnt = 0;

    if (buf == NULL || len == 0) {//0 length packet no need send
        printf("%s paraments invalid!\n", __func__);
        return -1;
    }

    while (left > 0) {
        if (left < step)
            step = left;
#ifdef ENABLE_PTCP
        n = ptcp_send(*(ptcp_socket_t *)&fd, p, step, 0);
#else
        n = send(fd, p, step, 0);
#endif
        if (n > 0) {
            p += n;
            left -= n;
            continue;
        } else if (n == 0) {
            perror("send");
            return -1;
        }
        if (errno == EINTR || errno == EAGAIN) {
            if (++cnt > MAX_RETRY_CNT) {
                printf("reach max retry count\n");
                break;
            }
            continue;
        }
        printf("send failed(%d): %s\n", errno, strerror(errno));
        return -1;
    }
    return (len - left);
}

int sock_sendto(int fd, const char *ip, uint16_t port,
                const void *buf, size_t len)
{
    ssize_t n;
    char *p = (char *)buf;
    size_t left = len;
    size_t step = len;
    struct sockaddr_in sa;
    int cnt = 0;

    if (buf == NULL || len == 0) {
        printf("%s paraments invalid!\n", __func__);
        return -1;
    }
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = ip?inet_addr(ip):INADDR_ANY;
    sa.sin_port = htons(port);

    while (left > 0) {
        if (left < step)
            step = left;
        n = sendto(fd, p, step, 0, (struct sockaddr*)&sa, sizeof(sa));
        if (n > 0) {
            p += n;
            left -= n;
            continue;
        } else if (n == 0) {
            printf("%s peer connect shutdown\n", __func__);
            return -1;
        }
        if (errno == EINTR || errno == EAGAIN) {
            if (++cnt > MAX_RETRY_CNT)
                break;
            continue;
        }
        printf("%s: sendto failed: %d\n", __func__, errno);
        return -1;
    }
    return (len - left);
}

int sock_recv(uint64_t fd, void *buf, size_t len)
{
    int n;
    char *p = (char *)buf;
    size_t left = len;
    size_t step = len;
    int cnt = 0;
    if (buf == NULL || len == 0) {
        printf("%s paraments invalid!\n", __func__);
        return -1;
    }
    while (left > 0) {
        if (left < step)
            step = left;
#ifdef ENABLE_PTCP
        n = ptcp_recv(*(ptcp_socket_t *)&fd, p, step, 0);
#else
        n = recv(fd, p, step, 0);
#endif
        if (n > 0) {
            p += n;
            left -= n;
            //continue;
            break;
        } else if (n == 0) {
            //perror("recv");//peer connect closed, no need print
            return 0;
        }
        if (errno == EINTR || errno == EAGAIN) {
            if (++cnt > MAX_RETRY_CNT)
                break;
            continue;
        }
        perror("recv");
        return -1;
    }
    return (len - left);
}

int sock_send_sync_recv(int fd, const void *sbuf, size_t slen,
                void *rbuf, size_t rlen, int timeout)
{
    sock_send(fd, sbuf, slen);
    sock_set_noblk(fd, 0);
    sock_recv(fd, rbuf, rlen);

    return 0;
}

int sock_recvfrom(int fd, uint32_t *ip, uint16_t *port, void *buf, size_t len)
{
    int n;
    char *p = (char *)buf;
    int cnt = 0;
    size_t left = len;
    size_t step = len;
    struct sockaddr_in si;
    socklen_t si_len = sizeof(si);

    memset(&si, 0, sizeof(si));

    while (left > 0) {
        if (left < step)
            step = left;
        n = recvfrom(fd, p, step, 0, (struct sockaddr *)&si, &si_len);
        if (n > 0) {
            p += n;
            left -= n;
            //continue;
            break;
        } else if (n == 0) {
            perror("recvfrom");
            return -1;
        }
        if (errno == EINTR || errno == EAGAIN) {
            if (++cnt > MAX_RETRY_CNT)
                break;
            continue;
        }
        perror("recvfrom");
        return -1;
    }

    *ip = si.sin_addr.s_addr;
    *port = ntohs(si.sin_port);

    return (len - left);
}

#ifdef ENABLE_PTCP
uint64_t sock_ptcp_bind_listen(const char *host, uint16_t port)
{
    struct sockaddr_in si;

    ptcp_socket_t fd = ptcp_socket();
    if (NULL == fd) {
        printf("socket failed: %s\n", strerror(errno));
        goto fail;
    }
    si.sin_family = AF_INET;
    if (!host || strlen(host) == 0) {
        si.sin_addr.s_addr = INADDR_ANY;
    } else {
        si.sin_addr.s_addr = inet_addr(host);
    }
    si.sin_port = htons(port);
    if (-1 == ptcp_bind(fd, (struct sockaddr*)&si, sizeof(si))) {
        printf("bind failed: %s\n", strerror(errno));
        goto fail;
    }
    if (-1 == ptcp_listen(fd, SOMAXCONN)) {
        printf("listen failed: %s\n", strerror(errno));
        goto fail;
    }

    return *(uint64_t *)&fd;
fail:
    if (NULL != fd) {
        ptcp_close(fd);
    }
    return -1;
}
#endif

#ifdef ENABLE_PTCP
struct sock_connection *sock_ptcp_connect(const char *host, uint16_t port)
{
    int ret;
    struct sockaddr_in si;
    struct sock_connection *sc = NULL;

    ptcp_socket_t ptcp = ptcp_socket();
    if (ptcp == NULL) {
        printf("ptcp_socket error!\n");
        return NULL;
    }

    si.sin_family = AF_INET;
    si.sin_addr.s_addr = inet_addr(host);
    si.sin_port = htons(port);
    ret = ptcp_connect(ptcp, (struct sockaddr*)&si, sizeof(si));
    if (-1 == ret) {
        printf("connect failed: %s\n", strerror(errno));
        goto fail;
    }

    sc = (struct sock_connection* )calloc(1, sizeof(struct sock_connection));
    if (sc == NULL) {
        printf("malloc failed!\n");
        return NULL;
    }
    sc->fd64 = *(uint64_t *)&ptcp;
    sc->remote.ip = inet_addr(host);
    sc->remote.port = port;
    sc->type = SOCK_DGRAM;
    sc->fd = ptcp_get_socket_fd(ptcp);
    printf("%s:%d xxxx fd=%d, fd64=%lx\n", __func__, __LINE__, sc->fd, sc->fd64);
    if (-1 == sock_getaddr_by_fd(sc->fd, &sc->local)) {
        printf("sock_getaddr_by_fd failed: %s\n", strerror(errno));
        goto fail;
    }
    return sc;
fail:
    if (NULL != ptcp) {
        ptcp_close(ptcp);
    }
    if (sc) {
        free(sc);
    }
    return NULL;
}
#endif
