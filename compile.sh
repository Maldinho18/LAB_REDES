#!/bin/bash
# Script de compilación para Lab 3

echo "=== Compilando Laboratorio 3 ==="

# TCP
echo "Compilando TCP..."
gcc -Wall -Wextra -pthread -std=c99 -O2 -o broker_tcp broker_tcp.c -pthread
gcc -Wall -Wextra -pthread -std=c99 -O2 -o publisher_tcp publisher_tcp.c -pthread
gcc -Wall -Wextra -pthread -std=c99 -O2 -o subscriber_tcp subscriber_tcp.c -pthread
echo "TCP OK"

# UDP
echo "Compilando UDP..."
gcc -Wall -Wextra -pthread -std=c99 -O2 -o broker_udp broker_udp.c -pthread
gcc -Wall -Wextra -pthread -std=c99 -O2 -o publisher_udp publisher_udp.c -pthread
gcc -Wall -Wextra -pthread -std=c99 -O2 -o subscriber_udp subscriber_udp.c -pthread
echo "UDP OK"

# Hybrid
echo "Compilando Hybrid..."
gcc -Wall -Wextra -pthread -std=c99 -O2 -o broker_hybrid broker_hybrid.c -pthread
gcc -Wall -Wextra -pthread -std=c99 -O2 -o publisher_hybrid publisher_hybrid.c -pthread
gcc -Wall -Wextra -pthread -std=c99 -O2 -o subscriber_hybrid subscriber_hybrid.c -pthread
echo "Hybrid OK"

# QUIC (Bono)
echo "Compilando QUIC (Bono)..."
gcc -Wall -Wextra -pthread -std=c99 -O2 -o broker_quic broker_quic.c -pthread
gcc -Wall -Wextra -pthread -std=c99 -O2 -o publisher_quic publisher_quic.c -pthread
gcc -Wall -Wextra -pthread -std=c99 -O2 -o subscriber_quic subscriber_quic.c -pthread
echo "QUIC OK"

echo ""
echo "=== Compilación completada ==="
echo ""
ls -lh broker_* publisher_* subscriber_* 2>/dev/null | wc -l
echo "ejecutables creados"
