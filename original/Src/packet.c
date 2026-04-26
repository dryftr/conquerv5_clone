#include "packet.h"
#include "sockets.h"
#include <arpa/inet.h>

/* Calculate simple checksum */
uint32_t packet_checksum(const void *data, uint32_t len) {
    const uint8_t *bytes = (const uint8_t*)data;
    uint32_t sum = 0;
    for (uint32_t i = 0; i < len; i++) {
        sum += bytes[i];
    }
    return sum & 0xFF;
}

/* Create a packet */
int packet_create(packet_t *pkt, packet_type_e type, const void *data, uint32_t len) {
    if (!pkt || len > sizeof(pkt->payload)) return -1;
    
    pkt->header.magic = htonl(PACKET_MAGIC);
    pkt->header.type = htonl(type);
    pkt->header.length = htonl(len);
    pkt->header.checksum = 0;
    
    if (data && len > 0) {
        memcpy(pkt->payload, data, len);
        pkt->header.checksum = packet_checksum(data, len);
    }
    
    return 0;
}

/* Validate packet */
int packet_validate(const packet_t *pkt) {
    if (!pkt) return -1;
    
    if (ntohl(pkt->header.magic) != PACKET_MAGIC) {
        return -1;  /* Invalid magic */
    }
    
    uint32_t len = ntohl(pkt->header.length);
    if (len > sizeof(pkt->payload)) {
        return -1;  /* Invalid length */
    }
    
    /* Verify checksum */
    uint32_t calc_checksum = packet_checksum(pkt->payload, len);
    if (calc_checksum != pkt->header.checksum) {
        return -1;  /* Checksum mismatch */
    }
    
    return 0;
}

/* Serialize lock state into buffer */
int packet_serialize_lock(const memory_lock_t *lock, char *buf, uint32_t buf_size) {
    if (!lock || !buf || buf_size < 128) return -1;
    
    packet_t pkt;
    char payload[128];
    int len = snprintf(payload, sizeof(payload), 
                      "%d|%s|%d|%ld",
                      lock->is_locked,
                      lock->locked_by,
                      lock->ref_count,
                      (long)lock->lock_time);
    
    if (len < 0) return -1;
    
    return packet_create(&pkt, PACKET_LOCK_STATE, payload, len);
}

/* Deserialize lock state from buffer */
int packet_deserialize_lock(const char *buf, uint32_t buf_size, memory_lock_t *lock) {
    if (!buf || !lock || buf_size < sizeof(packet_t)) return -1;
    
    packet_t *pkt = (packet_t*)buf;
    if (packet_validate(pkt) != 0) return -1;
    
    if (ntohl(pkt->header.type) != PACKET_LOCK_STATE) return -1;
    
    uint32_t len = ntohl(pkt->header.length);
    if (len > sizeof(pkt->payload)) return -1;
    
    char payload[1024];
    memcpy(payload, pkt->payload, len);
    payload[len] = '\0';
    
    int is_locked, ref_count;
    long lock_time;
    char locked_by[32] = {0};
    
    if (sscanf(payload, "%d|%31[^|]|%d|%ld",
               &is_locked, locked_by, &ref_count, &lock_time) == 4) {
        lock->is_locked = is_locked;
        strncpy(lock->locked_by, locked_by, 31);
        lock->locked_by[31] = '\0';
        lock->ref_count = ref_count;
        lock->lock_time = (time_t)lock_time;
        return 0;
    }
    
    return -1;
}

/* Serialize game state (placeholder) */
int packet_serialize_game_state(const void *game_state, uint32_t state_size,
                              char *buf, uint32_t buf_size) {
    if (!game_state || !buf || buf_size < sizeof(packet_t) + state_size) return -1;
    
    packet_t *pkt = (packet_t*)buf;
    if (packet_create(pkt, PACKET_GAME_STATE, game_state, state_size) != 0) {
        return -1;
    }
    
    return sizeof(packet_header_t) + state_size;
}

/* Deserialize game state (placeholder) */
int packet_deserialize_game_state(const char *buf, uint32_t buf_size,
                                 void *game_state, uint32_t state_size) {
    if (!buf || !game_state || buf_size < sizeof(packet_t)) return -1;
    
    packet_t *pkt = (packet_t*)buf;
    if (packet_validate(pkt) != 0) return -1;
    
    if (ntohl(pkt->header.type) != PACKET_GAME_STATE) return -1;
    
    uint32_t len = ntohl(pkt->header.length);
    if (len != state_size) return -1;
    
    memcpy(game_state, pkt->payload, len);
    return 0;
}
