/**

RMII interface is implemented between MAC and PHY.

Autonegotiation is used to determine link parameters.

The PHY_SCSR, PHY_SCSR_100M and PHY_SCSR_FD are PHY specific and need to be defined
for your particular PHY. The default is for LAN8720.

Receiving a packet returns a pointer to an internal buffer which is then owned
by the application (unavailable for reception) until eth_rxrelease() is called.
The number and size of rx buffers is configurable with ETH_NUMRXDESC and
ETH_RXBUFSIZE defines.

The transmit function accepts two buffers. This is for convenience, so the
application can use separate buffers for the headers and the payload. Both
buffers are copied to an internal buffer and queued for transmission. If not
required, either buffer can be empty. The number and size of tx buffers is
configurable with ETH_NUMTXDESC and ETH_TXBUFSIZE defines.

@file		eth.c
@brief		ETH MAC routines
@author		Matej Kogovsek (matej@hamradio.si)
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
*/

#include "stm32f10x.h"
#include <string.h>

#include "eth_defs.h"
#include "eth.h"

#ifndef ETH_NUMTXDESC
/** Number of TX descriptors to allocate */
#define ETH_NUMTXDESC 8
#endif

#ifndef ETH_TXBUFSIZE
/** Buffer size allocated to each TX descriptor */
#define ETH_TXBUFSIZE ((uint32_t)520) // must be a multiple of 4
#endif

#ifndef ETH_NUMRXDESC
/** Number of RX descriptors to allocate */
#define ETH_NUMRXDESC 8
#endif

#ifndef ETH_RXBUFSIZE
/** Buffer size allocated to each RX descriptor */
#define ETH_RXBUFSIZE ((uint32_t)520) // must be a multiple of 4
#endif

#ifndef PHY_SCSR
/** PHY status register number (not standardized, PHY specific) */
#define PHY_SCSR 31
#define PHY_SCSR_10M ((uint16_t)0x0004)
#define PHY_SCSR_100M ((uint16_t)0x0008)
#define PHY_SCSR_FD ((uint16_t)0x0010)
#endif

#ifndef PHY_ISR
#define PHY_ISR 29
#define PHY_ISR_ENERGYON ((uint16_t)0x0080)
#define PHY_ISR_AUTONEGO_COMPLETE ((uint16_t)0x0040)
#define PHY_ISR_LINKDOWN ((uint16_t)0x0010)
#endif

#ifndef PHY_IMR
#define PHY_IMR 30
#endif

#ifndef PHY_ADDR
#define PHY_ADDR 0
#endif

extern void _delay_ms (uint32_t);

void eth_header_size_check(void)
{
	switch(0) {case 0:case sizeof(eth_header_t) == 14:;}
}

/** @privatesection */

typedef struct {
	uint32_t   stat;
	uint32_t   cnt;
	uint8_t*   buf1;
	uint8_t*   buf2;
} eth_dmadesc_t;

void eth_dmadesc_size_check(void)
{
	switch(0) {case 0: case sizeof(eth_dmadesc_t) == 16:;}
}

static eth_dmadesc_t eth_txdesc[ETH_NUMTXDESC] __attribute__ ((aligned(4)));
static uint8_t eth_txbufs[ETH_NUMTXDESC*ETH_TXBUFSIZE] __attribute__ ((aligned(4)));
static uint16_t eth_curtxdesc;

static eth_dmadesc_t eth_rxdesc[ETH_NUMRXDESC] __attribute__ ((aligned(4)));
static uint8_t eth_rxbufs[ETH_NUMRXDESC*ETH_RXBUFSIZE] __attribute__ ((aligned(4)));
static uint16_t eth_currxdesc;

void eth_phywreg(uint16_t a, uint16_t d)
{
	ETH->MACMIIDR = d;
	ETH->MACMIIAR = (PHY_ADDR << 11) | (a << 6) | ETH_MACMIIAR_MW | ETH_MACMIIAR_MB;
	while( ETH->MACMIIAR & ETH_MACMIIAR_MB );
}

uint16_t eth_phyrreg(uint16_t a)
{
	ETH->MACMIIAR = (PHY_ADDR << 11) | (a << 6) | ETH_MACMIIAR_MB;
	while( ETH->MACMIIAR & ETH_MACMIIAR_MB );
	return ETH->MACMIIDR;
}

/** @publicsection */

