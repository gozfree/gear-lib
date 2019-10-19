#ifndef __PAIRING_H__
#define __PAIRING_H__

#include "crypto.h"

typedef enum {
    pairing_permissions_admin = (1 << 0),
} pairing_permissions_t;

typedef struct {
    int id;
    char *device_id;
    ed25519_key *device_key;
    pairing_permissions_t permissions;
} pairing_t;

pairing_t *pairing_new();
void pairing_free(pairing_t *pairing);

#endif // __PAIRING_H__
