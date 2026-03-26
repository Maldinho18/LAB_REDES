#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#define MAX_SUBSCRIBERS 100
#define MAX_TOPICS 100
#define MAX_MSG_LEN 1024
#define MAX_BUFFER 2048
#define TCP_PORT 5000
#define UDP_PORT 5001
#define HYBRID_PORT 5002
#define QUIC_PORT 5003
#define DEFAULT_HOST "127.0.0.1"

#define LOG_INFO(fmt, ...) \
    do { \
        time_t t = time(NULL); \
        struct tm *tm_info = localtime(&t); \
        printf("[%02d:%02d:%02d] [INFO] " fmt "\n", \
               tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, ##__VA_ARGS__); \
        fflush(stdout); \
    } while(0)

#define LOG_ERR(fmt, ...) \
    do { \
        time_t t = time(NULL); \
        struct tm *tm_info = localtime(&t); \
        fprintf(stderr, "[%02d:%02d:%02d] [ERROR] " fmt "\n", \
                tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, ##__VA_ARGS__); \
        fflush(stderr); \
    } while(0)

#define LOG_WARN(fmt, ...) \
    do { \
        time_t t = time(NULL); \
        struct tm *tm_info = localtime(&t); \
        printf("[%02d:%02d:%02d] [WARN] " fmt "\n", \
               tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, ##__VA_ARGS__); \
        fflush(stdout); \
    } while(0)

static inline long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

static inline void trim_newline(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len-1] == '\n' || str[len-1] == '\r')) {
        str[len-1] = '\0';
        len--;
    }
}

static inline int addr_equal(struct sockaddr_in *a, struct sockaddr_in *b) {
    return (a->sin_addr.s_addr == b->sin_addr.s_addr) &&
           (a->sin_port == b->sin_port);
}

static inline int make_tcp_server_socket(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(sock);
        return -1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        return -1;
    }
    if (listen(sock, 10) < 0) {
        perror("listen");
        close(sock);
        return -1;
    }
    LOG_INFO("TCP server socket created on port %d", port);
    return sock;
}

static inline int connect_tcp_client(const char *host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return -1;
    }
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }
    LOG_INFO("TCP client connected to %s:%d", host, port);
    return sock;
}

static inline int make_udp_server_socket(int port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(sock);
        return -1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        return -1;
    }
    LOG_INFO("UDP server socket created on port %d", port);
    return sock;
}

static inline int make_udp_client_socket(void) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }
    LOG_INFO("UDP client socket created");
    return sock;
}

static inline struct sockaddr_in make_remote_addr(const char *host, int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);
    return addr;
}

static inline int send_all(int sock, const char *data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        int n = send(sock, data + sent, len - sent, 0);
        if (n < 0) {
            perror("send");
            return -1;
        }
        if (n == 0) break;
        sent += n;
    }
    return sent;
}

static inline int send_line(int sock, const char *msg) {
    char buf[MAX_BUFFER];
    snprintf(buf, sizeof(buf), "%s\n", msg);
    return send_all(sock, buf, strlen(buf));
}

static inline int recv_line(int sock, char *buf, size_t len) {
    size_t i = 0;
    while (i < len - 1) {
        int n = recv(sock, buf + i, 1, 0);
        if (n < 0) {
            perror("recv");
            return -1;
        }
        if (n == 0) return 0;
        if (buf[i] == '\n') {
            buf[i] = '\0';
            return i;
        }
        i++;
    }
    buf[len-1] = '\0';
    return i;
}

static inline int split3(const char *str, char *p1, char *p2, char *p3,
                         size_t len1, size_t len2, size_t len3) {
    const char *pos1 = strchr(str, '|');
    if (!pos1) return -1;
    size_t sz1 = pos1 - str;
    if (sz1 >= len1) return -1;
    memcpy(p1, str, sz1);
    p1[sz1] = '\0';
    const char *pos2 = strchr(pos1 + 1, '|');
    if (!pos2) {
        size_t sz2 = strlen(pos1 + 1);
        if (sz2 >= len2) return -1;
        strcpy(p2, pos1 + 1);
        p3[0] = '\0';
        return 2;
    }
    size_t sz2 = pos2 - pos1 - 1;
    if (sz2 >= len2) return -1;
    memcpy(p2, pos1 + 1, sz2);
    p2[sz2] = '\0';
    size_t sz3 = strlen(pos2 + 1);
    if (sz3 >= len3) return -1;
    strcpy(p3, pos2 + 1);
    return 3;
}

static inline int split4(const char *str, char *p1, char *p2, char *p3, char *p4,
                         size_t len1, size_t len2, size_t len3, size_t len4) {
    const char *pos1 = strchr(str, '|');
    if (!pos1) return -1;
    size_t sz1 = pos1 - str;
    if (sz1 >= len1) return -1;
    memcpy(p1, str, sz1);
    p1[sz1] = '\0';
    const char *pos2 = strchr(pos1 + 1, '|');
    if (!pos2) return -1;
    size_t sz2 = pos2 - pos1 - 1;
    if (sz2 >= len2) return -1;
    memcpy(p2, pos1 + 1, sz2);
    p2[sz2] = '\0';
    const char *pos3 = strchr(pos2 + 1, '|');
    if (!pos3) return -1;
    size_t sz3 = pos3 - pos2 - 1;
    if (sz3 >= len3) return -1;
    memcpy(p3, pos2 + 1, sz3);
    p3[sz3] = '\0';
    size_t sz4 = strlen(pos3 + 1);
    if (sz4 >= len4) return -1;
    strcpy(p4, pos3 + 1);
    return 4;
}

static inline int wait_for_readable(int sock, int timeout_ms) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    int ret = select(sock + 1, &readfds, NULL, NULL, &tv);
    return ret;
}

#endif