/**
@brief	Init ETH.
@param[in]	macaddr	Pointer to 6 bytes containing MAC address
@return	0 on success, error code otherwise
*/
uint8_t eth_init(uint8_t* macaddr)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);

	RCC_AHBPeriphResetCmd(RCC_AHBPeriph_ETH_MAC, ENABLE);
	GPIO_ETH_MediaInterfaceConfig(GPIO_ETH_MediaInterface_RMII);
	RCC_AHBPeriphResetCmd(RCC_AHBPeriph_ETH_MAC, DISABLE);

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_ETH_MAC | RCC_AHBPeriph_ETH_MAC_Tx | RCC_AHBPeriph_ETH_MAC_Rx, ENABLE);

	// use PLL3 to generate the 50MHz clk needed for RMII
	RCC_PLL3Cmd(DISABLE);
	RCC_PREDIV2Config(RCC_PREDIV2_Div5); // 25MHz / 5 = 5MHz
	RCC_PLL3Config(RCC_PLL3Mul_10); // 5MHz * 10 = 50MHz
	RCC_PLL3Cmd(ENABLE);
	while( RCC_GetFlagStatus(RCC_FLAG_PLL3RDY) == RESET );
	RCC_MCOConfig(RCC_MCO_PLL3CLK); // output PLL3 clock on MCO (PA8)

	// configure RMII pins
	GPIO_TypeDef* ports[] =	{
		PHY_MDC_PORT,	PHY_REF_CLK_PORT,	PHY_MDIO_PORT,
		PHY_CRS_DV_PORT, PHY_RXD0_PORT, PHY_RXD1_PORT,
		PHY_TX_EN_PORT, PHY_TXD0_PORT, PHY_TXD1_PORT
	};
	uint16_t pins[] =	{
		PHY_MDC_PIN, PHY_REF_CLK_PIN, PHY_MDIO_PIN,
		PHY_CRS_DV_PIN, PHY_RXD0_PIN, PHY_RXD1_PIN,
		PHY_TX_EN_PIN, PHY_TXD0_PIN, PHY_TXD1_PIN
	};
	GPIOMode_TypeDef modes[] = {
		GPIO_Mode_AF_PP, GPIO_Mode_AF_PP, GPIO_Mode_AF_PP,
		GPIO_Mode_IN_FLOATING, GPIO_Mode_IN_FLOATING, GPIO_Mode_IN_FLOATING,
		GPIO_Mode_AF_PP, GPIO_Mode_AF_PP, GPIO_Mode_AF_PP
	};

	uint8_t i;
	for( i = 0; i < 9; ++i ) {
		GPIO_InitTypeDef gitd;
		gitd.GPIO_Pin = pins[i];
		gitd.GPIO_Speed = GPIO_Speed_50MHz;
		gitd.GPIO_Mode = modes[i];
		GPIO_Init(ports[i], &gitd);
	}

	eth_phywreg(PHY_BCR, PHY_BCR_RESET); // sw reset PHY
	_delay_ms(600); // 0.5s for sw reset

	// config MAC to PHY speed, hardware IPV4 CRC checking, auto padding/CRC stripping
	uint32_t rv = ETH->MACCR & 0xff308103; // read register and keep reserved bits
	ETH->MACCR = rv | ETH_MACCR_IPCO | ETH_MACCR_APCS;

	//rv = ETH->MACFFR & 0x7ffff800;
	//ETH->MACFFR = rv | ;

	// set MAC address
	ETH->MACA0HR = *(uint32_t*)(macaddr+4) & 0xffff;
	ETH->MACA0LR = *(uint32_t*)macaddr;

	// config DMA to rx tx store and forward
	rv = ETH->DMAOMR & 0xf8ce1f21;
	ETH->DMAOMR = rv | ETH_DMAOMR_RSF | ETH_DMAOMR_TSF;

	// config DMA transfers
	rv = ETH->DMABMR & 0xfc000080;
	ETH->DMABMR = rv | ETH_DMABMR_AAB | ETH_DMABMR_PBL_16Beat;

	// prepare tx descriptors
	for( i = 0; i < ETH_NUMTXDESC; ++i ) {
		eth_txdesc[i].stat = 0;
		eth_txdesc[i].buf2 = (uint8_t*)&eth_txbufs[i*ETH_TXBUFSIZE];
		eth_txdesc[i].cnt  = ETH_TXBUFSIZE << 16;
	}
	eth_curtxdesc = 0;
	// set DMA tx descriptor list address
	ETH->DMATDLAR = (uint32_t)eth_txdesc;

	// prepare rx descriptors
	for( i = 0; i < ETH_NUMRXDESC; ++i ) {
		eth_rxdesc[i].stat = ETH_DMARxDesc_OWN;
		eth_rxdesc[i].buf2 = (uint8_t*)&eth_rxbufs[i*ETH_RXBUFSIZE];
		eth_rxdesc[i].cnt  = ETH_RXBUFSIZE << 16;
	}
	eth_rxdesc[ETH_NUMRXDESC-1].cnt |= ETH_DMARxDesc_RER;
	eth_currxdesc = 0;
	// set DMA rx descriptor list address
	ETH->DMARDLAR = (uint32_t)eth_rxdesc;

	// enable DMA tx and rx processes
	ETH->DMAOMR |= (ETH_DMAOMR_ST | ETH_DMAOMR_SR);

	// enable autonego complete PHY interrupts
	eth_phywreg(PHY_IMR, PHY_ISR_AUTONEGO_COMPLETE);

	// deassert PHY interrupt
	eth_phyrreg(PHY_ISR);

	// enable autonego
	eth_phywreg(PHY_BCR, PHY_BCR_AUTONEGO);
