#include "multiplayer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Multiplayer context */
static struct {
    int initialized;
    int is_host;
    player_info_t players[8];  /* MAXNTN = 8 */
    int player_count;
    turn_state_t turn_state;
    int my_nation_id;
} mp_ctx = {0};

/* Initialize multiplayer */
int mp_init(const char *host, int port, socket_type_e type) {
    if (xfer_net_init(host, port, type) != 0) {
        return -1;
    }
    
    if (xfer_encrypt_init(ENCRYPT_NONE) != 0) {  /* Use XOR for now */
        return -1;
    }
    
    mp_ctx.initialized = 1;
    mp_ctx.player_count = 0;
    mp_ctx.turn_state.turn_number = 0;
    mp_ctx.turn_state.is_active = 0;
    
    printf("Multiplayer initialized: %s:%d\n", host, port);
    return 0;
}

/* Shutdown multiplayer */
void mp_shutdown(void) {
    if (!mp_ctx.initialized) return;
    
    for (int i = 0; i < mp_ctx.player_count; i++) {
        if (mp_ctx.players[i].name[0] != '\0') {
            mp_leave_game();  /* Notify others */
        }
    }
    
    mp_ctx.initialized = 0;
    printf("Multiplayer shutdown\n");
}

/* Join game */
int mp_join_game(const char *player_name, int nation_id) {
    if (!mp_ctx.initialized) return -1;
    
    /* Send join packet */
    char payload[128];
    int len = snprintf(payload, sizeof(payload), 
                      "JOIN|%s|%d",
                      player_name, nation_id);
    
    if (send_encrypted_packet(MP_JOIN, payload, len) != 0) {
        return -1;
    }
    
    mp_ctx.my_nation_id = nation_id;
    strncpy(mp_ctx.players[0].name, player_name, 31);
    mp_ctx.players[0].nation_id = nation_id;
    mp_ctx.players[0].is_ready = 0;
    mp_ctx.players[0].last_active = time(NULL);
    mp_ctx.player_count = 1;
    
    printf("Joined game as %s (nation %d)\n", player_name, nation_id);
    return 0;
}

/* Leave game */
int mp_leave_game(void) {
    if (!mp_ctx.initialized) return -1;
    
    /* Send leave packet */
    char payload[64];
    int len = snprintf(payload, sizeof(payload), 
                      "LEAVE|%d", mp_ctx.my_nation_id);
    
    if (send_encrypted_packet(MP_LEAVE, payload, len) != 0) {
        return -1;
    }
    
    printf("Left game\n");
    return 0;
}

/* Start turn */
int mp_start_turn(void) {
    if (!mp_ctx.initialized) return -1;
    
    mp_ctx.turn_state.turn_number++;
    mp_ctx.turn_state.is_active = 1;
    mp_ctx.turn_state.turn_start = time(NULL);
    mp_ctx.turn_state.current_player = mp_ctx.my_nation_id;
    
    /* Broadcast turn start */
    char payload[128];
    int len = snprintf(payload, sizeof(payload),
                      "TURN_START|%d|%d",
                      mp_ctx.turn_state.turn_number,
                      mp_ctx.turn_state.current_player);
    
    return send_encrypted_packet(MP_TURN_START, payload, len);
}

/* End turn */
int mp_end_turn(void) {
    if (!mp_ctx.initialized || !mp_ctx.turn_state.is_active) {
        return -1;
    }
    
    /* Broadcast turn end */
    char payload[128];
    int len = snprintf(payload, sizeof(payload),
                      "TURN_END|%d|%d",
                      mp_ctx.turn_state.turn_number,
                      mp_ctx.my_nation_id);
    
    if (send_encrypted_packet(MP_TURN_END, payload, len) != 0) {
        return -1;
    }
    
    mp_ctx.turn_state.is_active = 0;
    printf("Turn %d ended\n", mp_ctx.turn_state.turn_number);
    return 0;
}

/* Send move */
int mp_send_move(int unit_id, int from_x, int from_y, int to_x, int to_y) {
    if (!mp_ctx.initialized) return -1;
    
    char payload[128];
    int len = snprintf(payload, sizeof(payload),
                      "MOVE|%d|%d|%d|%d|%d",
                      unit_id, from_x, from_y, to_x, to_y);
    
    return send_encrypted_packet(MP_MOVE_UNIT, payload, len);
}

/* Send attack */
int mp_send_attack(int attacker_id, int defender_id) {
    if (!mp_ctx.initialized) return -1;
    
    char payload[128];
    int len = snprintf(payload, sizeof(payload),
                      "ATTACK|%d|%d",
                      attacker_id, defender_id);
    
    return send_encrypted_packet(MP_ATTACK, payload, len);
}

/* Send chat */
int mp_send_chat(const char *message) {
    if (!mp_ctx.initialized) return -1;
    
    char payload[1024];
    int len = snprintf(payload, sizeof(payload),
                      "CHAT|%d|%s",
                      mp_ctx.my_nation_id, message);
    
    return send_encrypted_packet(MP_CHAT, payload, len);
}

