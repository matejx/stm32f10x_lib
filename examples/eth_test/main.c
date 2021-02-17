/**
This is a demo of a minimal UDP capable IoT device. The code was developed and
tested on MIKROE EasyMX PRO v7 MCU card with STM32F107VC and LAN8720A PHY.

From the standard suite of protocols,
ARP, IP, ICMP and UDP are implemented,
TCP, DHCP, DNS are NOT implemented.

The devices MAC address is obtained from the MCU's unique chip ID.
Since DHCP is not supported, the device's IP address must be set statically.

When a packet is destined for an unknown MAC address, the packet is thrown
away and an ARP request is generated instead. This means that the very first
packet (ICMP ping or UDP) to a new address will be lost. This is normal.

Debug messages and AT commands are available on the UART1 serial interface @115200 baud.
A list of supported AT commands can be requested by issuing "AT?".

@file		main.c
@brief		eth test
@author		Matej Kogovsek (matej@hamradio.si)
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
*/

#include "stm32f10x.h"

#include "mat/circbuf8.h"
#include "mat/serialq.h"
#include "mat/eth.h"

#include <string.h>
#include <ctype.h>

#include "mat/misc.h"
#include "mat/itoa.h"
#include "mat/fifo.h"
#include "mat/net/arp.h"
#include "mat/net/ipv4.h"
#include "mat/net/icmp.h"
#include "mat/net/udp.h"
#include "mat/net/tcp.h"

//-----------------------------------------------------------------------------
//  Defines
//-----------------------------------------------------------------------------

#define TMR_ID_DELAY 0
#define TMR_ID_LED 1
#define TMR_ID_ARP_REQ 2
#define TMR_ID_SIZE 3

#define LED_PORT GPIOD
#define LED_PIN GPIO_Pin_3

#define PHY_INT_PORT GPIOB
#define PHY_INT_PIN GPIO_Pin_14

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

static const uint8_t AT_CMD_UART = 1;
static const uint32_t AT_CMD_BAUD = 115200;

static const uint16_t EPH_PORT_BEGIN = 49152;
static const uint16_t EPH_PORT_END = 65535;

static uint8_t mymac[6] = {0x00,0x11,0x22,0x33,0x44,0x55};

static const uint8_t bcmac[6] = {0xff,0xff,0xff,0xff,0xff,0xff};

static const uint16_t NOETHRX_TO = (60*60);

static const uint8_t* UNIQUE_DEVICE_ID_ADDR = (uint8_t*)(0x1ffff7e8);
static const uint8_t UNIQUE_DEVICE_ID_LEN = 12;

static const uint16_t LM_ERROR = 0x01;
static const uint16_t LM_ETH = 0x02;
static const uint16_t LM_IPV4 = 0x04;
static const uint16_t LM_UDP = 0x08;
static const uint16_t LM_TCP = 0x10;
static const uint16_t LM_ICMP = 0x20;
static const uint16_t LM_PHY = 0x40;
static const uint16_t LM_ARP = 0x80;

static const uint8_t ENOARP = 100;
static const uint8_t ENOGW = 101;

//-----------------------------------------------------------------------------
//  Typedefs
//-----------------------------------------------------------------------------

typedef struct __attribute__((__packed__)) {
	eth_header_t ethh;
	ipv4_header_t ipv4h;
	udp_header_t udph;
} full_udp_header_t;

void full_udp_header_size_check(void)
{
	switch(0) {case 0:case sizeof(full_udp_header_t) == sizeof(eth_header_t)+sizeof(ipv4_header_t)+sizeof(udp_header_t):;}
}

typedef struct __attribute__((__packed__)) {
	eth_header_t ethh;
	ipv4_header_t ipv4h;
	icmp_header_t icmph;
} full_icmp_header_t;

void full_icmp_header_size_check(void)
{
	switch(0) {case 0:case sizeof(full_icmp_header_t) == sizeof(eth_header_t)+sizeof(ipv4_header_t)+sizeof(icmp_header_t):;}
}

//-----------------------------------------------------------------------------
//  Global variables
//-----------------------------------------------------------------------------

