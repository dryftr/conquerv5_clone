#include "encrypt.h"
#include <string.h>
#include <stdlib.h>

/* Simplified encryption - XOR with key */
int encrypt_init(encrypt_ctx_t *ctx, encrypt_type_e type) {
    if (!ctx) return -1;
    
    ctx->type = type;
    ctx->enabled = (type != ENCRYPT_NONE);
    
    /* Generate simple key (in real implementation, use proper key exchange) */
    for (int i = 0; i < 32; i++) {
        ctx->key[i] = (char)(i + 42);  /* Simplified */
    }
    for (int i = 0; i < 16; i++) {
        ctx->iv[i] = (char)(i * 7);  /* Simplified */
    }
    
    return 0;
}

void encrypt_destroy(encrypt_ctx_t *ctx) {
    if (!ctx) return;
    memset(ctx->key, 0, sizeof(ctx->key));
    memset(ctx->iv, 0, sizeof(ctx->iv));
    ctx->enabled = 0;
}

/* Simple XOR encryption (placeholder for real TLS/DTLS) */
int encrypt_data(encrypt_ctx_t *ctx, const void *in, size_t in_len,
                  void *out, size_t *out_len) {
    if (!ctx || !ctx->enabled || !in || !out) return -1;
    
    if (*out_len < in_len) return -1;
    
    const unsigned char *input = (const unsigned char *)in;
    unsigned char *output = (unsigned char *)out;
    
    /* XOR with key (simplified - real implementation uses TLS library) */
    for (size_t i = 0; i < in_len; i++) {
        output[i] = input[i] ^ ctx->key[i % sizeof(ctx->key)];
    }
    
    *out_len = in_len;
    return 0;
}

/* Simple XOR decryption */
int decrypt_data(encrypt_ctx_t *ctx, const void *in, size_t in_len,
                  void *out, size_t *out_len) {
    /* XOR is symmetric - same operation */
    return encrypt_data(ctx, in, in_len, out, out_len);
}

/* Encrypt packet in-place */
int encrypt_packet(encrypt_ctx_t *ctx, packet_t *pkt) {
    if (!ctx || !ctx->enabled || !pkt) return -1;
    
    uint32_t payload_len = ntohl(pkt->header.length);
    if (payload_len == 0) return 0;  /* Nothing to encrypt */
    
    size_t out_len = payload_len;
    if (encrypt_data(ctx, pkt->payload, payload_len, 
                    pkt->payload, &out_len) != 0) {
        return -1;
    }
    
    /* Update checksum after encryption */
    pkt->header.checksum = htonl(packet_checksum(pkt->payload, payload_len));
    return 0;
}

/* Decrypt packet in-place */
int decrypt_packet(encrypt_ctx_t *ctx, const packet_t *in, packet_t *out) {
    if (!ctx || !ctx->enabled || !in || !out) return -1;
    
    /* Copy header */
    memcpy(out, in, sizeof(packet_header_t));
    
    uint32_t payload_len = ntohl(in->header.length);
    if (payload_len == 0) return 0;
    
    /* Decrypt payload */
    size_t out_len = payload_len;
    if (decrypt_data(ctx, in->payload, payload_len,
                     out->payload, &out_len) != 0) {
        return -1;
    }
    
    return 0;
}