/*
	while( !(eth_phyrreg(PHY_BSR) & PHY_BSR_AUTONEGO_COMPLETE) );

	if( eth_phyrreg(PHY_SCSR) & PHY_SCSR_100M ) ETH->MACCR |= ETH_MACCR_FES;
	if( eth_phyrreg(PHY_SCSR) & PHY_SCSR_FD ) ETH->MACCR |= ETH_MACCR_DM;

	// enable MAC tx and rx
	ETH->MACCR |= (ETH_MACCR_TE | ETH_MACCR_RE);
*/
	return 0;
}

uint16_t eth_phy_int(void)
{
	// deassert INT by reading ISR
	uint16_t isr = eth_phyrreg(PHY_ISR);

	eth_phyrreg(PHY_BSR); // read twice for some reason?
	uint16_t bsr = eth_phyrreg(PHY_BSR);
	if( bsr & PHY_BSR_LINK_UP ) {
		uint16_t scsr = eth_phyrreg(PHY_SCSR);
		// set MAC speed according to autonego
		if( scsr & PHY_SCSR_100M ) {
			ETH->MACCR |= ETH_MACCR_FES;
		} else {
			ETH->MACCR &= ~ETH_MACCR_FES;
		}
		if( scsr & PHY_SCSR_FD ) {
			ETH->MACCR |= ETH_MACCR_DM;
		} else {
			ETH->MACCR &= ~ETH_MACCR_DM;
		}

		// enable MAC tx/rx
		ETH->MACCR |= (ETH_MACCR_TE | ETH_MACCR_RE);
	}

	return isr;
}

/**
@brief	Ask PHY whether link is up
@return	0 on link down, nonzero on link up
*/
uint8_t eth_linkup(void)
{
	return (eth_phyrreg(PHY_BSR) & PHY_BSR_LINK_UP);
}

/**
@brief	Transmit buffer (split into two buffers for convenience).
@param[in]	header	Pointer to first buffer (can be 0)
@param[in]	headerlen	Length of first buffer (can be 0)
@param[in]	payload Pointer to second buffer
@param[in]	payloadlen	Length of second buffer
@return	0 on success, error code otherwise
*/
uint8_t eth_txbuf(uint8_t* header, uint32_t headerlen, uint8_t* payload, uint32_t payloadlen)
{
	if( eth_txdesc[eth_curtxdesc].stat & ETH_DMATxDesc_OWN ) {
		return 1; // out of descriptors
	}

	uint8_t nexttxdesc = (eth_curtxdesc + 1) % ETH_NUMTXDESC;
	uint32_t end_of_ring = nexttxdesc ? 0 : ETH_DMATxDesc_TER;

	if( headerlen + payloadlen > ETH_TXBUFSIZE ) {
		return 2; // packet too large
	}

	memcpy(eth_txdesc[eth_curtxdesc].buf2, header, headerlen);
	memcpy(eth_txdesc[eth_curtxdesc].buf2+headerlen, payload, payloadlen);
	eth_txdesc[eth_curtxdesc].cnt  = (payloadlen + headerlen) << 16;
	eth_txdesc[eth_curtxdesc].stat = ETH_DMATxDesc_OWN | ETH_DMATxDesc_LS | ETH_DMATxDesc_FS | ETH_DMATxDesc_CIC_TCPUDPICMP_Full | end_of_ring;

	eth_curtxdesc = nexttxdesc;

	ETH->DMATPDR = 0; // resume TX

	return 0;
}

/**
@brief	Return true if nothing more to be transmitted
*/
uint8_t eth_txdone(void)
{
	if( ETH->DMASR & ETH_DMASR_TBUS ) {
		return 1;
	}
	return 0;
}

/**
@brief	Release the frame buffer returned by eth_rx().
*/
void eth_rxrelease(void)
{
	if( eth_rxdesc[eth_currxdesc].stat & ETH_DMARxDesc_OWN ) {
		return; // DMA already owns the descriptor
	}

	eth_rxdesc[eth_currxdesc].stat = ETH_DMARxDesc_OWN; // return descriptor to DMA

	++eth_currxdesc;
	eth_currxdesc %= ETH_NUMRXDESC;

	ETH->DMARPDR = 0; // resume RX
}

/**
@brief	Receive a frame. A pointer to internal buffer is returned from this function. Call eth_rxrelease() after being done with the buffer.
@param[out]	Pointer to buffer pointer
@param[out]	Pointer to length
@return	0 on success, error code otherwise
*/
uint8_t eth_rx(uint8_t** buf, uint16_t* len)
{
	if( eth_rxdesc[eth_currxdesc].stat & ETH_DMARxDesc_OWN ) {
		return 1; // no frame received
	}
	if( (eth_rxdesc[eth_currxdesc].stat &
		(ETH_DMARxDesc_LS | ETH_DMARxDesc_FS | ETH_DMARxDesc_ES)) !=
		(ETH_DMARxDesc_LS | ETH_DMARxDesc_FS) ) {
		eth_rxrelease();
		return 2; // frame error, release rx desc and return error
	}
	*buf = eth_rxdesc[eth_currxdesc].buf2;
	*len = (eth_rxdesc[eth_currxdesc].stat & ETH_DMARxDesc_FL) >> 16;
	*len -= 4; // subtract CRC length
	return 0;
}
