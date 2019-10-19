#include "pairing.h"


pairing_t *pairing_new() {
    pairing_t *p = malloc(sizeof(pairing_t));
    p->id = -1;
    p->device_id = NULL;
    p->device_key = NULL;
    p->permissions = 0;

    return p;
}

void pairing_free(pairing_t *pairing) {
    if (pairing->device_id)
        free(pairing->device_id);

    if (pairing->device_key)
        crypto_ed25519_free(pairing->device_key);

    free(pairing);
}
