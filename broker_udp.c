#include "common.h"

typedef struct {
    struct sockaddr_in addr;
    char topic[64];
} udp_subscriber_t;

typedef struct {
    char name[64];
    udp_subscriber_t subscribers[MAX_SUBSCRIBERS];
    int subscriber_count;
} udp_topic_t;

typedef struct {
    udp_topic_t topics[MAX_TOPICS];
    int topic_count;
    pthread_mutex_t lock;
} udp_broker_t;

udp_broker_t broker;

static int get_or_create_topic(const char *name) {
    for (int i = 0; i < broker.topic_count; i++) {
        if (strcmp(broker.topics[i].name, name) == 0) {
            return i;
        }
    }
    if (broker.topic_count < MAX_TOPICS) {
        int idx = broker.topic_count;
        strcpy(broker.topics[idx].name, name);
        broker.topics[idx].subscriber_count = 0;
        broker.topic_count++;
        LOG_INFO("Created topic: %s", name);
        return idx;
    }
    return -1;
}

static int subscribe_client_udp(int topic_idx, const struct sockaddr_in *addr) {
    if (topic_idx < 0 || topic_idx >= broker.topic_count) return -1;
    udp_topic_t *topic = &broker.topics[topic_idx];
    for (int i = 0; i < topic->subscriber_count; i++) {
        if (addr_equal(&topic->subscribers[i].addr, (struct sockaddr_in *)addr)) {
            return 0;
        }
    }
    if (topic->subscriber_count < MAX_SUBSCRIBERS) {
        topic->subscribers[topic->subscriber_count].addr = *addr;
        strcpy(topic->subscribers[topic->subscriber_count].topic, topic->name);
        topic->subscriber_count++;
        LOG_INFO("UDP Subscriber added to topic %s from %s:%d (total: %d)", 
                 topic->name, inet_ntoa(addr->sin_addr), ntohs(addr->sin_port),
                 topic->subscriber_count);
        return 0;
    }
    return -1;
}

static void distribute_to_subscribers_udp(int sock, int topic_idx, const char *msg) {
    if (topic_idx < 0 || topic_idx >= broker.topic_count) return;
    udp_topic_t *topic = &broker.topics[topic_idx];
    for (int i = 0; i < topic->subscriber_count; i++) {
        struct sockaddr_in *addr = &topic->subscribers[i].addr;
        if (sendto(sock, msg, strlen(msg) + 1, 0, 
                   (struct sockaddr *)addr, sizeof(*addr)) < 0) {
            LOG_WARN("Failed to send to subscriber at %s:%d", 
                     inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
        }
    }
}

int main(int argc, char *argv[]) {
    int port = UDP_PORT;
    if (argc > 1) port = atoi(argv[1]);
    LOG_INFO("=== BROKER UDP STARTED ===");
    LOG_INFO("Listening on port %d", port);
    memset(&broker, 0, sizeof(broker));
    broker.topic_count = 0;
    pthread_mutex_init(&broker.lock, NULL);
    int sock = make_udp_server_socket(port);
    if (sock < 0) {
        LOG_ERR("Failed to create UDP server socket");
        exit(1);
    }
    LOG_INFO("Broker ready. Waiting for datagrams...");
    char buffer[MAX_BUFFER];
    struct sockaddr_in remote_addr;
    socklen_t addr_len = sizeof(remote_addr);
    while (1) {
        int n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                         (struct sockaddr *)&remote_addr, &addr_len);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }
        buffer[n] = '\0';
        LOG_INFO("Received from %s:%d: %s",
                 inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port),
                 buffer);
        char cmd[64], topic[64], msg[MAX_MSG_LEN];
        if (split3(buffer, cmd, topic, msg, sizeof(cmd), sizeof(topic), sizeof(msg)) >= 2) {
            if (strcmp(cmd, "SUBSCRIBE") == 0) {
                int idx = get_or_create_topic(topic);
                if (idx >= 0) {
                    subscribe_client_udp(idx, &remote_addr);
                    LOG_INFO("SUBSCRIBE request: %s from %s:%d", 
                             topic, inet_ntoa(remote_addr.sin_addr),
                             ntohs(remote_addr.sin_port));
                }
            } else if (strcmp(cmd, "PUBLISH") == 0 && strlen(msg) > 0) {
                int idx = get_or_create_topic(topic);
                if (idx >= 0) {
                    char event_msg[MAX_BUFFER];
                    snprintf(event_msg, sizeof(event_msg), "EVENT|%s|%s", topic, msg);
                    LOG_INFO("PUBLISH request: %s -> %s", topic, msg);
                    distribute_to_subscribers_udp(sock, idx, event_msg);
                }
            }
        }
    }
    close(sock);
    return 0;
}
