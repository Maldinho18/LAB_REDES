#include "common.h"

int main(int argc, char *argv[]) {
    const char *local_host = "0.0.0.0";
    int local_port = 9100 + (rand() % 1000);
    const char *broker_host = DEFAULT_HOST;
    int broker_port = HYBRID_PORT;
    const char *topic = "PARTIDO_A";
    const char *sub_id = "SUB_1";
    
    if (argc > 1) local_host = argv[1];
    if (argc > 2) local_port = atoi(argv[2]);
    if (argc > 3) broker_host = argv[3];
    if (argc > 4) broker_port = atoi(argv[4]);
    if (argc > 5) topic = argv[5];
    if (argc > 6) sub_id = argv[6];
    
    LOG_INFO("=== SUBSCRIBER HYBRID STARTED ===");
    LOG_INFO("Subscriber ID: %s | Topic: %s", sub_id, topic);
    LOG_INFO("Local port: %d | Broker: %s:%d", local_port, broker_host, broker_port);
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    if (strcmp(local_host, "0.0.0.0") == 0) {
        local_addr.sin_addr.s_addr = INADDR_ANY;
    } else if (inet_pton(AF_INET, local_host, &local_addr.sin_addr) <= 0) {
        LOG_WARN("Invalid local_host '%s', falling back to 0.0.0.0", local_host);
        local_addr.sin_addr.s_addr = INADDR_ANY;
    }
    local_addr.sin_port = htons(local_port);
    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind");
        close(sock);
        exit(1);
    }
    LOG_INFO("Socket bound to local port %d", local_port);
    struct sockaddr_in broker_addr = make_remote_addr(broker_host, broker_port);
    char subscribe_msg[MAX_BUFFER];
    snprintf(subscribe_msg, sizeof(subscribe_msg), "SUBSCRIBE|%s", topic);
    int n = sendto(sock, subscribe_msg, strlen(subscribe_msg) + 1, 0,
                   (struct sockaddr *)&broker_addr, sizeof(broker_addr));
    if (n < 0) {
        LOG_ERR("Failed to send subscription");
        close(sock);
        exit(1);
    }
    LOG_INFO("Subscription sent: %s", subscribe_msg);
    LOG_INFO("Waiting for events...");
    char buffer[MAX_BUFFER];
    int msg_count = 0;
    uint32_t last_seq = 0;
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
        msg_count++;
        char cmd[64], seq_str[32], recv_topic[64], content[MAX_MSG_LEN];
        if (split4(buffer, cmd, seq_str, recv_topic, content,
                   sizeof(cmd), sizeof(seq_str), sizeof(recv_topic), sizeof(content)) >= 3) {
            if (strcmp(cmd, "EVENT") == 0) {
                uint32_t seq = atol(seq_str);
                if (seq <= last_seq) {
                    LOG_WARN("Received [%d] DUPLICATE or OUT OF ORDER seq=%u (last=%u): %s",
                             msg_count, seq, last_seq, content);
                } else if (seq != last_seq + 1 && last_seq > 0) {
                    LOG_WARN("Received [%d] OUT OF ORDER seq=%u (expected %u): %s",
                             msg_count, seq, last_seq + 1, content);
                    last_seq = seq;
                } else {
                    LOG_INFO("Received [%d] seq=%u: %s", msg_count, seq, content);
                    last_seq = seq;
                }
                char ack_msg[64];
                snprintf(ack_msg, sizeof(ack_msg), "ACKSUB|%u", seq);
                sendto(sock, ack_msg, strlen(ack_msg) + 1, 0,
                       (struct sockaddr *)&remote_addr, sizeof(remote_addr));
            }
        }
    }
    close(sock);
    LOG_INFO("Subscriber finished. Total messages: %d", msg_count);
    return 0;
}
