#ifndef MAT_UDP_H
#define MAT_UDP_H

#include <stdint.h>

#define UDP_IPTYPE (uint8_t)17

typedef struct {
	uint16_t srcport;
	uint16_t dstport;
	uint16_t len; // length of header + payload
	uint16_t chksum;
} udp_header_t;

uint8_t udp_hdr_make(udp_header_t* udph, uint16_t srcport, uint16_t dstport, uint16_t len);
uint16_t udp_pkt_len(udp_header_t* udph);

#endif