/* Broadcast game state */
int mp_broadcast_state(const void *state, uint32_t state_size) {
    if (!mp_ctx.initialized) return -1;
    
    return packet_serialize_game_state(state, state_size, 
                                     (char*)&send_buffer, 
                                     sizeof(send_buffer));
}

/* Receive and process multiplayer packets */
int mp_process_packets(void) {
    if (!mp_ctx.initialized) return -1;
    
    packet_t pkt;
    while (recv_encrypted_packet(&pkt) == 0) {
        uint32_t type = ntohl(pkt.header.type);
        uint32_t len = ntohl(pkt.header.length);
        char payload[1024];
        memcpy(payload, pkt.payload, len);
        payload[len] = '\0';
        
        switch (type) {
            case MP_TURN_START: {
                int turn_num, current_player;
                if (sscanf(payload, "TURN_START|%d|%d",
                           &turn_num, &current_player) == 2) {
                    mp_ctx.turn_state.turn_number = turn_num;
                    mp_ctx.turn_state.current_player = current_player;
                    mp_ctx.turn_state.is_active = 1;
                    printf("Turn %d started (player %d's turn)\n",
                           turn_num, current_player);
                }
                break;
            }
            case MP_TURN_END: {
                int turn_num, player_id;
                if (sscanf(payload, "TURN_END|%d|%d",
                           &turn_num, &player_id) == 2) {
                    printf("Turn %d ended by player %d\n",
                           turn_num, player_id);
                }
                break;
            }
            case MP_JOIN: {
                char name[32];
                int nation_id;
                if (sscanf(payload, "JOIN|%31[^|]|%d",
                           name, &nation_id) == 2) {
                    if (mp_ctx.player_count < 8) {
                        strncpy(mp_ctx.players[mp_ctx.player_count].name, 
                                name, 31);
                        mp_ctx.players[mp_ctx.player_count].nation_id = nation_id;
                        mp_ctx.players[mp_ctx.player_count].is_ready = 0;
                        mp_ctx.players[mp_ctx.player_count].last_active = time(NULL);
                        mp_ctx.player_count++;
                        printf("Player %s joined (nation %d)\n",
                               name, nation_id);
                    }
                }
                break;
            }
            case MP_LEAVE: {
                int nation_id;
                if (sscanf(payload, "LEAVE|%d", &nation_id) == 1) {
                    for (int i = 0; i < mp_ctx.player_count; i++) {
                        if (mp_ctx.players[i].nation_id == nation_id) {
                            printf("Player %s left (nation %d)\n",
                                   mp_ctx.players[i].name, nation_id);
                            /* Remove player */
                            for (int j = i; j < mp_ctx.player_count - 1; j++) {
                                mp_ctx.players[j] = mp_ctx.players[j+1];
                            }
                            mp_ctx.player_count--;
                            break;
                        }
                    }
                }
                break;
            }
            case MP_MOVE_UNIT: {
                int unit_id, from_x, from_y, to_x, to_y;
                if (sscanf(payload, "MOVE|%d|%d|%d|%d|%d",
                           &unit_id, &from_x, &from_y, &to_x, &to_y) == 5) {
                    printf("Unit %d moved from (%d,%d) to (%d,%d)\n",
                           unit_id, from_x, from_y, to_x, to_y);
                }
                break;
            }
            case MP_ATTACK: {
                int attacker_id, defender_id;
                if (sscanf(payload, "ATTACK|%d|%d",
                           &attacker_id, &defender_id) == 2) {
                    printf("Unit %d attacks unit %d\n",
                           attacker_id, defender_id);
                }
                break;
            }
            case MP_CHAT: {
                int nation_id;
                char message[512];
                if (sscanf(payload, "CHAT|%d|%511[^]",
                           &nation_id, message) == 2) {
                    printf("[Nation %d]: %s\n", nation_id, message);
                }
                break;
            }
            case MP_SYNC_STATE: {
                printf("Game state sync received\n");
                /* Would deserialize and apply game state */
                break;
            }
            default: {
                printf("Unknown packet type: %d\n", type);
                break;
            }
        }
    }
    
    return 0;
}

/* Check active players */
int mp_check_players(player_info_t *players, int max_players) {
    if (!mp_ctx.initialized) return -1;
    
    int count = 0;
    time_t now = time(NULL);
    
    for (int i = 0; i < mp_ctx.player_count && count < max_players; i++) {
        if (difftime(now, mp_ctx.players[i].last_active) < 60) {  /* 60 sec timeout */
            players[count++] = mp_ctx.players[i];
        }
    }
    
    return count;
}

/* Sync turn state */
int mp_sync_turn(turn_state_t *turn) {
    if (!mp_ctx.initialized || !turn) return -1;
    
    *turn = mp_ctx.turn_state;
    return 0;
}
