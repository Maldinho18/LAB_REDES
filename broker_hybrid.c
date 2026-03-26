#include "common.h"

#define HYBRID_TIMEOUT_MS 2000
#define MAX_PENDING_MESSAGES 100

typedef struct {
    uint32_t seq;
    char topic[64];
    char content[MAX_MSG_LEN];
    struct sockaddr_in src_addr;
    long timestamp;
    int acked;
} pending_msg_t;

typedef struct {
    char name[64];
    struct sockaddr_in subscribers[MAX_SUBSCRIBERS];
    int subscriber_count;
    uint32_t last_seq[MAX_SUBSCRIBERS];
} hybrid_topic_t;

typedef struct {
    hybrid_topic_t topics[MAX_TOPICS];
    int topic_count;
    pending_msg_t pending[MAX_PENDING_MESSAGES];
    int pending_count;
    uint32_t publisher_seqs[50];
    int publisher_count;
    pthread_mutex_t lock;
} hybrid_broker_t;

hybrid_broker_t broker;

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
        LOG_INFO("[HYBRID] Created topic: %s", name);
        return idx;
    }
    return -1;
}

static int is_duplicate(int topic_idx, uint32_t seq) {
    if (topic_idx < 0) return 0;
    for (int i = 0; i < broker.pending_count; i++) {
        if (broker.pending[i].seq == seq && 
            strcmp(broker.pending[i].topic, broker.topics[topic_idx].name) == 0) {
            return 1;
        }
    }
    return 0;
}

static void add_pending_message(uint32_t seq, int topic_idx, const char *content,
                                const struct sockaddr_in *src_addr) {
    if (broker.pending_count < MAX_PENDING_MESSAGES) {
        pending_msg_t *p = &broker.pending[broker.pending_count];
        p->seq = seq;
        p->acked = 0;
        p->timestamp = now_ms();
        strcpy(p->topic, broker.topics[topic_idx].name);
        strcpy(p->content, content);
        p->src_addr = *src_addr;
        broker.pending_count++;
    }
}

static int subscribe_client_hybrid(int topic_idx, const struct sockaddr_in *addr) {
    if (topic_idx < 0) return -1;
    hybrid_topic_t *topic = &broker.topics[topic_idx];
    for (int i = 0; i < topic->subscriber_count; i++) {
        if (addr_equal(&topic->subscribers[i], (struct sockaddr_in *)addr)) {
            return 0;
        }
    }
    if (topic->subscriber_count < MAX_SUBSCRIBERS) {
        topic->subscribers[topic->subscriber_count] = *addr;
        topic->last_seq[topic->subscriber_count] = 0;
        topic->subscriber_count++;
        LOG_INFO("[HYBRID] Subscriber added to %s from %s:%d",
                 topic->name, inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
        return 0;
    }
    return -1;
}

static void distribute_event_hybrid(int sock, int topic_idx, uint32_t seq,
                                    const char *content) {
    if (topic_idx < 0) return;
    hybrid_topic_t *topic = &broker.topics[topic_idx];
    char event_msg[MAX_BUFFER];
    snprintf(event_msg, sizeof(event_msg), "EVENT|%u|%s|%s", seq, topic->name, content);
    for (int i = 0; i < topic->subscriber_count; i++) {
        struct sockaddr_in *addr = &topic->subscribers[i];
        if (sendto(sock, event_msg, strlen(event_msg) + 1, 0,
                   (struct sockaddr *)addr, sizeof(*addr)) < 0) {
            LOG_WARN("[HYBRID] Failed to send event");
        }
    }
}

static void send_ack_to_publisher(int sock, const struct sockaddr_in *addr, uint32_t seq) {
    char ack_msg[64];
    snprintf(ack_msg, sizeof(ack_msg), "ACKPUB|%u", seq);
    if (sendto(sock, ack_msg, strlen(ack_msg) + 1, 0,
               (struct sockaddr *)addr, sizeof(*addr)) < 0) {
        LOG_WARN("[HYBRID] Failed to send ACK");
    }
}

int main(int argc, char *argv[]) {
    int port = HYBRID_PORT;
    if (argc > 1) port = atoi(argv[1]);
    LOG_INFO("=== BROKER HYBRID UDP STARTED ===");
    LOG_INFO("Listening on port %d", port);
    memset(&broker, 0, sizeof(broker));
    broker.topic_count = 0;
    broker.pending_count = 0;
    pthread_mutex_init(&broker.lock, NULL);
    int sock = make_udp_server_socket(port);
    if (sock < 0) {
        LOG_ERR("Failed to create UDP socket");
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
        LOG_INFO("[HYBRID] Received from %s:%d: %s",
                 inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port), buffer);
        char cmd[64], seq_str[32], topic[64], content[MAX_MSG_LEN];
        if (strcmp(buffer, "ACKSUB") == 0 || strncmp(buffer, "ACKSUB|", 7) == 0) {
            continue;
        }
        if (split4(buffer, cmd, seq_str, topic, content, 
                   sizeof(cmd), sizeof(seq_str), sizeof(topic), sizeof(content)) >= 3) {
            if (strcmp(cmd, "DATA") == 0) {
                uint32_t seq = atol(seq_str);
                int idx = get_or_create_topic(topic);
                if (idx >= 0 && !is_duplicate(idx, seq)) {
                    LOG_INFO("[HYBRID] NEW DATA: seq=%u, topic=%s", seq, topic);
                    add_pending_message(seq, idx, content, &remote_addr);
                    distribute_event_hybrid(sock, idx, seq, content);
                    send_ack_to_publisher(sock, &remote_addr, seq);
                } else {
                    LOG_WARN("[HYBRID] DUPLICATE seq=%u", seq);
                    send_ack_to_publisher(sock, &remote_addr, seq);
                }
            }
        } else if (split3(buffer, cmd, topic, content, 
                          sizeof(cmd), sizeof(topic), sizeof(content)) >= 2) {
            if (strcmp(cmd, "SUBSCRIBE") == 0) {
                int idx = get_or_create_topic(topic);
                if (idx >= 0) {
                    subscribe_client_hybrid(idx, &remote_addr);
                    LOG_INFO("[HYBRID] SUBSCRIBE: topic=%s", topic);
                }
            }
        }
    }
    close(sock);
    return 0;
}
