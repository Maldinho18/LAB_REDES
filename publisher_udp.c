#include "common.h"

int main(int argc, char *argv[]) {
    const char *host = DEFAULT_HOST;
    int port = UDP_PORT;
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
    
    LOG_INFO("=== PUBLISHER UDP STARTED ===");
    LOG_INFO("Publisher ID: %s | Topic: %s | Messages: %d", pub_id, topic, num_msgs);
    
    int sock = make_udp_client_socket();
    if (sock < 0) {
        LOG_ERR("Failed to create UDP socket");
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
        char msg[MAX_BUFFER];
        snprintf(msg, sizeof(msg), "PUBLISH|%s|[%s] %s", topic, pub_id, events[i]);
        int n = sendto(sock, msg, strlen(msg) + 1, 0,
                       (struct sockaddr *)&broker_addr, sizeof(broker_addr));
        if (n < 0) {
            LOG_ERR("Failed to send message %d", i + 1);
            perror("sendto");
            break;
        }
        LOG_INFO("Sent message %d/%d: %s", i + 1, num_msgs, events[i]);
        if (i < num_msgs - 1) {
            sleep_ms(delay_ms);
        }
    }
    LOG_INFO("All messages sent. Exiting...");
    close(sock);
    return 0;
}
