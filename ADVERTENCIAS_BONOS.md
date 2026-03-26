# ADVERTENCIAS Y BONOS

## BONO 1: VERSIÓN QUIC-LIKE (25 PUNTOS) - IMPLEMENTADO

estado: INCLUIDO en el proyecto

Archivos:
- broker_quic.c
- publisher_quic.c  
- subscriber_quic.c

Características implementadas:
- ACKs: Cada mensaje publicado genera confirmación del broker
- Retransmisión: Si falta ACK en 2 segundos, reintentar (máx 3 veces)
- Orden: Números de secuencia de 32-bit
- Duplicados: Broker detecta y evita redistribuir
- Subscriber: Detecta duplicados y desorden por secuencia
- Simulación: Variable DROP_ACK_SEQ=N para demostrar pérdida

Diferencia vs Hybrid:
- QUIC: ACKPUBstatus=0 (pending) o 1 (todos confirmaron end-to-end)
- QUIC: Broker rastrea ACK de cada subscriber
- QUIC: Confirmación end-to-end (espera a TODOS los subs)
- QUIC: Más cercano a QUIC oficial (RFC 9000)

Cómo ejecutar:
  Terminal 1: ./broker_quic 5003
  Terminal 2: ./publisher_quic 127.0.0.1 5003 PARTIDO_A PUB1 10 500
  Terminal 3: ./subscriber_quic 0.0.0.0 9200 127.0.0.1 5003 PARTIDO_A SUB1

Esperado en Wireshark (filtro udp.port == 5003):
  Frame: DATA|1|PARTIDO_A|...
  Frame: ACKPUB|1|0 (broker recibió, pero subs pending)
  Frame: EVENT|1|PARTIDO_A|...
  Frame: ACKSUB|1 (sub confirmó)
  Frame: ACKPUB|1|1 (broker notifica: todos confirmaron)

---

## BONO 2: VERSIÓN EN ASSEMBLER (25 PUNTOS) - OPCIÓN VIABILIDAD

ADVERTENCIA: Este bono es OPCIONALMENTE oral y tiene RESTRICCIONES SEVERAS

### Requisitos del bono oral de Assembler:

LITERAL: "Si programan TODO el lab 3 en assembler, no pueden ser unas cosas en C 
y otras en assembler. Debe ser TODO en assembler."

Esto significa:
- TODO el proyecto en assembler x86-64
- NO mezclar C con assembler
- Incluye: sockets POSIX, threads, mutex, manejo de memoria
- Incluye: parseo de mensajes, lógica de distribución
- Incluye: TCP connect/bind/listen/accept, UDP sendto/recvfrom
- Incluye: pthread_create, pthread_mutex, etc.

### Análisis de viabilidad:

DIFICULTAD: Extremadamente Alta
TIEMPO ESTIMADO: 40-60 horas
RIESGO DE FALLOS: Alto
BENEFICIO: 25 puntos extra (pero proyecto debe estar 100% funcional)

### Razones por las que NO lo recomendamos:

1. **Complejidad POSIX en assembler:**
   
   Hacer socket en C:
   ```c
   int sock = socket(AF_INET, SOCK_STREAM, 0);
   ```
   
   Equivalente en gas assembler (x86-64):
   ```asm
   mov rax, 41           ; socket syscall
   mov rdi, 2            ; AF_INET
   mov rsi, 1            ; SOCK_STREAM
   mov rdx, 0
   syscall               ; resultado en rax
   ```
   
   Pero luego necesitas struct sockaddr_in en memoria, bind, listen, etc.
   TODO en assembler = 1000+ líneas solo para setup.

2. **Pthread en assembler:**
   
   Llamar pthread_create en C:
   ```c
   pthread_create(&tid, NULL, handler, args);
   ```
   
   En assembler necesitas:
   - Alinear stack (x86-64 exige 16-byte alignment)
   - Pasar argumentos en RDI, RSI, RDX, RCX, R8, R9
   - Guardar registros volatiles
   - Manejar return values
   - Libnpthread linking
   
   MUCHO MÁS complicado que en C.

3. **Manejo de memoria:**
   
   malloc(), free(), strings en C son simples.
   
   En assembler:
   - Escribir malloc() tu mismo es 100+ líneas
   - O llamar libc malloc pero necesitas ABI compleja
   - Errors son silenciosos y fatales

4. **Debugging casi imposible:**
   
   Error en C: `segmentation fault line 45`
   Fácil de debuggear con gdb
   
   Error en assembler: `segmentation fault at 0x123456ab`
   Necesitas disassgembler + profundo conocimiento de ABI

5. **Incompatibilidad POSIX:**
   
   Muchas funciones de POSIX requirenSpecifications de syscall,
   heap management, stack management, signal handling.
   
   Implementar TODO esto en assembler puro es viable solo para
   proyectos académicos muy específicos, NO para aplicaciones
   prácticas como este sistema.

