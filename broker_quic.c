#include "common.h"

#define QUIC_TIMEOUT_MS 2000
#define MAX_PENDING_QUIC 100

typedef struct {
    uint32_t seq;
    char topic[64];
    char content[MAX_MSG_LEN];
    struct sockaddr_in src_addr;
    long timestamp;
    int acked;
    struct sockaddr_in sub_addrs[MAX_SUBSCRIBERS];
    int sub_count;
    int sub_acks[MAX_SUBSCRIBERS];
} quic_pending_msg_t;

typedef struct {
    char name[64];
    struct sockaddr_in subscribers[MAX_SUBSCRIBERS];
    int subscriber_count;
    uint32_t last_seq[MAX_SUBSCRIBERS];
} quic_topic_t;

typedef struct {
    quic_topic_t topics[MAX_TOPICS];
    int topic_count;
    quic_pending_msg_t pending[MAX_PENDING_QUIC];
    int pending_count;
    pthread_mutex_t lock;
} quic_broker_t;

quic_broker_t broker;

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
        LOG_INFO("[QUIC] Created topic: %s", name);
        return idx;
    }
    return -1;
}

static int is_duplicate_quic(int topic_idx, uint32_t seq) {
    if (topic_idx < 0) return 0;
    for (int i = 0; i < broker.pending_count; i++) {
        if (broker.pending[i].seq == seq && 
            strcmp(broker.pending[i].topic, broker.topics[topic_idx].name) == 0) {
            return 1;
        }
    }
    return 0;
}

static int subscribe_client_quic(int topic_idx, const struct sockaddr_in *addr) {
    if (topic_idx < 0) return -1;
    quic_topic_t *topic = &broker.topics[topic_idx];
    for (int i = 0; i < topic->subscriber_count; i++) {
        if (addr_equal(&topic->subscribers[i], (struct sockaddr_in *)addr)) {
            return 0;
        }
    }
    if (topic->subscriber_count < MAX_SUBSCRIBERS) {
        topic->subscribers[topic->subscriber_count] = *addr;
        topic->last_seq[topic->subscriber_count] = 0;
        topic->subscriber_count++;
        LOG_INFO("[QUIC] Subscriber added to %s from %s:%d",
                 topic->name, inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
        return 0;
    }
    return -1;
}

static int add_pending_quic(uint32_t seq, int topic_idx, const char *content,
                             const struct sockaddr_in *src_addr) {
    if (broker.pending_count < MAX_PENDING_QUIC) {
        quic_pending_msg_t *p = &broker.pending[broker.pending_count];
        p->seq = seq;
        p->acked = 0;
        p->timestamp = now_ms();
        strcpy(p->topic, broker.topics[topic_idx].name);
        strcpy(p->content, content);
        p->src_addr = *src_addr;
        quic_topic_t *topic = &broker.topics[topic_idx];
        p->sub_count = topic->subscriber_count;
        for (int i = 0; i < topic->subscriber_count; i++) {
            p->sub_addrs[i] = topic->subscribers[i];
            p->sub_acks[i] = 0;
        }
        broker.pending_count++;
        return broker.pending_count - 1;
    }
    return -1;
}

static void distribute_event_quic(int sock, int msg_idx) {
    if (msg_idx < 0 || msg_idx >= broker.pending_count) return;
    quic_pending_msg_t *msg = &broker.pending[msg_idx];
    char event_msg[MAX_BUFFER];
    snprintf(event_msg, sizeof(event_msg), "EVENT|%u|%s|%s", 
             msg->seq, msg->topic, msg->content);
    for (int i = 0; i < msg->sub_count; i++) {
        if (sendto(sock, event_msg, strlen(event_msg) + 1, 0,
                   (struct sockaddr *)&msg->sub_addrs[i], sizeof(msg->sub_addrs[i])) < 0) {
            LOG_WARN("[QUIC] Failed to send event");
        }
    }
}

static void send_ack_to_publisher_quic(int sock, const struct sockaddr_in *addr, 
                                        uint32_t seq, int all_subs_acked) {
    char ack_msg[80];
    int status = all_subs_acked ? 1 : 0;
    snprintf(ack_msg, sizeof(ack_msg), "ACKPUB|%u|%d", seq, status);
    if (sendto(sock, ack_msg, strlen(ack_msg) + 1, 0,
               (struct sockaddr *)addr, sizeof(*addr)) < 0) {
        LOG_WARN("[QUIC] Failed to send ACK");
    }
}

int main(int argc, char *argv[]) {
    int port = QUIC_PORT;
    if (argc > 1) port = atoi(argv[1]);
    LOG_INFO("=== BROKER QUIC STARTED ===");
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
        LOG_INFO("[QUIC] Recv: %s", buffer);
        char cmd[64], seq_str[32], topic[64], content[MAX_MSG_LEN];
        
        if (strncmp(buffer, "ACKSUB|", 7) == 0) {
            if (split3(buffer, cmd, seq_str, content,
                       sizeof(cmd), sizeof(seq_str), sizeof(content)) >= 2) {
                uint32_t seq = atol(seq_str);
                for (int i = 0; i < broker.pending_count; i++) {
                    if (broker.pending[i].seq == seq) {
                        for (int j = 0; j < broker.pending[i].sub_count; j++) {
                            if (addr_equal(&broker.pending[i].sub_addrs[j], &remote_addr)) {
                                broker.pending[i].sub_acks[j] = 1;
                                LOG_INFO("[QUIC] Sub ACK seq=%u", seq);
                            }
                        }
                        int all_acked = 1;
                        for (int j = 0; j < broker.pending[i].sub_count; j++) {
                            if (!broker.pending[i].sub_acks[j]) {
                                all_acked = 0;
                                break;
                            }
                        }
                        if (all_acked) {
                            send_ack_to_publisher_quic(sock, &broker.pending[i].src_addr, seq, 1);
                            LOG_INFO("[QUIC] ALL subs ACKed seq=%u", seq);
                        }
                        break;
                    }
                }
            }
            continue;
        }
        
        if (split4(buffer, cmd, seq_str, topic, content, 
                   sizeof(cmd), sizeof(seq_str), sizeof(topic), sizeof(content)) >= 3) {
            if (strcmp(cmd, "DATA") == 0) {
                uint32_t seq = atol(seq_str);
                int idx = get_or_create_topic(topic);
                if (idx >= 0 && !is_duplicate_quic(idx, seq)) {
                    LOG_INFO("[QUIC] DATA seq=%u topic=%s", seq, topic);
                    int msg_idx = add_pending_quic(seq, idx, content, &remote_addr);
                    distribute_event_quic(sock, msg_idx);
                    send_ack_to_publisher_quic(sock, &remote_addr, seq, 0);
                } else {
                    LOG_WARN("[QUIC] DUP seq=%u", seq);
                    send_ack_to_publisher_quic(sock, &remote_addr, seq, -1);
                }
            }
        } else if (split3(buffer, cmd, topic, content, 
                          sizeof(cmd), sizeof(topic), sizeof(content)) >= 2) {
            if (strcmp(cmd, "SUBSCRIBE") == 0) {
                int idx = get_or_create_topic(topic);
                if (idx >= 0) {
                    subscribe_client_quic(idx, &remote_addr);
                    LOG_INFO("[QUIC] SUBSCRIBE topic=%s", topic);
                }
            }
        }
    }
    close(sock);
    return 0;
}
