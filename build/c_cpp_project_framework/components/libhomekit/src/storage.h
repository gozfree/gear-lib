#ifndef __STORAGE_H__
#define __STORAGE_H__

#include "pairing.h"

int homekit_storage_reset();

int homekit_storage_init();

void homekit_storage_save_accessory_id(const char *accessory_id);
char *homekit_storage_load_accessory_id();

void homekit_storage_save_accessory_key(const ed25519_key *key);
ed25519_key *homekit_storage_load_accessory_key();

bool homekit_storage_can_add_pairing();
int homekit_storage_add_pairing(const char *device_id, const ed25519_key *device_key, byte permissions);
int homekit_storage_update_pairing(const char *device_id, byte permissions);
int homekit_storage_remove_pairing(const char *device_id);
pairing_t *homekit_storage_find_pairing(const char *device_id);

struct _pairing_iterator;
typedef struct _pairing_iterator pairing_iterator_t;

pairing_iterator_t *homekit_storage_pairing_iterator();
pairing_t *homekit_storage_next_pairing(pairing_iterator_t *iterator);
void homekit_storage_pairing_iterator_free(pairing_iterator_t *iterator);


#endif // __STORAGE_H__
