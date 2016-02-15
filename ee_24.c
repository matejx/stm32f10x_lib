/**
@file		ee_24.c
@brief		24Cxxx EEPROM routines
@author		Matej Kogovsek (matej@hamradio.si)
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
@note		Tested with 24C256.
@warning	The code assumes 16 bit (2 byte) EE data addressing. Devices with more than 65kByte (or less than 256) will require code change.
*/

#include <inttypes.h>

#include "i2c.h"
#include "string.h"

uint8_t EE24_I2C_ADR = 0xa0; /**< EE I2C address */

extern void _delay_ms(uint32_t); /**< @brief extern */

/**
@brief Write to EE.
@param[in]	n		I2C peripheral number
@param[in]	adr		Starting byte address
@param[in]	buf		Pointer to data
@param[in]	len		Number of bytes to write (len <= sizeof(buf))
@return Same as i2c_wr
*/
uint8_t ee24_wr(const uint8_t n, uint32_t adr, uint8_t* buf, uint16_t len)
{
	if( len > 64 ) return 1;

	uint8_t buf2[len+2]; // two byte address assumed
	buf2[0] = adr >> 8;
	buf2[1] = adr;

	if( len ) memcpy(buf2+2, buf, len);

	uint8_t r = i2c_wr(n, EE24_I2C_ADR, buf2, len+2);
	_delay_ms(10);
	return r;
}

/**
@brief Read from EE.
@param[in]	n		I2C peripheral number
@param[in]	adr		Starting byte address
@param[in]	buf		Pointer to caller allocated buffer
@param[in]	len		Number of bytes to read (len <= sizeof(buf))
@return Same as i2c_rd
*/
uint8_t ee24_rd(const uint8_t n, uint32_t adr, uint8_t* buf, uint16_t len)
{
	if( ee24_wr(n, adr, 0, 0) ) return 1;
	return i2c_rd(n, EE24_I2C_ADR, buf, len);
}
