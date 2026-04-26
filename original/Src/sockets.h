#ifndef SOCKETS_H
#define SOCKETS_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define SOCKET_ERROR -1
#define MAX_MSG_SIZE 1024

/* Socket types */
typedef enum {
    SOCKET_TCP,
    SOCKET_UDP
} socket_type_e;

/* Socket structure */
typedef struct {
    int fd;
    socket_type_e type;
    struct sockaddr_in addr;
    int connected;
} conquer_socket_t;

/* Function declarations */
conquer_socket_t* socket_create(socket_type_e type, const char *host, int port);
void socket_destroy(conquer_socket_t *sock);
int socket_send(conquer_socket_t *sock, const void *data, size_t len);
int socket_recv(conquer_socket_t *sock, void *buf, size_t len);
int socket_set_nonblocking(conquer_socket_t *sock);
int socket_wait_ready(conquer_socket_t *sock, int timeout_ms);

#endif /* SOCKETS_H */
