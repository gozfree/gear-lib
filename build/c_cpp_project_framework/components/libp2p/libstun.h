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
#ifndef LIBSTUN_H
#define LIBSTUN_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STUN_NAT_TYPE_Unknown = 0,
    STUN_NAT_TYPE_Failure,
    STUN_NAT_TYPE_Open,
    STUN_NAT_TYPE_Blocked,
    STUN_NAT_TYPE_ConeNat,
    STUN_NAT_TYPE_RestrictedNat,
    STUN_NAT_TYPE_PortRestrictedNat,
    STUN_NAT_TYPE_SymNat,
    STUN_NAT_TYPE_SymFirewall,
} stun_nat_type_t;

typedef struct {
    uint16_t port;
    uint32_t addr;
} stun_addr;

struct stun_t {
    stun_addr addr;
};

int stun_init(struct stun_t *stun, const char *ip);
int stun_socket(struct stun_t *stun, const char *ip, uint16_t port, stun_addr *map);
stun_nat_type_t stun_nat_type(struct stun_t *stun);
void stun_keep_alive(struct stun_t *stun, int fd);


#ifdef __cplusplus
}
#endif
#endif
