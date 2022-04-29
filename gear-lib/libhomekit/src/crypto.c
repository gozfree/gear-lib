#include <string.h>

// #include "user_settings.h"
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/hmac.h>
#include <wolfssl/wolfcrypt/ed25519.h>
#include <wolfssl/wolfcrypt/curve25519.h>
#include <wolfssl/wolfcrypt/sha512.h>
#include <wolfssl/wolfcrypt/chacha20_poly1305.h>
#include <wolfssl/wolfcrypt/srp.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

#include "port.h"
#include "debug.h"


// 3072-bit group N (per RFC5054, Appendix A)
const byte N[] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc9, 0x0f, 0xda, 0xa2,
  0x21, 0x68, 0xc2, 0x34, 0xc4, 0xc6, 0x62, 0x8b, 0x80, 0xdc, 0x1c, 0xd1,
  0x29, 0x02, 0x4e, 0x08, 0x8a, 0x67, 0xcc, 0x74, 0x02, 0x0b, 0xbe, 0xa6,
  0x3b, 0x13, 0x9b, 0x22, 0x51, 0x4a, 0x08, 0x79, 0x8e, 0x34, 0x04, 0xdd,
  0xef, 0x95, 0x19, 0xb3, 0xcd, 0x3a, 0x43, 0x1b, 0x30, 0x2b, 0x0a, 0x6d,
  0xf2, 0x5f, 0x14, 0x37, 0x4f, 0xe1, 0x35, 0x6d, 0x6d, 0x51, 0xc2, 0x45,
  0xe4, 0x85, 0xb5, 0x76, 0x62, 0x5e, 0x7e, 0xc6, 0xf4, 0x4c, 0x42, 0xe9,
  0xa6, 0x37, 0xed, 0x6b, 0x0b, 0xff, 0x5c, 0xb6, 0xf4, 0x06, 0xb7, 0xed,
  0xee, 0x38, 0x6b, 0xfb, 0x5a, 0x89, 0x9f, 0xa5, 0xae, 0x9f, 0x24, 0x11,
  0x7c, 0x4b, 0x1f, 0xe6, 0x49, 0x28, 0x66, 0x51, 0xec, 0xe4, 0x5b, 0x3d,
  0xc2, 0x00, 0x7c, 0xb8, 0xa1, 0x63, 0xbf, 0x05, 0x98, 0xda, 0x48, 0x36,
  0x1c, 0x55, 0xd3, 0x9a, 0x69, 0x16, 0x3f, 0xa8, 0xfd, 0x24, 0xcf, 0x5f,
  0x83, 0x65, 0x5d, 0x23, 0xdc, 0xa3, 0xad, 0x96, 0x1c, 0x62, 0xf3, 0x56,
  0x20, 0x85, 0x52, 0xbb, 0x9e, 0xd5, 0x29, 0x07, 0x70, 0x96, 0x96, 0x6d,
  0x67, 0x0c, 0x35, 0x4e, 0x4a, 0xbc, 0x98, 0x04, 0xf1, 0x74, 0x6c, 0x08,
  0xca, 0x18, 0x21, 0x7c, 0x32, 0x90, 0x5e, 0x46, 0x2e, 0x36, 0xce, 0x3b,
  0xe3, 0x9e, 0x77, 0x2c, 0x18, 0x0e, 0x86, 0x03, 0x9b, 0x27, 0x83, 0xa2,
  0xec, 0x07, 0xa2, 0x8f, 0xb5, 0xc5, 0x5d, 0xf0, 0x6f, 0x4c, 0x52, 0xc9,
  0xde, 0x2b, 0xcb, 0xf6, 0x95, 0x58, 0x17, 0x18, 0x39, 0x95, 0x49, 0x7c,
  0xea, 0x95, 0x6a, 0xe5, 0x15, 0xd2, 0x26, 0x18, 0x98, 0xfa, 0x05, 0x10,
  0x15, 0x72, 0x8e, 0x5a, 0x8a, 0xaa, 0xc4, 0x2d, 0xad, 0x33, 0x17, 0x0d,
  0x04, 0x50, 0x7a, 0x33, 0xa8, 0x55, 0x21, 0xab, 0xdf, 0x1c, 0xba, 0x64,
  0xec, 0xfb, 0x85, 0x04, 0x58, 0xdb, 0xef, 0x0a, 0x8a, 0xea, 0x71, 0x57,
  0x5d, 0x06, 0x0c, 0x7d, 0xb3, 0x97, 0x0f, 0x85, 0xa6, 0xe1, 0xe4, 0xc7,
  0xab, 0xf5, 0xae, 0x8c, 0xdb, 0x09, 0x33, 0xd7, 0x1e, 0x8c, 0x94, 0xe0,
  0x4a, 0x25, 0x61, 0x9d, 0xce, 0xe3, 0xd2, 0x26, 0x1a, 0xd2, 0xee, 0x6b,
  0xf1, 0x2f, 0xfa, 0x06, 0xd9, 0x8a, 0x08, 0x64, 0xd8, 0x76, 0x02, 0x73,
  0x3e, 0xc8, 0x6a, 0x64, 0x52, 0x1f, 0x2b, 0x18, 0x17, 0x7b, 0x20, 0x0c,
  0xbb, 0xe1, 0x17, 0x57, 0x7a, 0x61, 0x5d, 0x6c, 0x77, 0x09, 0x88, 0xc0,
  0xba, 0xd9, 0x46, 0xe2, 0x08, 0xe2, 0x4f, 0xa0, 0x74, 0xe5, 0xab, 0x31,
  0x43, 0xdb, 0x5b, 0xfc, 0xe0, 0xfd, 0x10, 0x8e, 0x4b, 0x82, 0xd1, 0x20,
  0xa9, 0x3a, 0xd2, 0xca, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