6. **100% en assembler es NO-SOLO:**
   
   No puedes usar:
   -libc printf() (necesitarías syscall write())
   - No puedes usar libpthread (necesitarías syscall clone())
   - No puedes usar libsocket (necesitarías syscall socket/bind/etc)
   
   Resultaría en código de 5000+ líneas solo para setup básico.

### Si REALMENTE quieres intentarlo:

Recomendación: Hazlo SOLO si:
- Ya dominas x86-64 assembler profundamente
- Tienes 40+ horas disponibles
- Aceptas que puede no funcionar en time
- Primero termina proyecto en C (garantizado)
- El assembler es "extra" sobre C ya funcional

Toolkit mínimo necesario:
- NASM (Netwide Assembler) o GAS
- gdb (GNU Debugger)
- strace (system call tracer)
- objdump (disassembler)
- Linux man pages (syscall numbers, ABI)

Pseudocódigo mínimo de architecture:

```asm
; main.asm
extern __libc_start_main

section .text
global _start

_start:
  ; Setup stack
  ; Call libc_start_main con main()
  
main:
  ; Aquí inicia tu código
  ; syscall 1: write (printf)
  mov rax, 1
  mov rdi, 1           ; stdout
  mov rsi, msg         ; mensaje
  mov rdx, msg_len     ; longitud
  syscall
  
  ; syscall 41: socket
  ; TODO socket operations en asm
  
  ; pthread_create (si usas libc)
  ; TODO threading en asm
  
  ; TODO: TODO el broker/pub/sub logic aquí
  
  ; exit
  mov rax, 60          ; exit syscall
  xor rdi, rdi         ; exit code 0
  syscall
```

Líneas totales: ~10,000+ lines para proyecto completo

### DECISIÓN RECOMENDADA:

Entregar:
1. Proyecto C completo (12 archivos) - GARANTIZADO
2. Proyecto QUIC bono (3 archivos) - GARANTIZADO
3. **NO** proyecto assembler

Total de puntos obtenibles sin riesgo:
- Base: 100 puntos
- Bono QUIC: 25 puntos
- Total: 125 puntos

Si intentas assembler y falla:
- Proyecto C: 100 puntos
- Bono QUIC: 25 puntos
- Bono Assembler: 0 puntos (no funciona)
- Total: 125 puntos (pero con riesgo)

---

## RESUMEN DE BONOS DISPONIBLES

### BONO 1: Versión Hybrid (5-10 horas)
estado: INCLUIDO
riesgo: BAJO
puntos: Asume ~15 puntos (opcional según rubrica)
Recomendación: HACER

### BONO 2: Versión QUIC (5-10 horas)
estado: INCLUIDO
riesgo: BAJO
puntos: +25 puntos
Recomendación: HACER

### BONO 3: Versión Assembler (40-60 horas)
estado: NO INCLUIDO
riesgo: MUY ALTO
puntos: +25 puntos (si funciona 100%)
Recomendación: NO HACER (a menos que domines assembler)

---

## CHECKLIST FINAL ANTES DE ENTREGAR

Código:
  ✓ common.h compila sin errors
  ✓ broker_tcp.c, publisher_tcp.c, subscriber_tcp.c compilan
  ✓ broker_udp.c, publisher_udp.c, subscriber_udp.c compilan
  ✓ broker_hybrid.c, publisher_hybrid.c, subscriber_hybrid.c compilan
  ✓ broker_quic.c, publisher_quic.c, subscriber_quic.c compilan

Funcionalidad:
  ✓ Demo TCP funciona: broker + pub + sub = eventos recibidos
  ✓ Demo UDP funciona: 10/10 mensajes
  ✓ Demo Hybrid funciona: DROP_ACK_SEQ=3 causa retransmisión
  ✓ Demo QUIC funciona: Confirmación end-to-end
  ✓ Wireshark captura correcta en cada protocolo

Documentación:
  ✓ README.md incluye instrucciones
  ✓ INFORME_BASE.md y RESUMEN_EJECUCION.md completados
  ✓ Código comentado
  ✓ Makefile funcional / compile.sh funciona

Entrega:
  ✓ Archivos .c, .h, Makefile en repo
  ✓ INFORME_FINAL.pdf generado desde INFORME_BASE.md
  ✓ Capturas Wireshark: tcp_pubsub.pcap, udp_pubsub.pcap, hybrid_pubsub.pcap, quic_pubsub.pcap
  ✓ Logs de ejecución guardados

Sustentación:
  ✓ Puedo explicar TCP handshake
  ✓ Puedo explicar UDP sin conexión
  ✓ Puedo explicar ACKs y retransmisión en Hybrid
  ✓ Puedo ejecutar en vivo las 4 versiones
  ✓ Puedo mostrar Wireshark y explicar diferencias

---

**FIN DE ADVERTENCIAS Y BONOS**

Para dudas contacta: n.quiroga@uniandes.edu.co
