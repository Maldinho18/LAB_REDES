#include "common.h"

int main(int argc, char *argv[]) {
    const char *local_host = "0.0.0.0";
    int local_port = 9000 + (rand() % 1000);
    const char *broker_host = DEFAULT_HOST;
    int broker_port = UDP_PORT;
    const char *topic = "PARTIDO_A";
    const char *sub_id = "SUB_1";
    
    if (argc > 1) local_host = argv[1];
    if (argc > 2) local_port = atoi(argv[2]);
    if (argc > 3) broker_host = argv[3];
    if (argc > 4) broker_port = atoi(argv[4]);
    if (argc > 5) topic = argv[5];
    if (argc > 6) sub_id = argv[6];
    
    LOG_INFO("=== SUBSCRIBER UDP STARTED ===");
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
    LOG_INFO("UDP socket bound to local port %d", local_port);
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
        LOG_INFO("Received [%d]: %s", msg_count, buffer);
    }
    close(sock);
    LOG_INFO("Subscriber finished. Total messages received: %d", msg_count);
    return 0;
}
