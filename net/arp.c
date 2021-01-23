#include "arp.h"

#include <string.h>

#ifndef ARP_TABLE_SIZE
#define ARP_TABLE_SIZE 16
#endif

#ifndef ARP_MAX_AGE
#define ARP_MAX_AGE 300
#endif

static arp_entry_t arp_table[ARP_TABLE_SIZE];
static uint16_t arp_table_cnt = 0;
static uint16_t arp_table_idx = 0;

void arp_packet_size_check(void)
{
	switch(0) {case 0:case sizeof(arp_packet_t) == 28:;}
}

uint8_t arp_make_request(arp_packet_t* arp, const uint8_t* myip, const uint8_t* mymac, const uint8_t* ip)
{
	arp->htype = 0x0100;
	arp->ptype = 0x0008;
	arp->hadrlen = 6;
	arp->padrlen = 4;
	arp->oper = 0x0100;
	memset(arp->target_hadr, 0, 6);
	memcpy(arp->target_padr, ip, 4);
	memcpy(arp->sender_hadr, mymac, 6);
	memcpy(arp->sender_padr, myip, 4);
	return 1;
}

uint8_t arp_process_reply(const arp_packet_t* arp, const uint8_t* myip)
{
	if(
		(arp->htype == 0x0100) &&
		(arp->ptype == 0x0008) &&
		(arp->hadrlen == 6) &&
		(arp->padrlen == 4) &&
		(arp->oper == 0x0200) ) {
		if( memcmp(arp->target_padr, myip, 4) == 0) {
			// ARP reply addressed to me, add sender to ARP table
			arp_add_entry(arp->sender_padr, arp->sender_hadr);
			return 1;
		}
		if( memcmp(arp->target_padr, arp->sender_padr, 4) == 0) {
			// gratuitous ARP, do not add, only update
			arp_update_entry(arp->sender_padr, arp->sender_hadr);
			return 1;
		}
	}
	return 0;
}

uint8_t arp_process_request(arp_packet_t* arp, const uint8_t* myip, const uint8_t* mymac)
{
	if(
		(arp->htype == 0x0100) &&
		(arp->ptype == 0x0008) &&
		(arp->hadrlen == 6) &&
		(arp->padrlen == 4) &&
		(arp->oper == 0x0100) ) {
		if( memcmp(arp->target_padr, myip, 4) == 0) {
			// ARP request addressed to me, might as well add sender to ARP table
			arp_add_entry(arp->sender_padr, arp->sender_hadr);
			// convert ARP request to response
			arp->oper = 0x0200;
			memcpy(arp->target_hadr, arp->sender_hadr, 6);
			memcpy(arp->target_padr, arp->sender_padr, 4);
			memcpy(arp->sender_hadr, mymac, 6);
			memcpy(arp->sender_padr, myip, 4);
			return 1; // return 1 to indicate packet should be sent
		}
		if( memcmp(arp->target_padr, arp->sender_padr, 4) == 0) {
			// ARP announcement, do not add, only update
			arp_update_entry(arp->sender_padr, arp->sender_hadr);
			return 0;
		}
	}
	return 0;
}

uint8_t arp_update_entry(const uint8_t* ip, const uint8_t* mac)
{
	uint16_t i;
	for( i = 0; i < arp_table_cnt; ++i ) {
		if( memcmp(arp_table[i].ip, ip, 4) == 0 ) {
			memcpy(arp_table[i].mac, mac, 6);
			arp_table[i].age = 0;
			return 1;
		}
	}
	return 0;
}

void arp_add_entry(const uint8_t* ip, const uint8_t* mac)
{
#ifdef ARP_PRIVATE_ONLY
	if( (*ip != 192) && (*ip != 172) && (*ip != 10) ) return;
#endif

	if( arp_update_entry(ip, mac) ) {
		return;
	}
	memcpy(arp_table[arp_table_idx].ip, ip, 4);
	memcpy(arp_table[arp_table_idx].mac, mac, 6);
	arp_table[arp_table_idx].age = 0;
	++arp_table_idx;
	arp_table_idx %= ARP_TABLE_SIZE;
	if( arp_table_cnt < ARP_TABLE_SIZE) ++arp_table_cnt;
}

uint8_t arp_find_entry(const uint8_t* ip, uint8_t* mac)
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

uint8_t arp_get_entry(uint16_t i, arp_entry_t* ae)
{
	if( i < arp_table_cnt ) {
		memcpy(ae, &arp_table[i], sizeof(arp_entry_t));
		return 1;
	}
	return 0;
}

uint8_t arp_age(uint8_t sec)
{
	uint16_t i;
	for( i = 0; i < arp_table_cnt; ++i ) {
		arp_table[i].age += sec;
		if( arp_table[i].age >= ARP_MAX_AGE ) {
			--arp_table_cnt;
			arp_table_idx = arp_table_cnt;
			if( i == arp_table_cnt ) break;
			memcpy(&arp_table[i], &arp_table[arp_table_cnt], sizeof(arp_entry_t));
			--i; // stay on this index
		}
	}
	return 1;
}