// 3072-bit group generator (per RFC5054, Appendix A)
const byte g[] = {0x05};


int wc_SrpSetKeyH(Srp *srp, byte *secret, word32 size) {
    SrpHash hash;
    int r = BAD_FUNC_ARG;

    srp->key = (byte*) XMALLOC(WC_SHA512_DIGEST_SIZE, NULL, DYNAMIC_TYPE_SRP);
    if (!srp->key)
        return MEMORY_E;

    srp->keySz = WC_SHA512_DIGEST_SIZE;

    r = wc_InitSha512(&hash.data.sha512);
    if (!r) r = wc_Sha512Update(&hash.data.sha512, secret, size);
    if (!r) r = wc_Sha512Final(&hash.data.sha512, srp->key);

    // clean up hash data from stack for security
    memset(&hash, 0, sizeof(hash));

    return r;
}


Srp *crypto_srp_new() {
    Srp *srp = malloc(sizeof(Srp));

    DEBUG("Initializing SRP");
    int r = wc_SrpInit(srp, SRP_TYPE_SHA512, SRP_CLIENT_SIDE);
    if (r) {
        DEBUG("Failed to initialize SRP (code %d)", r);
        return NULL;
    }
    srp->keyGenFunc_cb = wc_SrpSetKeyH;

    return srp;
}


void crypto_srp_free(Srp *srp) {
    wc_SrpTerm(srp);
    free(srp);
}


int crypto_srp_init(Srp *srp, const char *username, const char *password) {
    DEBUG("Generating salt");
    byte salt[16];
    homekit_random_fill(salt, sizeof(salt));

    int r;
    DEBUG("Setting SRP username");
    r = wc_SrpSetUsername(srp, (byte*)username, strlen(username));
    if (r) {
        DEBUG("Failed to set SRP username (code %d)", r);
        return r;
    }

    DEBUG("Setting SRP params");
    r = wc_SrpSetParams(srp, N, sizeof(N), g, sizeof(g), salt, sizeof(salt));
    if (r) {
        DEBUG("Failed to set SRP params (code %d)", r);
        return r;
    }

    DEBUG("Setting SRP password");
    r = wc_SrpSetPassword(srp, (byte *)password, strlen(password));
    if (r) {
        DEBUG("Failed to set SRP password (code %d)", r);
        return r;
    }

    DEBUG("Getting SRP verifier");
    word32 verifierLen = 1024;
    byte *verifier = malloc(verifierLen);
    r = wc_SrpGetVerifier(srp, verifier, &verifierLen);
    if (r) {
        DEBUG("Failed to get SRP verifier (code %d)", r);
        free(verifier);
        return r;
    }

    srp->side = SRP_SERVER_SIDE;
    DEBUG("Setting SRP verifier");
    r = wc_SrpSetVerifier(srp, verifier, verifierLen);
    if (r) {
        DEBUG("Failed to set SRP verifier (code %d)", r);
        free(verifier);
        return r;
    }

    free(verifier);

    return 0;
}


int crypto_srp_get_salt(Srp *srp, byte *buffer, size_t *buffer_size) {
    if (buffer_size == NULL)
        return -1;

    if (*buffer_size < srp->saltSz) {
        *buffer_size = srp->saltSz;
        return -2;
    }

    memcpy(buffer, srp->salt, srp->saltSz);
    *buffer_size = srp->saltSz;
    return 0;
}


