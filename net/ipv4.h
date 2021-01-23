#ifndef MAT_IPV4_H
#define MAT_IPV4_H

#include <stdint.h>

#define IPV4_ETYPE (uint16_t)0x0800

typedef struct __attribute__((__packed__)) {
	uint8_t version_ihl; // version (high nibble) = 4, header len (low nibble) = 5
	uint8_t dscp_ecn; // = 0
	uint16_t len; // length of header + payload

	uint16_t iden; // no meaning for atomic (non fragmented) datagrams
	uint16_t frag_ofs; // 0x4000 for DF datagrams

	uint8_t ttl; // normally 64
	uint8_t proto; // specifies the encapsulated protocol
	uint16_t chksum;

	uint8_t srcip[4];
	uint8_t dstip[4];

	//uint32_t options[10];
} ipv4_header_t;

uint8_t ipv4_make_hdr(ipv4_header_t* iph, uint16_t len, uint8_t proto, const uint8_t* srcip, const uint8_t* dstip);
uint8_t ipv4_hdr_len(ipv4_header_t* iph);
uint16_t ipv4_pkt_len(ipv4_header_t* iph);
uint16_t ipv4_payload_len(ipv4_header_t* iph);

#endif
