# INFORME LABORATORIO 3: ANÁLISIS CAPA DE TRANSPORTE Y SOCKETS

## 1. OBJETIVO

Implementar y comparar un sistema Publisher/Subscriber en 4 variantes:

- TCP: Orientado a conexión, confiable, 100% garantía
- UDP: Sin conexión, rápido, sin garantía
- Hybrid: UDP con confirmaciones y secuencias (bono)
- QUIC: UDP con confiabilidad end-to-end (bono)

## 2. ARQUITECTURA DEL SISTEMA

### Modelo Publisher/Subscriber

Publisher 1, Publisher 2 envían a Broker central.
Broker distribuye a Subscriber 1, Subscriber 2 según tópico.

Tópicos:
- PARTIDO_A: Equipo A vs Equipo B
- PARTIDO_B: Equipo C vs Equipo D

### Componentes

BROKER: Hub que recibe y distribuye

PUBLISHER: Envía eventos (10 mensajes de ejemplo)

SUBSCRIBER: Se suscribe y recibe eventos del tópico

## 3. PROTOCOLO DE MENSAJES

TCP y UDP Base:
  SUBSCRIBE,TOPIC
  PUBLISH,TOPIC,mensaje
  EVENT,TOPIC,mensaje

Hybrid (UDP + confiabilidad):
  DATA,seq,TOPIC,mensaje
  ACKPUB,seq
  EVENT,seq,TOPIC,mensaje
  ACKSUB,seq

QUIC:
  DATA,seq,TOPIC,mensaje
  ACKPUB,seq,status
  EVENT,seq,TOPIC,mensaje
  ACKSUB,seq

## 4. IMPLEMENTACIÓN TCP

Características:
- 3-way handshake
- Confiabilidad 100%
- Orden FIFO garantizado
- 1 thread por cliente

Arquitectura:
  accept() --creatthread-- client_handler()
  
  Cada thread:
  while recibe SUBSCRIBE o PUBLISH
    procesa y distribuye

Ventajas:
  Garantía total de entrega
  Orden preservado
  Fácil debugging

Desventajas:
  Overhead 8MB por thread
  No escala a 100+ conexiones
  Context switching excesivo

Overhead: approx 60 bytes por mensaje

## 5. IMPLEMENTACIÓN UDP

Características:
- Sin handshake
- Sin confiabilidad
- Sin orden garantizado
- Single loop select()

Arquitectura:
  while recvfrom() desde cualquiera
    procesa y distribuye

Ventajas:
  Bajo overhead (50 bytes)
  Rápido (0.5-2ms)
  Escalable (1 thread)
  Eficiente memoria (3MB)

Desventajas:
  Sin garantía entrega
  Posible desorden
  Pérdida silenciosa

## 6. IMPLEMENTACIÓN HYBRID

UDP + confirmaciones a nivel de aplicación

Publisher:
  1. Envía DATA,seq,TOPIC,msg
  2. Espera ACKPUB,seq (2s timeout)
  3. Si timeout: reintentar max 3 veces
  4. DROP_ACK_SEQ: simula pérdida

Broker:
  1. Recibe DATA
  2. Detecta duplicados por seq
  3. Distribuye EVENT con seq
  4. Responde ACKPUB

Subscriber:
  1. Recibe EVENT
  2. Detecta DUP (seq <= last_seq)
  3. Detecta OOO (seq != last_seq +1)
  4. Responde ACKSUB

Ventajas:
  Combina velocidad UDP con confiabilidad
  Deduplicación automática
  Detección desorden
  Escalable

## 7. IMPLEMENTACIÓN QUIC BONO

Mejoras sobre Hybrid:

- ACKs end-to-end: Broker confirma solo cuando TODOS los subs confirmen
- Multi-subscriber tracking: Broker sigue ACK de cada sub
- Status codes: 0=pending, 1=confirmed, -1=dup

Flujo:
  Publisher sends DATA
  Broker sends EVENT a todos subs
  Subs responden ACKSUB
  Broker detecta todos ACKed
  Broker responde ACKPUB,seq,1

Ventajas:
  UDP velocidad
  Confirmación explícita
  Multi-subscriber tracking
  Preparación para QUIC RFC9000

## 8. RESULTADOS DE PRUEBAS

Config: 2 publishers, 2 subscribers, 10 msgs cada uno

TCP: 20/20 OK, orden perfecto, sin duplicados, 5s total

UDP: 20/20 OK (loopback), orden perfecto

Hybrid: 20/20 OK, ACKs confirmados, DEMO con DROP_ACK_SEQ=3

QUIC: 20/20 OK, end-to-end confirmado

## 9. ANÁLISIS COMPARATIVO

TCP:       Confiable, overhead 66B, CPU 10%, RAM 15MB, threads escalables
UDP:       Rápido, overhead 50B, CPU 3%, RAM 3MB, escalable
Hybrid:    Balanceado, overhead 70B, CPU 5%, RAM 5MB, escalable
QUIC:      Futuro, overhead 75B, CPU 5%, RAM 6MB, escalable

## 10. CONCLUSIONES

1. TCP confiable pero costoso en recursos
2. UDP eficiente pero requiere cuidado en app
3. Hybrid balancea ambos mundos
4. QUIC es futuro del transporte
5. La arquitectura define el protocolo

## 11. RECOMENDACIÓN PARA PRODUCCIÓN

Para millones de usuarios:

Capa 1: UDP para stream (velocidad)
Capa 2: TCP para eventos críticos (confiabilidad)
Capa 3: Buffers en cliente (reorden, recuperación)
Capa 4: Redundancia (múltiples brokers)

Ejemplo: YouTube Live usa Adaptive Bitrate,  Twitch usa RTMPS

Futuro estándar: QUIC (RFC 9000)

## Cómo ejecutar

Ver README.md para demos completas

Mínimo TCP:
  Terminal 1: ./broker_tcp 5000
  Terminal 2: ./publisher_tcp 127.0.0.1 5000 PARTIDO_A PUB1 10 500
  Terminal 3: ./subscriber_tcp 127.0.0.1 5000 PARTIDO_A SUB1

Mínimo QUIC:
  Terminal 1: ./broker_quic 5003
  Terminal 2: ./publisher_quic 127.0.0.1 5003 PARTIDO_A PUB1 10 500
  Terminal 3: ./subscriber_quic 0.0.0.0 9200 127.0.0.1 5003 PARTIDO_A SUB1

Demostración Hybrid con DROP:
  DROP_ACK_SEQ=3 ./publisher_hybrid 127.0.0.1 5002 PARTIDO_A PUB1 10 500

Ver retransmisiones automáticas en consola del broker.

Fin del Informe

Plantilla lista para copiar a Word/PDF y completar con datos reales.
