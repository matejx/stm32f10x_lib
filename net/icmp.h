#ifndef MAT_ICMP_H
#define MAT_ICMP_H

#include <stdint.h>

#define ICMP_IPTYPE (uint8_t)1

typedef struct {
	uint8_t type;
	uint8_t code;
	uint16_t chksum;
	uint32_t data;
} icmp_header_t;

#define ICMP_TYPE_ECHO_REPLY 0
#define ICMP_TYPE_DESTINATION_UNREACHABLE 3
#define ICMP_TYPE_ECHO_REQUEST 8

#endif
