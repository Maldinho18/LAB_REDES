#include "common.h"

#define MAX_CLIENTS 50

typedef struct {
    int socket;
    struct sockaddr_in addr;
    char topic[64];
    int is_publisher;
} client_t;

typedef struct {
    char name[64];
    client_t subscribers[MAX_SUBSCRIBERS];
    int subscriber_count;
    pthread_mutex_t lock;
} topic_registry_t;

typedef struct {
    topic_registry_t topics[MAX_TOPICS];
    int topic_count;
    pthread_mutex_t global_lock;
} broker_state_t;

broker_state_t broker;

static int get_or_create_topic(const char *name) {
    pthread_mutex_lock(&broker.global_lock);
    for (int i = 0; i < broker.topic_count; i++) {
        if (strcmp(broker.topics[i].name, name) == 0) {
            pthread_mutex_unlock(&broker.global_lock);
            return i;
        }
    }
    if (broker.topic_count < MAX_TOPICS) {
        int idx = broker.topic_count;
        strcpy(broker.topics[idx].name, name);
        broker.topics[idx].subscriber_count = 0;
        pthread_mutex_init(&broker.topics[idx].lock, NULL);
        broker.topic_count++;
        pthread_mutex_unlock(&broker.global_lock);
        LOG_INFO("Created topic: %s", name);
        return idx;
    }
    pthread_mutex_unlock(&broker.global_lock);
    return -1;
}

static int subscribe_client(int topic_idx, client_t *client) {
    if (topic_idx < 0 || topic_idx >= broker.topic_count) return -1;
    topic_registry_t *topic = &broker.topics[topic_idx];
    pthread_mutex_lock(&topic->lock);
    if (topic->subscriber_count < MAX_SUBSCRIBERS) {
        topic->subscribers[topic->subscriber_count] = *client;
        topic->subscriber_count++;
        pthread_mutex_unlock(&topic->lock);
        LOG_INFO("Subscriber added to topic %s (total: %d)", 
                 topic->name, topic->subscriber_count);
        return 0;
    }
    pthread_mutex_unlock(&topic->lock);
    return -1;
}

static void distribute_to_subscribers(int topic_idx, const char *msg) {
    if (topic_idx < 0 || topic_idx >= broker.topic_count) return;
    topic_registry_t *topic = &broker.topics[topic_idx];
    pthread_mutex_lock(&topic->lock);
    for (int i = 0; i < topic->subscriber_count; i++) {
        int sock = topic->subscribers[i].socket;
        if (send_line(sock, msg) < 0) {
            LOG_WARN("Failed to send to subscriber at socket %d", sock);
        }
    }
    pthread_mutex_unlock(&topic->lock);
}

static void* client_handler(void *arg) {
    client_t *client = (client_t *)arg;
    int sock = client->socket;
    char buffer[MAX_BUFFER];
    LOG_INFO("Client handler started for socket %d", sock);
    while (1) {
        int n = recv_line(sock, buffer, sizeof(buffer));
        if (n <= 0) {
            LOG_INFO("Client disconnected from socket %d", sock);
            close(sock);
            free(client);
            pthread_exit(NULL);
        }
        trim_newline(buffer);
        char cmd[64], topic[64], msg[MAX_MSG_LEN];
        if (split3(buffer, cmd, topic, msg, sizeof(cmd), sizeof(topic), sizeof(msg)) >= 2) {
            if (strcmp(cmd, "SUBSCRIBE") == 0) {
                int idx = get_or_create_topic(topic);
                if (idx >= 0) {
                    client->is_publisher = 0;
                    strcpy(client->topic, topic);
                    if (subscribe_client(idx, client) == 0) {
                        LOG_INFO("SUBSCRIBE request: %s from socket %d", topic, sock);
                    }
                }
            } else if (strcmp(cmd, "PUBLISH") == 0 && strlen(msg) > 0) {
                int idx = get_or_create_topic(topic);
                if (idx >= 0) {
                    char event_msg[MAX_BUFFER];
                    snprintf(event_msg, sizeof(event_msg), "EVENT|%s|%s", topic, msg);
                    LOG_INFO("PUBLISH request: %s -> %s", topic, msg);
                    distribute_to_subscribers(idx, event_msg);
                }
            }
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int port = TCP_PORT;
    if (argc > 1) port = atoi(argv[1]);
    LOG_INFO("=== BROKER TCP STARTED ===");
    LOG_INFO("Listening on port %d", port);
    memset(&broker, 0, sizeof(broker));
    pthread_mutex_init(&broker.global_lock, NULL);
    broker.topic_count = 0;
    int server_sock = make_tcp_server_socket(port);
    if (server_sock < 0) {
        LOG_ERR("Failed to create server socket");
        exit(1);
    }
    LOG_INFO("Broker ready. Waiting for connections...");
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }
        LOG_INFO("New connection from %s:%d", 
                 inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        client_t *client = malloc(sizeof(client_t));
        client->socket = client_sock;
        client->addr = client_addr;
        client->is_publisher = 0;
        pthread_t tid;
        if (pthread_create(&tid, NULL, client_handler, client) != 0) {
            LOG_ERR("Failed to create thread");
            close(client_sock);
            free(client);
        } else {
            pthread_detach(tid);
        }
    }
    close(server_sock);
    return 0;
}
