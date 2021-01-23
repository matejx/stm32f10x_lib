#include "udp.h"

void udp_header_size_check(void)
{
	switch(0) {case 0:case sizeof(udp_header_t) == 8:;}
}

uint8_t udp_make_hdr(udp_header_t* udph, uint16_t srcport, uint16_t dstport, uint16_t len)
{
	udph->srcport = __builtin_bswap16(srcport);
	udph->dstport = __builtin_bswap16(dstport);
	udph->len = __builtin_bswap16(len + sizeof(udp_header_t));
	return 0;
}

uint16_t udp_pkt_len(udp_header_t* udph)
{
	return __builtin_bswap16(udph->len);
}