static volatile uint32_t uptime;
static volatile uint32_t lastethrx;
static uint16_t logmask;

static uint8_t uart1rxbuf[32];
static uint8_t uart1txbuf[64];

volatile uint32_t tmr_cnt[TMR_ID_SIZE];
static uint32_t tmr_top[TMR_ID_SIZE];

static uint8_t myip[4] = {192,168,1,5};
static uint8_t mynm[4] = {255,255,255,0};
static uint8_t gwip[4] = {192,168,1,1};

static uint32_t arpreqbuf[8];
static volatile struct fifo_t arpreqfifo;

//-----------------------------------------------------------------------------
//  newlib required functions
//-----------------------------------------------------------------------------

void _exit(int status)
{
	while( 1 );
}

//int __errno; // required by math

//-----------------------------------------------------------------------------
//  Timers
//-----------------------------------------------------------------------------

void tmr_set(uint8_t tid, uint32_t cnt)
{
	tmr_top[tid] = cnt;
	tmr_cnt[tid] = cnt;
}

void tmr_reset(uint8_t tid)
{
	tmr_cnt[tid] = tmr_top[tid];
}

uint8_t tmr_elapsed(uint8_t tid)
{
	return (tmr_cnt[tid] == 0);
}

void tmr_tick(void)
{
	uint8_t i;
	for( i = 0; i < TMR_ID_SIZE; ++i ) {
		if( tmr_cnt[i] > 0 )
			--(tmr_cnt[i]);
	}
}

//-----------------------------------------------------------------------------
//  SysTick handler
//-----------------------------------------------------------------------------

void SysTick_Handler(void)
{
	tmr_tick();

	static uint16_t ms = 0;
	if( ++ms == 1000 ) {
		ms = 0;
		++uptime;
	}
}

//-----------------------------------------------------------------------------
//  delay functions
//-----------------------------------------------------------------------------

void _delay_ms (uint32_t ms)
{
	tmr_set(TMR_ID_DELAY, ms);
	while( !tmr_elapsed(TMR_ID_DELAY));
}

void _delay_us(uint32_t us)
{
	us *= 8;

	asm volatile(
		"mov r0, %[us]             \n\t"
		"1: subs r0, #1            \n\t"
		"nop                       \n\t"
		"nop                       \n\t"
		"bhi 1b                    \n\t"
		:
		: [us] "r" (us)
		: "r0"
	);
}

//-----------------------------------------------------------------------------
//  print functions
//-----------------------------------------------------------------------------

void hprintbuf(const uint8_t* buf, uint16_t len) // hex print buf
{
	while( len ) {
		ser_puti_lc(AT_CMD_UART, *buf, 16, 2, '0');
		++buf;
		--len;
	}
	ser_puts(AT_CMD_UART, "\r\n");
}

char* iptoa(uint8_t* a)
{
	static char s[20];
	char* p = s;
	uint8_t i;
	for( i = 0; i < 4; ++i ) {
		itoa(a[i], p, 10);
		while( *p ) ++p;
		*p = '.';
		++p;
	}
	--p;
	*p = 0;
	return s;
}

void printippkt(ipv4_header_t* p)
{
	ser_printf("ver %d\r\n", p->version_ihl >> 4);
	ser_printf("ihl %d\r\n", p->version_ihl & 0xf);
	ser_printf("dscp %d\r\n", p->dscp_ecn);
	ser_printf("len %d\r\n", __builtin_bswap16(p->len));

	ser_printf("iden %x\r\n", __builtin_bswap16(p->iden));
	ser_printf("frag ofs %x\r\n", __builtin_bswap16(p->frag_ofs));

	ser_printf("ttl %d\r\n", p->ttl);
	ser_printf("proto %d\r\n", p->proto);
	ser_printf("chksum %x\r\n", __builtin_bswap16(p->chksum));

	ser_printf("src %s\r\n", iptoa(p->srcip));
	ser_printf("dst %s\r\n", iptoa(p->dstip));
}

