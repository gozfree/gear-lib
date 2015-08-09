/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libstun.h
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-05 00:14
 * updated: 2015-08-05 00:14
 *****************************************************************************/
#ifndef _LIBSTUN_H_
#define _LIBSTUN_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t port;
    uint32_t addr;
} stun_addr;

int stun_init(const char *ip);
int stun_socket(const char *ip, uint16_t port, stun_addr *map);
int stun_nat_type();
void stun_keep_alive(int fd);


#ifdef __cplusplus
}
#endif
#endif
