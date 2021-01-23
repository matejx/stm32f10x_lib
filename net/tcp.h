#ifndef MAT_TCP_H
#define MAT_TCP_H

#include <stdint.h>

#define TCP_IPTYPE (uint8_t)6

typedef struct __attribute__((__packed__)) {
	uint16_t srcport;
	uint16_t dstport;
	uint32_t seqnum;
	uint32_t acqnum;
	uint8_t dataofs;
	uint8_t flags;
	uint16_t winsize;
	uint16_t chksum;
	uint16_t urgptr;
} tcp_header_t;

uint16_t tcp_hdr_len(tcp_header_t* tcph);

#endif
