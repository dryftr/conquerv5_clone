#ifndef MULTIPLAYER_H
#define MULTIPLAYER_H "

#include "sockets.h"
#include "packet.h"
#include "encrypt.h"

/* Multiplayer protocol types */
typedef enum {
    MP_TURN_START = 100,
    MP_TURN_END = 101,
    MP_MOVE_UNIT = 102,
    MP_ATTACK = 103,
    MP_CHAT = 104,
    MP_JOIN = 105,
    MP_LEAVE = 106,
    MP_SYNC_STATE = 107,
    MP_ERROR = 108
} mp_type_e;

/* Player info */
typedef struct {
    char name[32];
    int nation_id;
    int is_ready;
    time_t last_active;
} player_info_t;

/* Turn state */
typedef struct {
    int turn_number;
    int current_player;
    int is_active;
    time_t turn_start;
} turn_state_t;

/* Function declarations */
int mp_init(const char *host, int port, socket_type_e type);
void mp_shutdown(void);
int mp_join_game(const char *player_name, int nation_id);
int mp_leave_game(void);
int mp_start_turn(void);
int mp_end_turn(void);
int mp_send_move(int unit_id, int from_x, int from_y, int to_x, int to_y);
int mp_send_attack(int attacker_id, int defender_id);
int mp_send_chat(const char *message);
int mp_broadcast_state(const void *state, uint32_t state_size);
int mp_receive_state(void *state, uint32_t state_size);
int mp_check_players(player_info_t *players, int max_players);
int mp_sync_turn(turn_state_t *turn);

#endif /* MULTIPLAYER_H */