int crypto_srp_get_public_key(Srp *srp, byte *buffer, size_t *buffer_size) {
    if (buffer_size == NULL)
        return -1;

    // TODO: Fix hardcoded public key size
    if (*buffer_size < 384) {
        *buffer_size = 384;
        return -2;
    }

    DEBUG("Calculating public key");
    word32 len = *buffer_size;
    int r = wc_SrpGetPublic(srp, buffer, &len);
    *buffer_size = len;
    return r;
}


int crypto_srp_compute_key(
    Srp *srp,
    const byte *client_public_key, size_t client_public_key_size,
    const byte *server_public_key, size_t server_public_key_size
) {
    int r = wc_SrpComputeKey(
        srp,
        (byte *)client_public_key, client_public_key_size,
        (byte *)server_public_key, server_public_key_size
    );
    if (r) {
        DEBUG("Failed to generate SRP shared secret key (code %d)", r);
        return r;
    }

    return 0;
}


int crypto_srp_verify(Srp *srp, const byte *proof, size_t proof_size) {
    int r = wc_SrpVerifyPeersProof(srp, (byte *)proof, proof_size);
    if (r) {
        DEBUG("Failed to verify client SRP proof (code %d)", r);
        return r;
    }

    return 0;
}


int crypto_srp_get_proof(Srp *srp, byte *proof, size_t *proof_size) {
    if (proof_size == NULL)
        return -1;

    if (*proof_size < WC_SHA512_DIGEST_SIZE) {
        *proof_size = WC_SHA512_DIGEST_SIZE;
        return -2;
    }

    word32 proof_len = *proof_size;
    int r = wc_SrpGetProof(srp, proof, &proof_len);
    *proof_size = proof_len;

    return r;
}


int crypto_hkdf(
    const byte *key, size_t key_size,
    const byte *salt, size_t salt_size,
    const byte *info, size_t info_size,
    byte *output, size_t *output_size
) {
    if (output_size == NULL)
        return -1;

    if (*output_size < 32) {
        *output_size = 32;
        return -2;
    }

    *output_size = 32;

    int r = wc_HKDF(
        SHA512,
        key, key_size,
        salt, salt_size,
        info, info_size,

        output, *output_size
    );

    return r;
}


int crypto_srp_hkdf(
    Srp *srp,
    const byte *salt, size_t salt_size,
    const byte *info, size_t info_size,
    byte *output, size_t *output_size
) {
    return crypto_hkdf(
        srp->key, srp->keySz,
        salt, salt_size,
        info, info_size,
        output, output_size
    );
}


int crypto_chacha20poly1305_decrypt(
    const byte *key, const byte *nonce, const byte *aad, size_t aad_size,
    const byte *message, size_t message_size,
    byte *decrypted, size_t *decrypted_size
) {
    if (message_size <= CHACHA20_POLY1305_AEAD_AUTHTAG_SIZE) { 
        DEBUG("Decrypted message is too small");
        return -2;
    }

    if (decrypted_size == NULL)
        return -1;

    size_t len = message_size - CHACHA20_POLY1305_AEAD_AUTHTAG_SIZE;
    if (*decrypted_size < len) {
        *decrypted_size = len;
        return -2;
    }

    *decrypted_size = len;

    int r = wc_ChaCha20Poly1305_Decrypt(
        key, nonce, aad, aad_size,
        message, len, &message[len],
        decrypted
    );

    return r;
}

int crypto_chacha20poly1305_encrypt(
    const byte *key, const byte *nonce, const byte *aad, size_t aad_size,
    const byte *message, size_t message_size,
    byte *encrypted, size_t *encrypted_size
) {
    if (encrypted_size == NULL)
        return -1;

    size_t len = message_size + CHACHA20_POLY1305_AEAD_AUTHTAG_SIZE;
    if (*encrypted_size < len) {
        *encrypted_size = len;
        return -1;
    }

    *encrypted_size = len;

    int r = wc_ChaCha20Poly1305_Encrypt(
        key, nonce, aad, aad_size,
        message, message_size,
        encrypted, encrypted+message_size
    );

    return r;
}


ed25519_key *crypto_ed25519_new() {
    ed25519_key *key = malloc(sizeof(ed25519_key));
    int r = wc_ed25519_init(key);
    if (r) {
        free(key);
        return NULL;
    }
    return key;
}

void crypto_ed25519_free(ed25519_key *key) {
    if (key)
        free(key);
}

