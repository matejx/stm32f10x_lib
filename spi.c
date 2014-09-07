/**
@file		spi.c
@brief		SPI routines for STM32 F1
@author		Matej Kogovsek (matej@hamradio.si)
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
*/

#include <inttypes.h>
#include "stm32f10x.h"

/** @privatesection */

struct SPI_DevDef
{
	uint8_t spi_apb;
	SPI_TypeDef* spi;
	uint32_t spi_clk;
	GPIO_TypeDef* gpio;
	uint32_t gpio_clk;
	uint16_t pin_nss;
	uint16_t pin_sck;
	uint16_t pin_miso;
	uint16_t pin_mosi;
};

/** Register and pin defs for SPI1 */
struct SPI_DevDef SPI1_PinDef = {2, SPI1, RCC_APB2Periph_SPI1, GPIOA, RCC_APB2Periph_GPIOA, GPIO_Pin_4, GPIO_Pin_5, GPIO_Pin_6, GPIO_Pin_7};
/** Register and pin defs for SPI2 */
struct SPI_DevDef SPI2_PinDef = {1, SPI2, RCC_APB1Periph_SPI2, GPIOB, RCC_APB2Periph_GPIOB, GPIO_Pin_12, GPIO_Pin_13, GPIO_Pin_14, GPIO_Pin_15};

struct SPI_DevDef* spi_get_pdef(uint8_t devnum)
{
	if( devnum == 1 ) return &SPI1_PinDef;
	if( devnum == 2 ) return &SPI2_PinDef;

	return 0;
}

/**
@brief SPI chip select control.
@param[in]	devnum		SPI peripheral number (1..2)
@param[in]	cs			bool, true = NSS high (deselect), false = NSS low (select)
*/
void spi_cs(uint8_t devnum, uint8_t nss)
{
	struct SPI_DevDef* pdef = spi_get_pdef(devnum);

	if( nss ) {
		GPIO_WriteBit(pdef->gpio, pdef->pin_nss, Bit_SET);
	} else {
		GPIO_WriteBit(pdef->gpio, pdef->pin_nss, Bit_RESET);
	}
}

/**
@brief Send and receive byte (NSS not controlled)

This is the only function which does not control NSS. All others do.
@param[in]	devnum		SPI peripheral number (1..2)
@param[in]	d			Byte to send
@return byte received
*/
uint8_t spi_rw(uint8_t devnum, const uint8_t d)
{
	struct SPI_DevDef* pdef = spi_get_pdef(devnum);

	while (SPI_I2S_GetFlagStatus(pdef->spi, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(pdef->spi, d);
	while (SPI_I2S_GetFlagStatus(pdef->spi, SPI_I2S_FLAG_RXNE) == RESET);
	return SPI_I2S_ReceiveData(pdef->spi);
}

/** @publicsection */

/**
@brief Initialize SPI interface.

Although SPI can have different clock phase and polarity, I have never ran across anything that uses other
than low polarity and 1st edge phase. Therefore these parameters are implied and not variable. As are 8 bit
words and MSB first.
@param[in]	devnum		SPI peripheral number (1..2)
@param[in]	brps		Baudrate prescaler, F_CPU dependent
*/
void spi_init(uint8_t devnum, uint16_t brps)
{
	struct SPI_DevDef* pdef = spi_get_pdef(devnum);

	// clock config
	if( pdef->spi_apb == 1 ) {
		RCC_APB1PeriphClockCmd(pdef->spi_clk, ENABLE);
	} else {
		RCC_APB2PeriphClockCmd(pdef->spi_clk, ENABLE);
	}
	RCC_APB2PeriphClockCmd(pdef->gpio_clk, ENABLE);

	// GPIO config
	GPIO_InitTypeDef iotd;

	iotd.GPIO_Pin = pdef->pin_sck | pdef->pin_mosi;
	iotd.GPIO_Speed = GPIO_Speed_50MHz;
	iotd.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(pdef->gpio, &iotd);

	iotd.GPIO_Pin = pdef->pin_nss;
	iotd.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(pdef->gpio, &iotd);

	iotd.GPIO_Pin = pdef->pin_miso;
	iotd.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(pdef->gpio, &iotd);

	// NSS high
	spi_cs(devnum, 1);

	// SPI config
	SPI_InitTypeDef sptd;
	sptd.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	sptd.SPI_Mode = SPI_Mode_Master;
	sptd.SPI_DataSize = SPI_DataSize_8b;
	sptd.SPI_CPOL = SPI_CPOL_Low;
	sptd.SPI_CPHA = SPI_CPHA_1Edge;
	sptd.SPI_NSS = SPI_NSS_Soft;
	sptd.SPI_BaudRatePrescaler = brps;
	sptd.SPI_FirstBit = SPI_FirstBit_MSB;
	sptd.SPI_CRCPolynomial = 7;
	SPI_Init(pdef->spi, &sptd);

	SPI_Cmd(pdef->spi, ENABLE);
}

/**
@brief Send and receive byte
@param[in]		devnum		SPI peripheral number (1..2)
@param[in,out]	d			Byte to send/receive
*/
void spi_putc(uint8_t devnum, uint8_t* d)
{
	spi_cs(devnum, 0);
	*d = spi_rw(devnum, *d);
	spi_cs(devnum, 1);
}

/**
@brief Send and receive string.
@param[in]		devnum		SPI peripheral number (1..2)
@param[in,out]	s			Zero terminated string to send/receive
*/
void spi_puts(uint8_t devnum, char* s)
{
	spi_cs(devnum, 0);
	while( *s ) {
		*s = spi_rw(devnum, *s);
		s++;
	}
	spi_cs(devnum, 1);
}

/**
@brief Send and receive string of length n.
@param[in]		devnum		SPI peripheral number (1..2)
@param[in,out]	s			String to send/receive
@param[in]		n			Number of bytes to send/receive
*/
void spi_putsn(uint8_t devnum, char* s, uint16_t n)
{
	spi_cs(devnum, 0);
	while( n-- ) {
		*s = spi_rw(devnum, *s);
		s++;
	}
	spi_cs(devnum, 1);
}
