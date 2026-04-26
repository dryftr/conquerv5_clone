#include "sockets.h"
#include <stdio.h>

/* Create a new socket */
conquer_socket_t* socket_create(socket_type_e type, const char *host, int port) {
    conquer_socket_t *sock = malloc(sizeof(conquer_socket_t));
    if (!sock) return NULL;
    
    sock->fd = socket(AF_INET, type == SOCKET_TCP ? SOCK_STREAM : SOCK_DGRAM, 0);
    if (sock->fd == -1) {
        free(sock);
        return NULL;
    }
    
    sock->type = type;
    sock->connected = 0;
    memset(&sock->addr, 0, sizeof(sock->addr));
    sock->addr.sin_family = AF_INET;
    sock->addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host, &sock->addr.sin_addr) <= 0) {
        close(sock->fd);
        free(sock);
        return NULL;
    }
    
    if (type == SOCKET_TCP) {
        if (connect(sock->fd, (struct sockaddr*)&sock->addr, sizeof(sock->addr)) {
            close(sock->fd);
            free(sock);
            return NULL;
        }
        sock->connected = 1;
    }
    
    return sock;
}

/* Destroy a socket */
void socket_destroy(conquer_socket_t *sock) {
    if (!sock) return;
    if (sock->fd != -1) {
        close(sock->fd);
    }
    free(sock);
}

/* Send data */
int socket_send(conquer_socket_t *sock, const void *data, size_t len) {
    if (!sock || sock->fd == -1) return -1;
    
    if (sock->type == SOCKET_TCP) {
        return send(sock->fd, data, len, 0);
    } else {
        return sendto(sock->fd, data, len, 0, 
                     (struct sockaddr*)&sock->addr, sizeof(sock->addr));
    }
}

/* Receive data */
int socket_recv(conquer_socket_t *sock, void *buf, size_t len) {
    if (!sock || sock->fd == -1) return -1;
    
    if (sock->type == SOCKET_TCP) {
        return recv(sock->fd, buf, len, 0);
    } else {
        socklen_t addr_len = sizeof(sock->addr);
        return recvfrom(sock->fd, buf, len, 0,
                         (struct sockaddr*)&sock->addr, &addr_len);
    }
}

/* Set non-blocking mode */
int socket_set_nonblocking(conquer_socket_t *sock) {
    if (!sock || sock->fd == -1) return -1;
    int flags = fcntl(sock->fd, F_GETFL, 0);
    return fcntl(sock->fd, F_SETFL, flags | O_NONBLOCK);
}

/* Wait for socket ready */
int socket_wait_ready(conquer_socket_t *sock, int timeout_ms) {
    if (!sock || sock->fd == -1) return 0;
    
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock->fd, &readfds);
    
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    return select(sock->fd + 1, &readfds, NULL, NULL, &tv) > 0;
}
