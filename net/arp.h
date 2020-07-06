#ifndef MAT_ARP_H
#define MAT_ARP_H

#include <inttypes.h>

#define ARP_ETYPE (uint16_t)0x0806

typedef struct {
	uint16_t htype; // 0x0001 = ethernet
	uint16_t ptype; // 0x0800 = IPv4
	uint8_t hadrlen; // 6 = size of MAC address
	uint8_t padrlen; // 4 = size of IPv4 address
	uint16_t oper; // 1 = request, 2 = response
	uint8_t sender_hadr[6]; // sender MAC
	uint8_t sender_padr[4]; // sender IP
	uint8_t target_hadr[6]; // target MAC
	uint8_t target_padr[4]; // target IP
} arp_packet_t;

uint8_t arp_request_make(arp_packet_t* arp, const uint8_t* myip, const uint8_t* mymac, const uint8_t* tip);
uint8_t arp_reply_make(arp_packet_t* arp, const uint8_t* myip, const uint8_t* mymac);
uint8_t arp_add_entry(uint8_t* ip, uint8_t* mac);
uint8_t arp_find_entry(uint8_t* ip, uint8_t* mac);

#endif
