#include "ipv4.h"

#include <string.h>

void ipv4_header_size_check(void)
{
	switch(0) {case 0:case sizeof(ipv4_header_t) == 20:;}
}

uint8_t ipv4_hdr_make(ipv4_header_t* iph, uint16_t len, uint8_t proto, const uint8_t* srcip, const uint8_t* dstip)
{
	iph->version_ihl = 0x45; // version = 4, header len = 5 words (word = 32 bits)
	iph->dscp_ecn = 0;
	iph->len = __builtin_bswap16(len + ipv4_hdr_hdrlen(iph));

	iph->frag_ofs = __builtin_bswap16(0x4000); // set do not fragment flag
	iph->iden = 0; // in atomic datagrams IPv4 ID field has no meaning (RFC 6864)

	iph->ttl = 64;
	iph->proto = proto;
	iph->chksum = 0; // will be calculated by MAC

	memcpy(iph->srcip, srcip, 4);
	memcpy(iph->dstip, dstip, 4);

	return 0;
}

uint8_t ipv4_hdr_hdrlen(ipv4_header_t* iph)
{
	return (iph->version_ihl & 0x0f)*4;
}

uint16_t ipv4_hdr_pktlen(ipv4_header_t* iph)
{
	return __builtin_bswap16(iph->len);
}

uint16_t ipv4_hdr_payloadlen(ipv4_header_t* iph)
{
	return ipv4_hdr_pktlen(iph)-ipv4_hdr_hdrlen(iph);
}