ed25519_key *crypto_ed25519_generate() {
    ed25519_key *key = crypto_ed25519_new();

    WC_RNG rng;
    int r = wc_ed25519_make_key(&rng, ED25519_KEY_SIZE, key);
    if (r) {
        DEBUG("Failed to generate key (code %d)", r);
        crypto_ed25519_free(key);
        return NULL;
    }

    return key;
}

int crypto_ed25519_import_key(ed25519_key *key, const byte *data, size_t size) {
    return wc_ed25519_import_private_key(
        data, ED25519_KEY_SIZE,
        data + ED25519_KEY_SIZE, ED25519_PUB_KEY_SIZE,
        key
    );
}

int crypto_ed25519_export_key(const ed25519_key *key, byte *buffer, size_t *size) {
    if (size == NULL)
        return -1;

    if (*size < ED25519_KEY_SIZE + ED25519_PUB_KEY_SIZE) {
        *size = ED25519_KEY_SIZE + ED25519_PUB_KEY_SIZE;
        return -2;
    }

    word32 key_size = *size;
    int r = wc_ed25519_export_private((ed25519_key *)key, buffer, &key_size);
    *size = key_size;
    return r;
}

int crypto_ed25519_import_public_key(ed25519_key *key, const byte *data, size_t size) {
    return wc_ed25519_import_public(data, size, key);
}

int crypto_ed25519_export_public_key(const ed25519_key *key, byte *buffer, size_t *size) {
    if (size == NULL) {
        return -1;
    }

    if (*size < ED25519_PUB_KEY_SIZE) {
        *size = ED25519_PUB_KEY_SIZE;
        return -2;
    }

    word32 len = *size;
    int r = wc_ed25519_export_public((ed25519_key *)key, buffer, &len);
    *size = len;
    return r;
}


int crypto_ed25519_sign(
    const ed25519_key *key,
    const byte *message, size_t message_size,
    byte *signature, size_t *signature_size
) {
    if (signature_size == NULL) {
        return -1;
    }

    if (*signature_size < ED25519_SIG_SIZE) {
        *signature_size = ED25519_SIG_SIZE;
        return -2;
    }

    word32 len = *signature_size;

    int r = wc_ed25519_sign_msg(
        message, message_size,
        signature, &len,
        (ed25519_key *)key
    );
    *signature_size = len;
    return r;
}


int crypto_ed25519_verify(
    const ed25519_key *key,
    const byte *message, size_t message_size,
    const byte *signature, size_t signature_size
) {
    int verified;
    int r = wc_ed25519_verify_msg(
        signature, signature_size,
        message, message_size,
        &verified, (ed25519_key *)key
    );
    return !r && !verified;
}


curve25519_key *crypto_curve25519_new() {
    curve25519_key *key = malloc(sizeof(curve25519_key));
    int r = wc_curve25519_init(key);
    if (r) {
        free(key);
        return NULL;
    }
    return key;
}


void crypto_curve25519_free(curve25519_key *key) {
    if (!key)
        return;

    wc_curve25519_free(key);
    free(key);
}


curve25519_key *crypto_curve25519_generate() {
    curve25519_key *key = crypto_curve25519_new();
    if (!key)
        return NULL;

    WC_RNG rng;
    int r = wc_curve25519_make_key(&rng, 32, key);
    if (r) {
        crypto_curve25519_free(key);
        return NULL;
    }

    return key;
}


int crypto_curve25519_import_public(curve25519_key *key, const byte *data, size_t size) {
    return wc_curve25519_import_public_ex(data, size, key, EC25519_LITTLE_ENDIAN);
}


int crypto_curve25519_export_public(const curve25519_key *key, byte *buffer, size_t *size) {
    if (*size == 0) {
        word32 len = 0;
        int r = wc_curve25519_export_public_ex(
            (curve25519_key *)key,
            (byte *)&len, &len,
            EC25519_LITTLE_ENDIAN
        );
        *size = len;
        return r;
    }

    word32 len = *size;
    int r = wc_curve25519_export_public_ex(
        (curve25519_key *)key,
        buffer, &len,
        EC25519_LITTLE_ENDIAN
    );
    *size = len;
    return r;
}


int crypto_curve25519_shared_secret(const curve25519_key *private_key, const curve25519_key *public_key, byte *buffer, size_t *size) {
    if (*size < CURVE25519_KEYSIZE) {
        *size = CURVE25519_KEYSIZE;
        return -2;
    }

    word32 len = *size;
    int r = wc_curve25519_shared_secret_ex(
        (curve25519_key *)private_key,
        (curve25519_key *)public_key,
        buffer, &len, EC25519_LITTLE_ENDIAN
    );
    *size = len;
    return r;
}

