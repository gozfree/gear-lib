#ifndef __CRYPTO_H__
#define __CRYPTO_H__

#include <stdlib.h>

typedef unsigned char byte;

#define HKDF_HASH_SIZE 32  // CHACHA20_POLY1305_AEAD_KEYSIZE

int crypto_hkdf(
    const byte *key, size_t key_size,
    const byte *salt, size_t salt_size,
    const byte *info, size_t info_size,
    byte *output, size_t *output_size
);

// SRP
struct _Srp;
typedef struct _Srp Srp;


Srp *crypto_srp_new();
void crypto_srp_free(Srp *srp);

int crypto_srp_init(Srp *srp, const char *username, const char *password);

int crypto_srp_get_salt(Srp *srp, byte *buffer, size_t *buffer_length);
int crypto_srp_get_public_key(Srp *srp, byte *buffer, size_t *buffer_length);

int crypto_srp_compute_key(
    Srp *srp,
    const byte *client_public_key, size_t client_public_key_size,
    const byte *server_public_key, size_t server_public_key_size
);
int crypto_srp_verify(Srp *srp, const byte *proof, size_t proof_size);
int crypto_srp_get_proof(Srp *srp, byte *proof, size_t *proof_size);

int crypto_srp_hkdf(
    Srp *srp,
    const byte *salt, size_t salt_size,
    const byte *info, size_t info_size,
    byte *output, size_t *output_size
);

int crypto_chacha20poly1305_encrypt(
    const byte *key, const byte *nonce, const byte *aad, size_t aad_size,
    const byte *message, size_t message_size,
    byte *encrypted, size_t *encrypted_size
);
int crypto_chacha20poly1305_decrypt(
    const byte *key, const byte *nonce, const byte *aad, size_t aad_size,
    const byte *message, size_t message_size,
    byte *decrypted, size_t *descrypted_size
);

// ED25519
struct _ed25519_key;
typedef struct _ed25519_key ed25519_key;

ed25519_key *crypto_ed25519_new();
ed25519_key *crypto_ed25519_generate();
void crypto_ed25519_free(ed25519_key *key);

int crypto_ed25519_import_key(
    ed25519_key *key,
    const byte *data, size_t size
);
int crypto_ed25519_export_key(
    const ed25519_key *key,
    byte *buffer, size_t *size
);

int crypto_ed25519_import_public_key(
    ed25519_key *key,
    const byte *data, size_t size
);
int crypto_ed25519_export_public_key(
    const ed25519_key *key,
    byte *buffer, size_t *size
);

int crypto_ed25519_sign(
    const ed25519_key *key,
    const byte *message, size_t message_size,
    byte *signature, size_t *signature_size
);
int crypto_ed25519_verify(
    const ed25519_key *key,
    const byte *message, size_t message_size,
    const byte *signature, size_t signature_size
);


// CURVE25519
struct _curve25519_key;
typedef struct _curve25519_key curve25519_key;

curve25519_key *crypto_curve25519_new();
curve25519_key *crypto_curve25519_generate();
void crypto_curve25519_free(curve25519_key *key);
int crypto_curve25519_import_public(
    curve25519_key *key,
    const byte *data, size_t size
);
int crypto_curve25519_export_public(
    const curve25519_key *key,
    byte *buffer, size_t *size
);
int crypto_curve25519_shared_secret(
    const curve25519_key *private_key,
    const curve25519_key *public_key,
    byte *buffer, size_t *size
);

#endif // __CRYPTO_H__