//-----------------------------------------------------------------------------
//  utility functions
//-----------------------------------------------------------------------------

uint32_t udtoi(const char* s) // unsigned decimal string to u32
{
	uint32_t x = 0;

	while( isdigit((int)*s) ) {
		x *= 10;
		x += *s - '0';
		++s;
	}

	return x;
}

uint32_t uhtoi(const char* s, uint8_t n) // unsigned hex string to u32
{
	uint32_t x = 0;

	while( n-- ) {
		char c = *s;
		if( isdigit((int)c) ) {
			c -= '0';
		} else
		if( (c >= 'A') && (c <= 'F') ) {
			c -= 'A' - 10;
		} else
		if( (c >= 'a') && (c <= 'f') ) {
			c -= 'a' - 10;
		} else {
			break;
		}
		x *= 16;
		x += c;
		++s;
	}

	return x;
}

uint8_t parse_ip(const char* s, uint8_t* ip)
{
	uint8_t i;
	uint32_t a;

	for( i = 0; i < 4; ++i ) {
		a = udtoi(s);
		if( a > 255 ) return 1;
		ip[i] = a;
		if( 3 == i ) break;
		s = strchr(s, '.');
		if( !s ) return 1;
		++s;
	}

	return 0;
}

uint8_t parse_port(const char* s, uint16_t* a)
{
	uint32_t port = udtoi(s);
	if( (port < 10) || (port > 0xffff) ) return 1;
	*a = port;
	return 0;
}

uint16_t eph_port(void)
{
	static uint16_t ephport;

	if( ++ephport >= EPH_PORT_END ) ephport = EPH_PORT_BEGIN;

	return ephport;
}

uint8_t valid_ip(uint8_t* ip)
{
	if( *ip == 0 ) return 0;
	return 1;
}

uint8_t same_subnet(uint8_t* ip1, uint8_t* ip2, uint8_t* nm)
{
	return ( *(uint32_t*)ip1 & *(uint32_t*)nm ) == ( *(uint32_t*)ip2 & *(uint32_t*)nm );
}

uint8_t arp_request(uint8_t* dstip)
{
	return fifo_put(&arpreqfifo, dstip);
}

uint8_t arp_request_send(uint8_t* dstip)
{
	eth_header_t hdr;
	// construct eth header
	memcpy(hdr.dmac, bcmac, 6);
	memcpy(hdr.smac, mymac, 6);
	hdr.etype = __builtin_bswap16(ARP_ETYPE);
	// construct arp packet
	arp_packet_t arp;
	arp_make_request(&arp, myip, mymac, dstip);
	// send packet
	return eth_txbuf((uint8_t*)&hdr, sizeof(hdr), (uint8_t*)&arp, sizeof(arp));
}

uint8_t udp_send(uint8_t* dstip, uint16_t sport, uint16_t dport, void* data, uint16_t len)
{
	uint8_t dstmac[6];

	uint8_t ssn = same_subnet(myip, dstip, mynm);

	if( !ssn && !valid_ip(gwip) )
		return ENOGW;

	if( !arp_find_entry(ssn ? dstip : gwip, dstmac) ) {
		arp_request(ssn ? dstip : gwip);
		return ENOARP;
	}

	if( sport == 0 ) sport = eph_port();

	full_udp_header_t hdr;
	// construct eth header
	memcpy(hdr.ethh.dmac, dstmac, 6);
	memcpy(hdr.ethh.smac, mymac, 6);
	hdr.ethh.etype = __builtin_bswap16(IPV4_ETYPE);
	// construct udp header
	udp_make_hdr(&hdr.udph, sport, dport, len);
	// construct ipv4 header
	ipv4_make_hdr(&hdr.ipv4h, udp_pkt_len(&hdr.udph), UDP_IPTYPE, myip, dstip);

	// send packet
	return eth_txbuf((uint8_t*)&hdr, sizeof(hdr), data, len);
}

