CC = gcc
CFLAGS = -Wall -Wextra -pthread -std=c99 -O2
LDFLAGS = -pthread

TARGETS_TCP = broker_tcp publisher_tcp subscriber_tcp
TARGETS_UDP = broker_udp publisher_udp subscriber_udp
TARGETS_HYBRID = broker_hybrid publisher_hybrid subscriber_hybrid
TARGETS_QUIC = broker_quic publisher_quic subscriber_quic

ALL_TARGETS = $(TARGETS_TCP) $(TARGETS_UDP) $(TARGETS_HYBRID) $(TARGETS_QUIC)

.PHONY: all tcp udp hybrid quic clean help

all: $(ALL_TARGETS)

tcp: $(TARGETS_TCP)

udp: $(TARGETS_UDP)

hybrid: $(TARGETS_HYBRID)

quic: $(TARGETS_QUIC)

broker_tcp: broker_tcp.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

publisher_tcp: publisher_tcp.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

subscriber_tcp: subscriber_tcp.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

broker_udp: broker_udp.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

publisher_udp: publisher_udp.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

subscriber_udp: subscriber_udp.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

broker_hybrid: broker_hybrid.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

publisher_hybrid: publisher_hybrid.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

subscriber_hybrid: subscriber_hybrid.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

broker_quic: broker_quic.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

publisher_quic: publisher_quic.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

subscriber_quic: subscriber_quic.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(ALL_TARGETS)

help:
	@echo "Targets:"
	@echo "  make all       - All (TCP+UDP+Hybrid+QUIC)"
	@echo "  make tcp       - TCP only"
	@echo "  make udp       - UDP only"
	@echo "  make hybrid    - Hybrid only"
	@echo "  make quic      - QUIC only"
	@echo "  make clean     - Delete executables"
