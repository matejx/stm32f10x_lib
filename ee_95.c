/**
@file		ee_95.c
@brief		ST95P08 EEPROM routines
@author		Matej Kogovsek (matej@hamradio.si)
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
@note		Written for ST95P08. Compatibility with other SPI EE devices unknown.
*/

#include <inttypes.h>
#include <string.h>

#include "spi.h"

#define EE95_CS_HIGH spi_cs(n, 1) /**< Set NSS macro */
#define EE95_CS_LOW spi_cs(n, 0) /**< Clear NSS macro */

extern void _delay_ms(uint32_t); /**< @brief extern */

/**
@brief Read status register.
@param[in]	n	SPI peripheral number
@return SR
*/
uint8_t ee95_rdsr(uint8_t n)
{
	EE95_CS_LOW;

	spi_rw(n, 0x05);	// RDSR
	uint8_t sr = spi_rw(n, 0);

	EE95_CS_HIGH;
	return sr;
}

/**
@brief Init EE.
@param[in]	n	SPI peripheral number
*/
void ee95_init(uint8_t n)
{
	EE95_CS_HIGH;
}

/**
@brief Read from EE.
@param[in]	n		SPI peripheral number
@param[in]	adr		Starting byte address
@param[out]	buf		Caller provided buffer for data
@param[in]	len		Number of bytes to read (len <= sizeof(buf))
@return Always returns 0.
*/
uint8_t ee95_rd(uint8_t n, uint32_t adr, uint8_t* buf, uint16_t len)
{
	EE95_CS_LOW;

	spi_rw(n, ((adr >> 5) & 0x18) | 0x03 ); // READ instruction containing A9 and A8
	spi_rw(n, adr & 0xff ); // address A7..A0
	memset(buf, 0, len);
	spi_putsn(n, (char*)buf, len);

	EE95_CS_HIGH;

	return 0;
}

/**
@brief EE write enable.
@param[in]	n		SPI peripheral number
*/
void ee95_wren(uint8_t n)
{
	EE95_CS_LOW;

	spi_rw(n, 0x06);	// WREN

	EE95_CS_HIGH;
}

/**
@brief Write to flash.
@param[in]	n		SPI peripheral number
@param[in]	adr		Starting byte address
@param[in]	buf		Caller provided buffer with data
@param[in]	len		Number of bytes to write (len <= sizeof(buf))
@return Always returns 0
@warning Can hang waiting for correct SR.
*/
uint8_t ee95_wr(uint8_t n, uint32_t adr, uint8_t* buf, uint16_t len)
{
	EE95_CS_LOW;

	spi_rw(n, ((adr >> 5) & 0x18) | 0x02 ); // WRITE instruction containing A9 and A8
	spi_rw(n, adr & 0xff ); // address A7..A0
	while( len-- ) {
		spi_rw(n, *buf);
		buf++;
	}

	EE95_CS_HIGH;

	while( ee95_rdsr(n) & 1 ) {
		_delay_ms(1);
	}

	return 0;
}
