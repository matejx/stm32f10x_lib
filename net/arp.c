#include "arp.h"

#include <string.h>

#ifndef ARP_TABLE_SIZE
#define ARP_TABLE_SIZE 16
#endif

typedef struct {
	uint8_t mac[6];
	uint8_t ip[4];
} arp_entry_t;

static arp_entry_t arp_table[ARP_TABLE_SIZE];
static uint16_t arp_table_cnt = 0;
static uint16_t arp_table_idx = 0;

void arp_packet_size_check(void)
{
	switch(0) {case 0:case sizeof(arp_packet_t) == 28:;}
}

uint8_t arp_request_make(arp_packet_t* arp, const uint8_t* myip, const uint8_t* mymac, const uint8_t* tip)
{
	arp->htype = 0x0100;
	arp->ptype = 0x0008;
	arp->hadrlen = 6;
	arp->padrlen = 4;
	arp->oper = 0x0100;
	memset(arp->target_hadr, 0, 6);
	memcpy(arp->target_padr, tip, 4);
	memcpy(arp->sender_hadr, mymac, 6);
	memcpy(arp->sender_padr, myip, 4);
	return 0;
}

uint8_t arp_reply_make(arp_packet_t* arp, const uint8_t* myip, const uint8_t* mymac)
{
	if(
		(arp->htype == 0x0100) &&
		(arp->ptype == 0x0008) &&
		(arp->hadrlen == 6) &&
		(arp->padrlen == 4) &&
		(arp->oper == 0x0100) &&
		(memcmp(arp->target_padr, myip, 4) == 0) ) {
			// set arp operation to arp response
			arp->oper = 0x0200;
			// copy sender mac address to target mac address
			memcpy(arp->target_hadr, arp->sender_hadr, 6);
			// copy sender ip address to target ip address
			memcpy(arp->target_padr, arp->sender_padr, 4);
			// copy my mac address to sender mac address
			memcpy(arp->sender_hadr, mymac, 6);
			// copy my ip address to sender ip address
			memcpy(arp->sender_padr, myip, 4);

			return 0;
	}
	return 1;
}

uint8_t arp_add_entry(uint8_t* ip, uint8_t* mac)
{
	if( !arp_find_entry(ip, 0) ) {
		memcpy(arp_table[arp_table_idx].ip, ip, 4);
		memcpy(arp_table[arp_table_idx].mac, mac, 6);
		++arp_table_idx;
		arp_table_idx %= ARP_TABLE_SIZE;
		if( arp_table_cnt < ARP_TABLE_SIZE) ++arp_table_cnt;
	}
	return 1;
}

uint8_t arp_find_entry(uint8_t* ip, uint8_t* mac)
{
	uint16_t i;
	for( i = 0; i < arp_table_cnt; ++i ) {
		if( memcmp(arp_table[i].ip, ip, 4) == 0 ) {
			if( mac ) memcpy(mac, arp_table[i].mac, 6);
			return 1;
		}
	}
	return 0;
}
