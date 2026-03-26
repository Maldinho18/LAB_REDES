#include "common.h"

int main(int argc, char *argv[]) {
    const char *host = DEFAULT_HOST;
    int port = TCP_PORT;
    const char *topic = "PARTIDO_A";
    const char *sub_id = "SUB_1";
    
    if (argc > 1) host = argv[1];
    if (argc > 2) port = atoi(argv[2]);
    if (argc > 3) topic = argv[3];
    if (argc > 4) sub_id = argv[4];
    
    LOG_INFO("=== SUBSCRIBER TCP STARTED ===");
    LOG_INFO("Subscriber ID: %s | Topic: %s", sub_id, topic);
    
    int sock = connect_tcp_client(host, port);
    if (sock < 0) {
        LOG_ERR("Failed to connect to broker at %s:%d", host, port);
        exit(1);
    }
    LOG_INFO("Connected to broker at %s:%d", host, port);
    
    char subscribe_msg[MAX_BUFFER];
    snprintf(subscribe_msg, sizeof(subscribe_msg), "SUBSCRIBE|%s", topic);
    
    if (send_line(sock, subscribe_msg) < 0) {
        LOG_ERR("Failed to send subscription");
        close(sock);
        exit(1);
    }
    LOG_INFO("Subscription sent: %s", subscribe_msg);
    LOG_INFO("Waiting for events...");
    
    char buffer[MAX_BUFFER];
    int msg_count = 0;
    
    while (1) {
        int n = recv_line(sock, buffer, sizeof(buffer));
        if (n <= 0) {
            LOG_INFO("Broker disconnected");
            break;
        }
        trim_newline(buffer);
        msg_count++;
        LOG_INFO("Received [%d]: %s", msg_count, buffer);
    }
    
    close(sock);
    LOG_INFO("Subscriber finished. Total messages received: %d", msg_count);
    return 0;
}
