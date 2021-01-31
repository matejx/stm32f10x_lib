#ifndef MAT_UDP_H
#define MAT_UDP_H

#include <stdint.h>

#define UDP_IPTYPE (uint8_t)17

typedef struct __attribute__((__packed__)) {
	uint16_t srcport;
	uint16_t dstport;
	uint16_t len; // length of header + payload
	uint16_t chksum;
} udp_header_t;

uint8_t udp_make_hdr(udp_header_t* udph, uint16_t srcport, uint16_t dstport, uint16_t len);
uint16_t udp_pkt_len(udp_header_t* udph);
uint16_t udp_payload_len(udp_header_t* udph);

#endif