uint8_t icmp_send(uint8_t* dstip, uint8_t icmp_type, uint8_t icmp_code, uint32_t icmp_data, uint8_t* data, uint16_t len)
{
	uint8_t dstmac[6];

	uint8_t ssn = same_subnet(myip, dstip, mynm);

	if( !ssn && !valid_ip(gwip) )
		return ENOGW;

	if( !arp_find_entry(ssn ? dstip : gwip, dstmac) ) {
		arp_request(ssn ? dstip : gwip);
		return ENOARP;
	}

	full_icmp_header_t hdr;
	// construct eth header
	memcpy(hdr.ethh.dmac, dstmac, 6);
	memcpy(hdr.ethh.smac, mymac, 6);
	hdr.ethh.etype = __builtin_bswap16(IPV4_ETYPE);
	// construct icmp header
	hdr.icmph.type = icmp_type;
	hdr.icmph.code = icmp_code;
	hdr.icmph.chksum = 0; // checksum will be calculated by MAC
	hdr.icmph.data = icmp_data;
	// construct ip header
	ipv4_make_hdr(&hdr.ipv4h, sizeof(icmp_header_t)+len, ICMP_IPTYPE, myip, dstip);

	// send packet
	return eth_txbuf((uint8_t*)&hdr, sizeof(hdr), data, len);
}

//-----------------------------------------------------------------------------
//  AT command processing
//-----------------------------------------------------------------------------

uint8_t proc_at_cmd(const char* s)
{
	if( s[0] == 0 ) {
		return 1;
	}

	if( 0 == strcmp(s, "AT") ) {
		return 0;
	}

	if( 0 == strcmp(s, "ATI") ) {
		ser_printf("eth test\r\n");
		return 0;
	}

	char atclks[] = "AT+CLKS";

	if( 0 == strcmp(s, atclks) ) {
		RCC_ClocksTypeDef c;
		RCC_GetClocksFreq(&c);

		ser_printf("HCLK %d\r\n", c.HCLK_Frequency);
		ser_printf("PCLK1 %d\r\n", c.PCLK1_Frequency);
		ser_printf("PCLK2 %d\r\n", c.PCLK2_Frequency);
		ser_printf("SYSCLK %d\r\n", c.SYSCLK_Frequency);

		return 0;
	}

	char atphyregs[] = "AT+PHYREGS";

	if( 0 == strcmp(s, atphyregs) ) {
		uint8_t i;
		for( i = 0; i < 32; ++i ) {
			if( (i>6)&&(i<17) ) continue;
			if( (i>18)&&(i<26) ) continue;
			if( i == 28 ) continue;
			uint16_t r = eth_phyrreg(i);
			ser_printf("%2d %4x\r\n", i, r);
		}

		return 0;
	}

	char atstat[] = "AT+STAT";

	if( 0 == strcmp(s, atstat) ) {
		ser_printf("uptime %d\r\n", uptime);
		ser_printf("lastethrx %d\r\n", lastethrx);
		ser_printf("uid ");
		hprintbuf(UNIQUE_DEVICE_ID_ADDR, UNIQUE_DEVICE_ID_LEN);
		ser_printf("---\r\n");

		ser_printf("ip %s\r\n", iptoa(myip));
		ser_printf("nm %s\r\n", iptoa(mynm));
		ser_printf("gw %s\r\n", iptoa(gwip));
		ser_printf("mac ");
		hprintbuf(mymac, 6);
		ser_printf("link ");
		if( eth_linkup() ) {
			uint16_t r = eth_phyrreg(31);
			if( r & 0x04 ) ser_printf("10M ");
			if( r & 0x08 ) ser_printf("100M ");
			if( r & 0x10 ) ser_printf("FD\r\n"); else ser_printf("HD\r\n");
		} else {
			ser_printf("DOWN\r\n");
		}

		return 0;
	}

	char atping[] = "AT+PING=";

	if( 0 == strncmp(s, atping, strlen(atping)) ) {
		s += strlen(atping);
		uint8_t dstip[4];
		if( parse_ip(s, dstip) ) return 1;

		uint8_t pingdata[] = {1,2,3,4,5,6,7,8};
		static uint16_t pingseq = 0;

		uint8_t r = icmp_send(dstip, ICMP_TYPE_ECHO_REQUEST, 0, __builtin_bswap32(++pingseq), pingdata, sizeof(pingdata));
		ser_printf("seq %d\r\n", pingseq);
		if( r ) {
			ser_printf("error %d\r\n", r);
		}

		return 0;
	}

	char atudp[] = "AT+UDP=";

	if( 0 == strncmp(s, atudp, strlen(atudp)) ) {
		s += strlen(atudp);
		uint8_t dstip[4];
		if( parse_ip(s, dstip) ) return 1;

		uint16_t dstport;
		s = strchr(s, ',');
		if( !s ) return 1;
		++s;
		if( parse_port(s, &dstport) ) return 1;

		s = strchr(s, ',');
		if( !s ) return 1;
		++s;

		uint8_t r = udp_send(dstip, 0, dstport, (uint8_t*)s, strlen(s));
		if( r ) {
			ser_printf("error %d\r\n", r);
		}

		return 0;
	}

	char atarp[] = "AT+ARP=";

	if( 0 == strncmp(s, atarp, strlen(atarp)) ) {
		s += strlen(atudp);
		uint8_t dstip[4];
		if( parse_ip(s, dstip) ) return 1;

		arp_request(dstip);

		return 0;
	}

	char atarptable[] = "AT+ARPTABLE";

	if( 0 == strcmp(s, atarptable) ) {
		uint8_t i = 0;
		arp_entry_t a;
		while( arp_get_entry(i, &a) ) {
			ser_printf("%d %s ", i, iptoa(a.ip));
			hprintbuf(a.mac, 6);

			++i;
		}
		return 0;
	}

	char atlogmask[] = "AT+LOGMASK=";

	if( 0 == strncmp(s, atlogmask, strlen(atlogmask)) ) {
		s += strlen(atlogmask);

		if( strlen(s) < 1 )
			return 1;

		logmask = uhtoi(s, sizeof(logmask)*2);

		return 0;
	}

	// help

	if( 0 == strcmp(s, "AT?") ) {
		char* atall[] = {
			atclks,
			atphyregs,
			atstat,
			atping,
			atudp,
			atarp,
			atarptable,
			atlogmask
		};
		uint8_t i;
		for( i = 0; i < sizeof(atall)/sizeof(char*); ++i ) {
			ser_printf("%s\r\n", atall[i]);
		}
		return 0;
	}

	return 1;
}

