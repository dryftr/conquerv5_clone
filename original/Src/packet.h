#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Packet types */
typedef enum {
    PACKET_LOCK_STATE = 1,
    PACKET_GAME_STATE = 2,
    PACKET_CHAT_MESSAGE = 3,
    PACKET_PLAYER_ACTION = 4,
    PACKET_HEARTBEAT = 5
} packet_type_e;

/* Packet header */
typedef struct {
    uint32_t magic;           /* Magic number for validation */
    uint32_t type;            /* Packet type */
    uint32_t length;          /* Payload length */
    uint32_t checksum;        /* Simple checksum */
} packet_header_t;

#define PACKET_MAGIC 0x434F4E51  /* "CONQ" in ASCII */

/* Packet structure */
typedef struct {
    packet_header_t header;
    char payload[1024];      /* Variable payload */
} packet_t;

/* Function declarations */
int packet_create(packet_t *pkt, packet_type_e type, const void *data, uint32_t len);
int packet_validate(const packet_t *pkt);
uint32_t packet_checksum(const void *data, uint32_t len);
int packet_serialize_lock(const memory_lock_t *lock, char *buf, uint32_t buf_size);
int packet_deserialize_lock(const char *buf, uint32_t buf_size, memory_lock_t *lock);
int packet_serialize_game_state(const void *game_state, uint32_t state_size, 
                              char *buf, uint32_t buf_size);
int packet_deserialize_game_state(const char *buf, uint32_t buf_size, 
                                 void *game_state, uint32_t state_size);

#endif /* PACKET_H */
