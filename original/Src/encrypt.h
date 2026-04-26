#ifndef ENCRYPT_H
#define ENCRYPT_H

#include "packets.h"

/* Encryption types */
typedef enum {
    ENCRYPT_NONE = 0,
    ENCRYPT_TLS = 1,      /* For TCP */
    ENCRYPT_DTLS = 2      /* For UDP */
} encrypt_type_e;

/* Encryption context */
typedef struct {
    encrypt_type_e type;
    int enabled;
    /* Simplified - real implementation would use OpenSSL */
    char key[32];
    char iv[16];
} encrypt_ctx_t;

/* Function declarations */
int encrypt_init(encrypt_ctx_t *ctx, encrypt_type_e type);
void encrypt_destroy(encrypt_ctx_t *ctx);
int encrypt_data(encrypt_ctx_t *ctx, const void *in, size_t in_len, 
                  void *out, size_t *out_len);
int decrypt_data(encrypt_ctx_t *ctx, const void *in, size_t in_len,
                  void *out, size_t *out_len);
int encrypt_packet(encrypt_ctx_t *ctx, packet_t *pkt);
int decrypt_packet(encrypt_ctx_t *ctx, const packet_t *in, packet_t *out);

#endif /* ENCRYPT_H */
