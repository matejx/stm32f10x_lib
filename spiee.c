// ------------------------------------------------------------------
// --- spiee.c                                                    ---
// --- serial EE routines for ST95P08                             ---
// ---                         Matej Kogovsek (matej@hamradio.si) ---
// ------------------------------------------------------------------

#include <inttypes.h>
#include <string.h>

#include "spi.h"

#define SPIEE_CS_LOW spi_cs(n, 0)
#define SPIEE_CS_HIGH spi_cs(n, 1)

extern void _delay_ms(uint32_t);

// ------------------------------------------------------------------
// PRIVATE FUNCTIONS
// ------------------------------------------------------------------

uint8_t spiee_rdsr(uint8_t n)
{
	SPIEE_CS_LOW;
	
	spi_rw(n, 0x05);	// RDSR
	uint8_t sr = spi_rw(n, 0);
	
	SPIEE_CS_HIGH;
	return sr;
}

// ------------------------------------------------------------------
// PUBLIC FUNCTIONS
// ------------------------------------------------------------------

void spiee_init(uint8_t n)
{
//	spi_init(n);
	SPIEE_CS_HIGH;
}

// ------------------------------------------------------------------

uint8_t spiee_rd(uint8_t n, uint32_t adr, uint8_t* buf, uint16_t len)
{
	SPIEE_CS_LOW;
	
	spi_rw(n, ((adr >> 5) & 0x18) | 0x03 ); // READ instruction containing A9 and A8
	spi_rw(n, adr & 0xff ); // address A7..A0
	memset(buf, 0, len);
	spi_putsn(n, (char*)buf, len);	
	
	SPIEE_CS_HIGH;
	
	return 0;
}

// ------------------------------------------------------------------

void spiee_wren(uint8_t n)
{
	SPIEE_CS_LOW;
	
	spi_rw(n, 0x06);	// WREN
	
	SPIEE_CS_HIGH;	
}

// ------------------------------------------------------------------

uint8_t spiee_wr(uint8_t n, uint32_t adr, uint8_t* buf, uint16_t len)
{
	SPIEE_CS_LOW;
	
	spi_rw(n, ((adr >> 5) & 0x18) | 0x02 ); // WRITE instruction containing A9 and A8
	spi_rw(n, adr & 0xff ); // address A7..A0
	while( len-- ) {
		spi_rw(n, *buf);
		buf++;
	}
	
	SPIEE_CS_HIGH;

	while( spiee_rdsr(n) & 1 ) {
		_delay_ms(1);
	}
	
	return 0;
}

// ------------------------------------------------------------------
