/**
@file		fls_25.c
@brief		Spansion S25FL SPI flash routines
@author		Matej Kogovsek
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
@note		Tested with S25FL208 1Mbyte flash. Likely works with other densities and other manufacturers' x25 serial flash chips.
@warning	The code assumes 24 bit (3 byte) data addressing. Devices with more than 32MByte (or less than ?) will require code change.
*/

#include <inttypes.h>

#include "spi.h"

extern void _delay_ms(uint32_t); /**< @brief extern */
extern void fls25_cs(uint8_t devnum, uint8_t nss); /**< @brief extern */

#define FLS25_NSS_HIGH fls25_cs(n, 1) /**< Set NSS macro */
#define FLS25_NSS_LOW fls25_cs(n, 0)	/**< Clear NSS macro */

/**
@brief Wait for flash to become ready.
@param[in]	n		SPI peripheral number
@param[in]	to_ms	Timeout in ms
@return True if timeout expired, false otherwise.
*/
uint8_t fls25_waitready(const uint8_t n, uint16_t to_ms)
{
	FLS25_NSS_LOW;

	spi_rw(n, 0x05);
	while( --to_ms && (1 & spi_rw(n, 0)) ) {
		//wdt_reset();
		_delay_ms(1);
	}

	FLS25_NSS_HIGH;

	return (to_ms == 0);
}

/**
@brief Init flash.
@param[in]	n		SPI peripheral number
*/
void fls25_init(const uint8_t n)
{
	FLS25_NSS_HIGH;
}

/**
@brief Read from flash.
@param[in]	n		SPI peripheral number
@param[in]	adr		Starting byte address
@param[out]	buf		Caller provided buffer for data
@param[in]	len		Number of bytes to read (len <= sizeof(buf))
@return Always returns 0.
*/
uint8_t fls25_rd(const uint8_t n, const uint32_t adr, uint8_t* buf, uint16_t len)
{
	FLS25_NSS_LOW;

	spi_rw(n, 0x03);
	spi_rw(n, adr >> 16);
	spi_rw(n, adr >> 8);
	spi_rw(n, adr);

	while( len-- ) {
		*buf = spi_rw(n, 0);
		buf++;
	}

	FLS25_NSS_HIGH;

	return 0;
}

/**
@brief Flash write enable.
@param[in]	n		SPI peripheral number
*/
void fls25_we(const uint8_t n)
{
	FLS25_NSS_LOW;

	spi_rw(n, 0x06);

	FLS25_NSS_HIGH;
}

/**
@brief Write to flash.
@param[in]	n		SPI peripheral number
@param[in]	adr		Starting byte address
@param[in]	buf		Caller provided buffer with data
@param[in]	len		Number of bytes to write (len <= sizeof(buf))
@return Same as fls25_waitready
*/
uint8_t fls25_wr(const uint8_t n, const uint32_t adr, uint8_t* buf, uint16_t len)
{
	FLS25_NSS_LOW;

	spi_rw(n, 0x02);
	spi_rw(n, adr >> 16);
	spi_rw(n, adr >> 8);
	spi_rw(n, adr);

	while( len-- ) {
		spi_rw(n, *buf);
		buf++;
	}

	FLS25_NSS_HIGH;

	return fls25_waitready(n, 100);
}

/*
uint8_t fls25_busy(void)
{
	FLS25_NSS_LOW;

	spi_rw(n, 0x05);
	uint8_t d = spi_rw(n, 0);

	FLS25_NSS_HIGH;

	return (d & 1);	// busy = bit 0
}
*/

/**
@brief Execute erase device command.
@param[in]	n		SPI peripheral number
@return Same as fls25_waitready
*/
uint8_t fls25_chiperase(const uint8_t n)
{
	FLS25_NSS_LOW;

	spi_rw(n, 0xC7);

	FLS25_NSS_HIGH;

	return fls25_waitready(n, 30000);
}

/**
@brief Execute erase sector command.
@param[in]	n		SPI peripheral number
@param[in]	adr		Address of any byte in sector to erase.
@return Same as fls25_waitready
*/
uint8_t fls25_secterase(const uint8_t n, const uint16_t adr)
{
	FLS25_NSS_LOW;

	spi_rw(n, 0x20);
	spi_rw(n, 0);
	spi_rw(n, adr >> 8);
	spi_rw(n, adr);

	FLS25_NSS_HIGH;

	return fls25_waitready(n, 500);
}

/**
@brief Execute read manufacturer/device ID command.
@param[in]	n		SPI peripheral number
@return 32bit JEDEC assigned ID
*/
uint32_t fls25_id(const uint8_t n)
{
	FLS25_NSS_LOW;

	uint32_t r = 0;

	spi_rw(n, 0x9f);
	r += spi_rw(n, 0);
	r <<= 8;
	r += spi_rw(n, 0);
	r <<= 8;
	r += spi_rw(n, 0);

	FLS25_NSS_HIGH;

	return r;
}

/**
@brief Clear status (unprotects blocks).
@param[in]	n		SPI peripheral number
*/
void fls25_clrstat(const uint8_t n)
{
	FLS25_NSS_LOW;

	spi_rw(n, 0x01);
	spi_rw(n, 0);
	spi_rw(n, 0);

	FLS25_NSS_HIGH;

	fls25_waitready(n, 10);
}
