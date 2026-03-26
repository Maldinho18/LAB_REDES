# Sistema Publisher/Subscriber - Laboratorio 3

Implementación completa de un sistema de publicación-suscripción com**sockets en C** para distribuir eventos deportivos en tiempo real.

**Versiones incluidas:**
- TCP: Conexión persistente y confiable
- UDP: Datagramas, menor overhead
- Híbrido: UDP con confirmaciones y retransmisión
- QUIC: UDP con confiabilidad a nivel de aplicación (BONO)

## Requisitos

- gcc / clang
- Linux (WSL Ubuntu)
- Standard POSIX

## Compilación

```bash
# Todo
make all

# Solo TCP
make tcp

# Solo UDP
make udp

# Solo Híbrido
make hybrid

# Solo QUIC
make quic

# Limpiar
make clean
```

## Ejecución Rápida

### Demo Minimal TCP (4 terminales)

```bash
# Terminal 1
./broker_tcp 5000

# Terminal 2
./publisher_tcp 127.0.0.1 5000 PARTIDO_A PUB1 10 500

# Terminal 3
./subscriber_tcp 127.0.0.1 5000 PARTIDO_A SUB1
```

### Demo Minimal UDP (4 terminales)

```bash
# Terminal 1
./broker_udp 5001

# Terminal 2
./publisher_udp 127.0.0.1 5001 PARTIDO_A PUB1 10 500

# Terminal 3
./subscriber_udp 0.0.0.0 9000 127.0.0.1 5001 PARTIDO_A SUB1
```

### Demo Minimal Hybrid (4 terminales)

```bash
# Terminal 1
./broker_hybrid 5002

# Terminal 2
./publisher_hybrid 127.0.0.1 5002 PARTIDO_A PUB1 10 500

# Terminal 3
./subscriber_hybrid 0.0.0.0 9100 127.0.0.1 5002 PARTIDO_A SUB1
```

### Demo Minimal QUIC (4 terminales)

```bash
# Terminal 1
./broker_quic 5003

# Terminal 2
./publisher_quic 127.0.0.1 5003 PARTIDO_A PUB1 10 500

# Terminal 3
./subscriber_quic 0.0.0.0 9200 127.0.0.1 5003 PARTIDO_A SUB1
```

## Demo Completa (2 Publishers + 2 Subscribers)

### TCP

```bash
# Terminal 1: Broker
./broker_tcp 5000

# Terminal 2: Publisher 1 (PARTIDO_A)
./publisher_tcp 127.0.0.1 5000 PARTIDO_A PUB1 10 500

# Terminal 3: Publisher 2 (PARTIDO_B)
./publisher_tcp 127.0.0.1 5000 PARTIDO_B PUB2 10 500

# Terminal 4: Subscriber 1 (PARTIDO_A)
./subscriber_tcp 127.0.0.1 5000 PARTIDO_A SUB1

# Terminal 5: Subscriber 2 (PARTIDO_B)
./subscriber_tcp 127.0.0.1 5000 PARTIDO_B SUB2
```

### UDP

```bash
# Terminal 1
./broker_udp 5001

# Terminal 2
./publisher_udp 127.0.0.1 5001 PARTIDO_A PUB1 10 500

# Terminal 3
./publisher_udp 127.0.0.1 5001 PARTIDO_B PUB2 10 500

# Terminal 4
./subscriber_udp 0.0.0.0 9000 127.0.0.1 5001 PARTIDO_A SUB1

# Terminal 5
./subscriber_udp 0.0.0.0 9001 127.0.0.1 5001 PARTIDO_B SUB2
```

### Hybrid

```bash
# Terminal 1
./broker_hybrid 5002

# Terminal 2
./publisher_hybrid 127.0.0.1 5002 PARTIDO_A PUB1 10 500

# Terminal 3 (simular pérdida de ACK para seq 3)
DROP_ACK_SEQ=3 ./publisher_hybrid 127.0.0.1 5002 PARTIDO_B PUB2 10 500

# Terminal 4
./subscriber_hybrid 0.0.0.0 9100 127.0.0.1 5002 PARTIDO_A SUB1

# Terminal 5
./subscriber_hybrid 0.0.0.0 9101 127.0.0.1 5002 PARTIDO_B SUB2
```

