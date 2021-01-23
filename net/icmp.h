#ifndef MAT_ICMP_H
#define MAT_ICMP_H

#include <stdint.h>

#define ICMP_IPTYPE (uint8_t)1

typedef struct __attribute__((__packed__)) {
	uint8_t type;
	uint8_t code;
	uint16_t chksum;
	uint32_t data;
} icmp_header_t;

#define ICMP_TYPE_ECHO_REPLY 0
#define ICMP_TYPE_DEST_UNREACH 3
#define ICMP_TYPE_ECHO_REQUEST 8

#define ICMP_CODE_DEST_NET_UNREACH 0
#define ICMP_CODE_DEST_HOST_UNREACH 1
#define ICMP_CODE_DEST_PROTO_UNREACH 2
#define ICMP_CODE_DEST_PORT_UNREACH 3

#endif
