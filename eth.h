// ------------------------------------------------------------------
// --- eth.c - basic routines for sending data over ETH MAC       ---
// ---                                                            ---
// ---            06.jun.2020, Matej Kogovsek (matej@hamradio.si) ---
// ------------------------------------------------------------------

#ifndef MAT_ETH_H
#define MAT_ETH_H

#include <inttypes.h>

typedef struct __attribute__((__packed__)) {
	uint8_t dmac[6];
	uint8_t smac[6];
	uint16_t etype;
} eth_header_t;

uint8_t eth_init(uint8_t* macaddr);
uint16_t eth_phy_int(void);
uint8_t eth_linkup(void);
uint8_t eth_txbuf(uint8_t* header, uint32_t headerlen, uint8_t* payload, uint32_t payloadlen);
uint8_t eth_txdone(void);
uint8_t eth_rx(uint8_t** buf, uint16_t* len);
void eth_rxrelease(void);

void eth_setmacaddr(uint8_t* a);
void eth_phywreg(uint16_t a, uint16_t d);
uint16_t eth_phyrreg(uint16_t a);

#endif