### QUIC

```bash
# Terminal 1
./broker_quic 5003

# Terminal 2
./publisher_quic 127.0.0.1 5003 PARTIDO_A PUB1 10 500

# Terminal 3 (simular pérdida para demostración)
DROP_ACK_SEQ=5 ./publisher_quic 127.0.0.1 5003 PARTIDO_B PUB2 10 500

# Terminal 4
./subscriber_quic 0.0.0.0 9200 127.0.0.1 5003 PARTIDO_A SUB1

# Terminal 5
./subscriber_quic 0.0.0.0 9201 127.0.0.1 5003 PARTIDO_B SUB2
```

## Protocolo de Mensajes

### TCP y UDP

```
SUBSCRIBE|TOPIC           Suscribirse
PUBLISH|TOPIC|mensaje     Publicar
EVENT|TOPIC|mensaje       Evento distribuido
```

### Hybrid

```
DATA|seq|TOPIC|mensaje    Publicación con secuencia
ACKPUB|seq                ACK del broker
EVENT|seq|TOPIC|mensaje   Evento con secuencia
ACKSUB|seq                ACK del suscriptor
```

### QUIC

```
DATA|seq|TOPIC|mensaje         Publicación con secuencia
ACKPUB|seq|status              ACK (status: 0=recibido, 1=todos ACKed, -1=duplicado)
EVENT|seq|TOPIC|mensaje        Evento con secuencia
ACKSUB|seq                     ACK del suscriptor
```

## Estructura del Proyecto

```
.
├── common.h               Funciones utilitarias
├── broker_tcp.c           Broker TCP
├── publisher_tcp.c        Publisher TCP
├── subscriber_tcp.c       Subscriber TCP
├── broker_udp.c           Broker UDP
├── publisher_udp.c        Publisher UDP
├── subscriber_udp.c       Subscriber UDP
├── broker_hybrid.c        Broker Hybrid
├── publisher_hybrid.c     Publisher Hybrid
├── subscriber_hybrid.c    Subscriber Hybrid
├── broker_quic.c          Broker QUIC  [BONO]
├── publisher_quic.c       Publisher QUIC  [BONO]
├── subscriber_quic.c      Subscriber QUIC  [BONO]
├── Makefile               Compilación
├── README.md              Este archivo
└── INFORME_BASE.md        Plantilla informe
```

## Características

### TCP
✓ Garantía entrega  
✓ Orden preservado  
✓ Conexión persistente  
✓ Multi-threading  

### UDP
✓ Sin conexión  
✓ Bajo overhead  
✓ Rápido  
✓ No garantiza entrega  

### Hybrid
✓ UDP + confirmaciones  
✓ Números de secuencia  
✓ Detección duplicados  
✓ Retransmisión automática  
✓ DROP_ACK_SEQ para simulación  

### QUIC [BONO]
✓ UDP base  
✓ Confirmaciones end-to-end  
✓ Números de secuencia  
✓ Deduplicación  
✓ Detección orden/desorden  
✓ Confirmación múltiples suscriptores  

## Wireshark

### Capturar TCP
```bash
sudo tcpdump -i lo -w tcp_pubsub.pcap 'tcp port 5000'
```

### Capturar UDP
```bash
sudo tcpdump -i lo -w udp_pubsub.pcap 'udp port 5001'
```

### Capturar Hybrid
```bash
sudo tcpdump -i lo -w hybrid_pubsub.pcap 'udp port 5002'
```

### Capturar QUIC
```bash
sudo tcpdump -i lo -w quic_pubsub.pcap 'udp port 5003'
```

## Troubleshooting

**Puerto en uso:** Usar puerto diferente
```bash
./broker_tcp 5010
./publisher_tcp 127.0.0.1 5010 ...
```

**Conexión rechazada:** Asegurar broker está corriendo primero

**No hay mensajes:** Verificar subscriber esté suscrito ANTES de que publisher envíe

## Sustentación

Ver INFORME_BASE.md para guía completa
