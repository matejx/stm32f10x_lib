// ------------------------------------------------------------------
// --- spiflash.c - serial flash routines                         ---
// ---                                                            ---
// ---                                 4.mar.2011, Matej Kogovsek ---
// ------------------------------------------------------------------

#include <inttypes.h>

#include "spi.h"

extern void _delay_ms(uint32_t);

#define SPIFLS_SS_LOW spi_cs(n, 0)
#define SPIFLS_SS_HIGH spi_cs(n, 1)

void fls_init(const uint8_t n)
{
/*	DDR(SPIFLS_SS_PORT) |= _BV(SPIFLS_SS_BIT);
	SPIFLS_SS_HIGH;

	spi_init(1);		// Fosc / 16*/
}

// ------------------------------------------------------------------

uint8_t fls_waitready(const uint8_t n, uint16_t to_ms)
{
	SPIFLS_SS_LOW;
	
	spi_rw(n, 0x05);
	while( --to_ms && (1 & spi_rw(n, 0)) ) {
		//wdt_reset();
		_delay_ms(1);
	}

	SPIFLS_SS_HIGH;	
	
	return (to_ms == 0);
}

// ------------------------------------------------------------------

void fls_gets(const uint8_t n, const uint32_t adr, char* s, uint16_t len)
{
	SPIFLS_SS_LOW;
	
	spi_rw(n, 0x03);
	spi_rw(n, adr >> 16);
	spi_rw(n, adr >> 8);
	spi_rw(n, adr);
	
	while( len-- ) {
		*s = spi_rw(n, 0);
		s++;
	}

	SPIFLS_SS_HIGH;	
}

// ------------------------------------------------------------------

uint8_t fls_getc(const uint8_t n, const uint32_t adr)
{
	char c;
	
	fls_gets(n, adr, &c, 1);
	
	return c;
}

// ------------------------------------------------------------------
/*
uint32_t fls_used(uint32_t fs)
{
	SPIFLS_SS_LOW;
	
	spi_rw(n, 0x03);
	spi_rw(n, 0);
	spi_rw(n, 0);
	spi_rw(n, 0);
	
	uint32_t r = 0;
	uint32_t i;
	for( i = 0; i < fs; i++) {
		if( 0xff != spi_rw(n, 0) )	
			r = i;
	}
	
	SPIFLS_SS_HIGH;
	
	return r;
}
*/
// ------------------------------------------------------------------

void fls_we(const uint8_t n)
{
	SPIFLS_SS_LOW;
	
	spi_rw(n, 0x06);
	
	SPIFLS_SS_HIGH;	
}

// ------------------------------------------------------------------

uint8_t fls_puts(const uint8_t n, const uint32_t adr, char* s, uint16_t len)
{
	SPIFLS_SS_LOW;

	spi_rw(n, 0x02);
	spi_rw(n, adr >> 16);
	spi_rw(n, adr >> 8);
	spi_rw(n, adr);

	while( len-- ) {
		spi_rw(n, *s);
		s++;
	}
	
	SPIFLS_SS_HIGH;	
	
	return fls_waitready(n, 100);
}

// ------------------------------------------------------------------

void fls_putc(const uint8_t n, const uint32_t adr, char c)
{
	fls_puts(n, adr, &c, 1);
}

// ------------------------------------------------------------------
/*
uint8_t fls_busy(void)
{
	SPIFLS_SS_LOW;
	
	spi_rw(n, 0x05);
	uint8_t d = spi_rw(n, 0);

	SPIFLS_SS_HIGH;

	return (d & 1);	// busy = bit 0
}
*/
// ------------------------------------------------------------------

uint8_t fls_chiperase(const uint8_t n)
{
	SPIFLS_SS_LOW;
	
	spi_rw(n, 0xC7);
	
	SPIFLS_SS_HIGH;
	
	return fls_waitready(n, 30000);
}

// ------------------------------------------------------------------

uint8_t fls_secterase(const uint8_t n, const uint16_t adr)
{
	SPIFLS_SS_LOW;
	
	spi_rw(n, 0x20);
	spi_rw(n, 0);
	spi_rw(n, adr >> 8);
	spi_rw(n, adr);
	
	SPIFLS_SS_HIGH;
	
	return fls_waitready(n, 500);
}

// ------------------------------------------------------------------

uint32_t fls_id(const uint8_t n)
{
	SPIFLS_SS_LOW;
	
	uint32_t r = 0;

	spi_rw(n, 0x9f);
	r += spi_rw(n, 0);
	r <<= 8;
	r += spi_rw(n, 0);
	r <<= 8;
	r += spi_rw(n, 0);

	SPIFLS_SS_HIGH;
	
	return r;
}

// ------------------------------------------------------------------

uint16_t fls_status(const uint8_t n)
{
	// read status 1
	SPIFLS_SS_LOW;
	spi_rw(n, 0x05);
	uint8_t s1 = spi_rw(n, 0);
	SPIFLS_SS_HIGH;
	
	asm("nop\nnop\nnop\n");
	
	// read status 2
	SPIFLS_SS_LOW;
	spi_rw(n, 0x35);
	uint8_t s2 = spi_rw(n, 0);
	SPIFLS_SS_HIGH;

	return (s1 * 0x100) + s2;
}

// ------------------------------------------------------------------

void fls_clrstat(const uint8_t n)
{
	SPIFLS_SS_LOW;
	
	spi_rw(n, 1);
	spi_rw(n, 0);
	spi_rw(n, 0);

	SPIFLS_SS_HIGH;
	
	fls_waitready(n, 10);
}

// ------------------------------------------------------------------

uint8_t fls_empty(const uint8_t n, uint32_t fs)
{
	SPIFLS_SS_LOW;

	spi_rw(n, 0x03);
	spi_rw(n, 0);
	spi_rw(n, 0);
	spi_rw(n, 0);
	
	while( fs ) {
		fs--;
		if( 0xff != spi_rw(n, 0) ) break;
		//wdt_reset();
	}

	SPIFLS_SS_HIGH;
	
	return (0 == fs);
}
