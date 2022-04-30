#pragma once

#include <libsock.h>
#include <sys/types.h>
#include <sys/socket.h>

typedef struct sock_addr ip4_addr_t;

void camera_accessory_init();
void camera_accessory_set_ip_address(ip4_addr_t ip);
