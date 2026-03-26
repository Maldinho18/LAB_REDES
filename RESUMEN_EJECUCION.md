# RESUMEN EJECUCION - LAB 3: PUBLISHER SUBSCRIBER CON SOCKETS

Este documento es tu GUIA RAPIDA para compilar y ejecutar TODO el laboratorio.

## ARCHIVOS DEL PROYECTO

Total: 17 archivos

Headers Compartidos:
- common.h (7.3K): Funciones utilitarias POSIX

TCP (3 pares):
- broker_tcp.c (5.2K)
- publisher_tcp.c (1.9K)
- subscriber_tcp.c (1.5K)

UDP (3 pares):
- broker_udp.c (4.4K)
- publisher_udp.c (2.0K)
- subscriber_udp.c (2.4K)

Hybrid (3 pares):
- broker_hybrid.c (6.2K)
- publisher_hybrid.c (4.0K)
- subscriber_hybrid.c (3.5K)

QUIC BONO (3 pares):
- broker_quic.c (7.8K)
- publisher_quic.c (4.1K)
- subscriber_quic.c (3.4K)

Build & Docs:
- Makefile (1.8K)
- compile.sh (1.5K)
- README.md (5.8K)
- INFORME_BASE.md (4.8K)
- RESUMEN_EJECUCION.md (este archivo)

---

## PASO 1: PREPARAR EN WSL

1.1 Abrir WSL Ubuntu desde Windows Terminal

1.2 Navegar al directorio:
  cd /mnt/c/ANDES/BI/LAB_REDES
  ls -la

1.3 Verificar compilador:
  gcc --version

Si no está:
  sudo apt update
  sudo apt install build-essential

---

## PASO 2: COMPILAR

Opción A: Script automático (RECOMENDADO)
  bash compile.sh

Opción B: Manualmente
  gcc -Wall -Wextra -pthread -std=c99 -O2 -o broker_tcp broker_tcp.c -pthread
  (y así para cada archivo)

Verificar:
  ls -lh broker_* | wc -l

---

## PASO 3: EJECUTAR DEMOS

### DEMO TCP MINIMO (3 terminales)

Terminal 1: Broker
  ./broker_tcp 5000

Terminal 2: Publisher
  ./publisher_tcp 127.0.0.1 5000 PARTIDO_A PUB1 10 500

Terminal 3: Subscriber
  ./subscriber_tcp 127.0.0.1 5000 PARTIDO_A SUB1

Esperado: 10 mensajes recibidos sin duplicados, en orden

---

### DEMO UDP MINIMO (3 terminales)

Terminal 1: Broker
  ./broker_udp 5001

Terminal 2: Publisher
  ./publisher_udp 127.0.0.1 5001 PARTIDO_A PUB1 10 500

Terminal 3: Subscriber
  ./subscriber_udp 0.0.0.0 9000 127.0.0.1 5001 PARTIDO_A SUB1

Notar: Más rápido que TCP (~3s vs ~5s)

---

### DEMO HYBRID CON DROP (3 terminales)

Terminal 1: Broker
  ./broker_hybrid 5002

Terminal 2: Publisher SIN drop
  ./publisher_hybrid 127.0.0.1 5002 PARTIDO_A PUB1 10 500

Terminal 3: Publisher CON drop simulado para seq 3
  DROP_ACK_SEQ=3 ./publisher_hybrid 127.0.0.1 5002 PARTIDO_B PUB2 10 500

Terminal 3 (agregada): Subscriber
  ./subscriber_hybrid 0.0.0.0 9100 127.0.0.1 5002 PARTIDO_A SUB1

En Terminal 3 verás las retransmisiones:
  [WARN] Timeout waiting for ACK seq=3
  [INFO] Sending message 3 (seq=3) attempt 2/3
  [WARN] Simulator: ignoring ACK for seq=3

---

### DEMO QUIC BONO (3 terminales)

Terminal 1: Broker
  ./broker_quic 5003

Terminal 2: Publisher
  ./publisher_quic 127.0.0.1 5003 PARTIDO_A PUB1 5 500

Terminal 3: Subscriber
  ./subscriber_quic 0.0.0.0 9200 127.0.0.1 5003 PARTIDO_A SUB1

Diferencia: Confirmación end-to-end (espera que TODOS los subs confirmen)

---

## PASO 4: CAPTURAR CON WIRESHARK

Abrir Wireshark:
  sudo wireshark &

Seleccionar interfaz: lo (loopback)

Filtros:
  TCP:    tcp.port == 5000
  UDP:    udp.port == 5001
  Hybrid: udp.port == 5002
  QUIC:   udp.port == 5003

Guardar capturas:
  File > Export As > tcp_pubsub.pcap
  File > Export As > udp_pubsub.pcap
  File > Export As > hybrid_pubsub.pcap
  File > Export As > quic_pubsub.pcap

---

## PASO 5: VERIFICAR LOGS

Todos los programas imprimen logs con timestamp:
  [HH:MM:SS] [INFO] Nuevo evento
  [HH:MM:SS] [WARN] Advertencia
  [HH:MM:SS] [ERROR] Error

Guardar logs:
  ./broker_tcp 5000 > broker_tcp.log 2>&1 &
  ./publisher_tcp 127.0.0.1 5000 PARTIDO_A PUB1 10 500 > pub_tcp.log 2>&1
  ./subscriber_tcp 127.0.0.1 5000 PARTIDO_A SUB1 > sub_tcp.log 2>&1

---

## COMMANDOS RAPIDOS

Compilar todo:
  bash compile.sh

Demo TCP:
  Terminal 1: ./broker_tcp 5000
  Terminal 2: ./publisher_tcp 127.0.0.1 5000 PARTIDO_A PUB1 10 500
  Terminal 3: ./subscriber_tcp 127.0.0.1 5000 PARTIDO_A SUB1

Demo Hybrid con drop:
  ./broker_hybrid 5002 &
  DROP_ACK_SEQ=3 ./publisher_hybrid 127.0.0.1 5002 PARTIDO_A PUB1 10 500 &
  ./subscriber_hybrid 0.0.0.0 9100 127.0.0.1 5002 PARTIDO_A SUB1

Demo QUIC:
  ./broker_quic 5003 &
  ./publisher_quic 127.0.0.1 5003 PARTIDO_A PUB1 5 500 &
  ./subscriber_quic 0.0.0.0 9200 127.0.0.1 5003 PARTIDO_A SUB1

---

## TROUBLESHOOTING

Puerto ya en uso:
  Error: bind: Address already en use
  Solución: Usar diferente puerto o esperar 1 minuto

Conexión rechazada:
  Error: connect: Connection refused
  Solución: Asegurar que broker esté corriendo primero

No hay mensajes:
  Problema: Subscriber se inicia después que publisher
  Solución: Iniciar subscriber ANTES que publisher

Error de compilación:
  error: undefined reference a `pthread_create'
  Solución: Compilar con -pthread flag

---

## ENTREGA FINAL

Archivos necesarios:
1. Código fuente (12 .c + 1 .h)
2. Makefile
3. README.md
4. INFORME_FINAL.pdf (rellenado con datos reales)
5. Capturas Wireshark (.pcap)
6. Logs de ejecución

Bonos:
- Hybrid (obligatorio si quieres 100%)
- QUIC (extra 25 puntos)
- Assembler (extra 25 puntos, pero TODO en assembler, no mixto)

---

Generado: 2026-03-26
Laboratorio 3 - Infraestructura de Comunicaciones
Universidad de los Andes
