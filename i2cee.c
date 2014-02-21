// ------------------------------------------------------------------
// --- i2cee.c - I2C EE routines for 24Cxxx                       ---
// ---                                                            ---
// ---                                30.nov.2013, Matej Kogovsek ---
// ------------------------------------------------------------------

#include <inttypes.h>

#include "i2c.h"
#include "string.h"

#define EE_I2C_ADR 0xa0

extern void _delay_ms(uint32_t);

// ------------------------------------------------------------------
// PUBLIC FUNCTIONS
// ------------------------------------------------------------------

uint8_t i2cee_wr(const uint8_t n, uint32_t adr, uint8_t* buf, uint16_t len)
{
	if( len > 64 ) return 1;
	
	uint8_t buf2[len+2];
	buf2[0] = adr >> 8;
	buf2[1] = adr;
	
	if( len ) memcpy(buf2+2, buf, len);
	
	uint8_t r = i2c_wr(n, EE_I2C_ADR, buf2, len+2);
	_delay_ms(10);
	return r;
}

// ------------------------------------------------------------------

uint8_t i2cee_rd(const uint8_t n, uint32_t adr, uint8_t* buf, uint16_t len)
{
	if( i2cee_wr(n, adr, 0, 0) ) return 1;
	return i2c_rd(n, EE_I2C_ADR, buf, len);
}

// ------------------------------------------------------------------
