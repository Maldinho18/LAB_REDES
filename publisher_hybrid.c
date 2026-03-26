#include "common.h"

#define RETRY_TIMEOUT_MS 2000
#define MAX_RETRIES 3

int main(int argc, char *argv[]) {
    const char *host = DEFAULT_HOST;
    int port = HYBRID_PORT;
    const char *topic = "PARTIDO_A";
    const char *pub_id = "PUB_1";
    int num_msgs = 10;
    int delay_ms = 500;
    
    if (argc > 1) host = argv[1];
    if (argc > 2) port = atoi(argv[2]);
    if (argc > 3) topic = argv[3];
    if (argc > 4) pub_id = argv[4];
    if (argc > 5) num_msgs = atoi(argv[5]);
    if (argc > 6) delay_ms = atoi(argv[6]);
    
    const char *drop_ack_env = getenv("DROP_ACK_SEQ");
    int drop_ack_seq = drop_ack_env ? atoi(drop_ack_env) : -1;
    
    LOG_INFO("=== PUBLISHER HYBRID STARTED ===");
    LOG_INFO("Publisher ID: %s | Topic: %s | Messages: %d", pub_id, topic, num_msgs);
    if (drop_ack_seq >= 0) {
        LOG_WARN("SIMULATING LOSS: Will ignore ACKPUB for sequence %d", drop_ack_seq);
    }
    
    int sock = make_udp_client_socket();
    if (sock < 0) {
        LOG_ERR("Failed to create socket");
        exit(1);
    }
    struct sockaddr_in broker_addr = make_remote_addr(host, port);
    LOG_INFO("Target: %s:%d", host, port);
    
    const char *events[] = {
        "Gol de equipo A al minuto 12",
        "Cambio: jugador 10 entra por jugador 20",
        "Tarjeta amarilla al número 5",
        "Gol de equipo B al minuto 35",
        "Cambio: jugador 8 entra por jugador 15",
        "Tarjeta roja directa al número 12",
        "Gol de equipo A al minuto 67",
        "Cambio: portero entra",
        "Gol de equipo B al minuto 78",
        "Fin del partido"
    };
    
    for (int i = 0; i < num_msgs && i < 10; i++) {
        uint32_t seq = i + 1;
        char msg[MAX_BUFFER];
        snprintf(msg, sizeof(msg), "DATA|%u|%s|[%s] %s", seq, topic, pub_id, events[i]);
        int acked = 0;
        int retries = 0;
        while (!acked && retries < MAX_RETRIES) {
            LOG_INFO("Sending message %d (seq=%u), attempt %d/%d", 
                     i + 1, seq, retries + 1, MAX_RETRIES);
            if (sendto(sock, msg, strlen(msg) + 1, 0,
                       (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
                perror("sendto");
                break;
            }
            if (wait_for_readable(sock, RETRY_TIMEOUT_MS) > 0) {
                char ack_buf[64];
                struct sockaddr_in resp_addr;
                socklen_t addr_len = sizeof(resp_addr);
                int n = recvfrom(sock, ack_buf, sizeof(ack_buf) - 1, 0,
                                (struct sockaddr *)&resp_addr, &addr_len);
                if (n > 0) {
                    ack_buf[n] = '\0';
                    LOG_INFO("Received: %s", ack_buf);
                    char cmd[32], ack_seq_str[32];
                    char ack_extra[8];
                    if (split3(ack_buf, cmd, ack_seq_str, ack_extra,
                               sizeof(cmd), sizeof(ack_seq_str), sizeof(ack_extra)) >= 2) {
                        if (strcmp(cmd, "ACKPUB") == 0) {
                            uint32_t ack_seq = atol(ack_seq_str);
                            if (ack_seq == seq && ack_seq != (uint32_t)drop_ack_seq) {
                                LOG_INFO("ACK confirmed for seq=%u", seq);
                                acked = 1;
                            } else if (ack_seq == (uint32_t)drop_ack_seq) {
                                LOG_WARN("Simulator: ignoring ACK for seq=%u (DROP_ACK_SEQ)", seq);
                                retries++;
                            }
                        }
                    }
                }
            } else {
                LOG_WARN("Timeout waiting for ACK, seq=%u", seq);
                retries++;
            }
        }
        if (acked) {
            LOG_INFO("Message %d successfully delivered", i + 1);
        } else {
            LOG_WARN("Message %d failed after %d retries", i + 1, MAX_RETRIES);
        }
        if (i < num_msgs - 1) {
            usleep(delay_ms * 1000);
        }
    }
    LOG_INFO("All messages processed. Exiting...");
    close(sock);
    return 0;
}