//-----------------------------------------------------------------------------
//  MAIN function
//-----------------------------------------------------------------------------

int main(void)
{
	if( SysTick_Config(SystemCoreClock / 1000) ) { // setup SysTick Timer for 1 msec interrupts
		while( 1 );                                  // capture error
	}

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); // disable preemption

	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	IWDG_SetPrescaler(IWDG_Prescaler_32); // approx 3s
	IWDG_SetReload(0xfff);
	IWDG_ReloadCounter();
	IWDG_Enable();

	logmask = 0xffff;

	ser_init(AT_CMD_UART, AT_CMD_BAUD, uart1txbuf, sizeof(uart1txbuf), uart1rxbuf, sizeof(uart1rxbuf));
	ser_printf_devnum = AT_CMD_UART;
	ser_printf("reset\r\n");

	misc_gpio_config(LED_PORT, LED_PIN, GPIO_Mode_Out_PP);
	misc_gpio_config(PHY_INT_PORT, PHY_INT_PIN, GPIO_Mode_IPU);

	memcpy(mymac, UNIQUE_DEVICE_ID_ADDR, sizeof(mymac));
	mymac[0] = 0x02; // locally administered MAC

	if( eth_init((uint8_t*)mymac) ) {
		ser_printf("eth_init() error\r\n");
	}

	//misc_exti_setup(PHY_INT_PORT, PHY_INT_PIN, EXTI_Trigger_Falling);

	fifo_clear(&arpreqfifo, arpreqbuf, sizeof(arpreqbuf), sizeof(arpreqbuf[0]));

	tmr_set(TMR_ID_LED, 0);
	tmr_set(TMR_ID_ARP_REQ, 50);

	while( 1 ) {
		// AT command processing
		{
			uint8_t d;
			if( ser_getc(AT_CMD_UART, &d) ) {
				static char atbuf[64];
				static uint8_t atbuflen = 0;

				// buffer overflow guard
				if( atbuflen >= sizeof(atbuf) ) { atbuflen = 0; }

				// execute on enter
				if( (d == '\r') || (d == '\n') ) {
					if( atbuflen ) {
						atbuf[atbuflen] = 0;
						atbuflen = 0;
						uint8_t r = proc_at_cmd(atbuf);
						if( r == 0 ) ser_printf("OK\r\n");
						if( r == 1 ) ser_printf("ERR\r\n");
					}
				} else
				if( d == 0x7f ) {	// backspace
					if( atbuflen ) { --atbuflen; }
				} else {			// store character
					atbuf[atbuflen++] = toupper(d);
				}
			}
		}
		// ETH PHY INT
		{
			if( GPIO_ReadInputDataBit(PHY_INT_PORT, PHY_INT_PIN) == Bit_RESET ) {
				uint16_t ei = eth_phy_int();
				if( logmask & LM_PHY ) {
					ser_printf("PHY int %x\r\n", ei);
				}
			}
		}
		// ETH RX
		{
			uint8_t* p;
			uint16_t plen;
			if( eth_rx(&p, &plen) == 0 ) {
				lastethrx = uptime;
				eth_header_t* hdr = (eth_header_t*)p;
				p += sizeof(eth_header_t);

				if( hdr->etype == __builtin_bswap16(ARP_ETYPE) ) {
					// received frame contains an ARP packet
					arp_packet_t* arp = (arp_packet_t*)p;

					if( arp_process_reply(arp, myip) ) {
						if( logmask & LM_ARP ) {
							ser_printf("ARP got reply from %s\r\n", iptoa(arp->sender_padr));
						}
					} else
					if( arp_process_request(arp, myip, mymac) ) {
						memcpy(hdr->dmac, arp->target_hadr, 6);
						memcpy(hdr->smac, arp->sender_hadr, 6);
						eth_txbuf((uint8_t*)hdr, sizeof(eth_header_t), (uint8_t*)arp, sizeof(arp_packet_t));
						if( logmask & LM_ARP ) {
							ser_printf("ARP replied to %s\r\n", iptoa(arp->target_padr));
						}
					}
				} else
				if( hdr->etype == __builtin_bswap16(IPV4_ETYPE) ) {
					// received frame contains an ipv4 packet
					ipv4_header_t* iph = (ipv4_header_t*)p;
					p += ipv4_hdr_len(iph);
					uint8_t unicast = (memcmp(iph->dstip, myip, 4) == 0);

					if( iph->proto == ICMP_IPTYPE ) {
						icmp_header_t* icmph = (icmp_header_t*)p;
						p += sizeof(icmp_header_t);

						if( icmph->type == ICMP_TYPE_ECHO_REQUEST ) {
							uint16_t erdlen = ipv4_payload_len(iph)-sizeof(icmp_header_t);
							icmp_send(iph->srcip, ICMP_TYPE_ECHO_REPLY, 0, icmph->data, p, erdlen);
						} else
						if( icmph->type == ICMP_TYPE_DEST_UNREACH ) {
							if( logmask & LM_ICMP ) {
								ser_printf("ICMP dst unreach, code %d\r\n", icmph->code);
							}
						} else
						if( icmph->type == ICMP_TYPE_ECHO_REPLY ) {
							if( logmask & LM_ICMP ) {
								ser_printf("ICMP echo reply, seq %d\r\n", __builtin_bswap32(icmph->data));
							}
						} else {
							if( logmask & LM_ICMP ) {
								ser_printf("ICMP type %d, code %d\r\n", icmph->type, icmph->code);
							}
						}
					} else
					if( iph->proto == UDP_IPTYPE ) {
						udp_header_t* udph = (udp_header_t*)p;
						p += sizeof(udp_header_t);

						if( logmask & LM_UDP ) {
							ser_printf("UDP %s:%d", iptoa(iph->srcip), __builtin_bswap16(udph->srcport));
							ser_printf(" => %s:%d", iptoa(iph->dstip), __builtin_bswap16(udph->dstport));
							ser_printf(", len %d\r\n", udp_payload_len(udph));
						}

						if( unicast ) {
							/*if( __builtin_bswap16(udph->dstport) == your_app_port ) {
								// process UDP packet
							} else */
							{
								// return ICMP port unreachable
								icmp_send(iph->srcip, ICMP_TYPE_DEST_UNREACH, ICMP_CODE_DEST_PORT_UNREACH, 0, (uint8_t*)iph, ipv4_hdr_len(iph)+sizeof(udp_header_t));
							}
						}
					} else
					if( iph->proto == TCP_IPTYPE ) {
						tcp_header_t* tcph = (tcp_header_t*)p;
						p += tcp_hdr_len(tcph);

						if( logmask & LM_TCP ) {
							ser_printf("TCP %s:%d", iptoa(iph->srcip), __builtin_bswap16(tcph->srcport));
							ser_printf(" => %s:%d", iptoa(iph->dstip), __builtin_bswap16(tcph->dstport));
							ser_printf(", len %d\r\n", ipv4_payload_len(iph)-tcp_hdr_len(tcph));
						}

						// return ICMP protocol unreachable
						if( unicast ) {
							icmp_send(iph->srcip, ICMP_TYPE_DEST_UNREACH, ICMP_CODE_DEST_PROTO_UNREACH, 0, (uint8_t*)iph, ipv4_hdr_len(iph)+tcp_hdr_len(tcph));
						}
					} else {
						if( logmask & LM_IPV4 ) {
							ser_printf("unk IPv4\r\n");
							ser_printf("src %s\r\n", iptoa(iph->srcip));
							ser_printf("dst %s\r\n", iptoa(iph->dstip));
							ser_printf("proto %d\r\n, iph->proto");
							ser_printf("---\r\n");
						}
					}
				} else {
					if( logmask & LM_ETH ) {
						ser_printf("unk ETH\r\n");
						hprintbuf(hdr->dmac, 6);
						hprintbuf(hdr->smac, 6);
						ser_printf("%x\r\n", __builtin_bswap16(hdr->etype));
						//hprintbuf(p, plen-sizeof(eth_header_t));
						ser_printf("---\r\n");
					}
				}
				eth_rxrelease();
			}
		}
		// ARP requests
		if( tmr_elapsed(TMR_ID_ARP_REQ) )	{
			uint8_t ip[4];
			if( fifo_get(&arpreqfifo, &ip) ) {
				if( !arp_find_entry(ip, 0) ) {
					arp_request_send(ip);
					tmr_reset(TMR_ID_ARP_REQ);
					if( logmask & LM_ARP ) {
						ser_printf("ARP sent req for %s\r\n", iptoa(ip));
					}
				}
			}
		}
		// LED
		if( tmr_elapsed(TMR_ID_LED) ) {
			if( GPIO_ReadOutputDataBit(LED_PORT, LED_PIN) == Bit_SET ) {
				GPIO_ResetBits(LED_PORT, LED_PIN);
				tmr_set(TMR_ID_LED, 2000);
			} else {
				GPIO_SetBits(LED_PORT, LED_PIN);
				tmr_set(TMR_ID_LED, 50);
			}
		}
		// reset if no ETH frames received
		if( uptime - lastethrx > NOETHRX_TO ) {
			ser_printf("noethrx timeout\r\n");
			_delay_ms(100);
			NVIC_SystemReset();
		}
		// watchdog
		IWDG_ReloadCounter();
	}
}

//-----------------------------------------------------------------------------
//  INTERRUPTS
//-----------------------------------------------------------------------------

void EXTI15_10_IRQHandler(void)
{
	if( EXTI_GetITStatus(PHY_INT_PIN) ) {
		EXTI_ClearITPendingBit(PHY_INT_PIN);
		ser_printf("PHY int\r\n");
	}
}
