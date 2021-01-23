#ifndef MAT_ARP_H
#define MAT_ARP_H

#include <inttypes.h>

#define ARP_ETYPE (uint16_t)0x0806

typedef struct __attribute__((__packed__)) {
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

typedef struct {
	uint8_t mac[6];
	uint8_t ip[4];
	uint16_t age;
} arp_entry_t;

uint8_t arp_make_request(arp_packet_t* arp, const uint8_t* myip, const uint8_t* mymac, const uint8_t* ip);
uint8_t arp_process_reply(const arp_packet_t* arp, const uint8_t* myip);
uint8_t arp_process_request(arp_packet_t* arp, const uint8_t* myip, const uint8_t* mymac);
uint8_t arp_update_entry(const uint8_t* ip, const uint8_t* mac);
void arp_add_entry(const uint8_t* ip, const uint8_t* mac);
uint8_t arp_find_entry(const uint8_t* ip, uint8_t* mac);
uint8_t arp_get_entry(uint16_t i, arp_entry_t* ae);
uint8_t arp_age(uint8_t sec);

#endif
