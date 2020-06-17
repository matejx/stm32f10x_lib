// ------------------------------------------------------------------
// --- spi.h - Basic routines for sending data over SPI           ---
// ---                                                            ---
// ---            24.jul.2013, Matej Kogovsek (matej@hamradio.si) ---
// ------------------------------------------------------------------

#ifndef MAT_SPI_H
#define MAT_SPI_H

#include <inttypes.h>

void spi_init(uint8_t devnum, const uint16_t brps, const uint8_t mode);
void spi_putc(uint8_t devnum, uint8_t* d);
void spi_puts(uint8_t devnum, char* s);
void spi_putsn(uint8_t devnum, char* s, uint16_t n);

// low level routines
void spi_cs(uint8_t devnum, uint8_t nss);
uint8_t spi_rw(uint8_t devnum, const uint8_t d);

#endif
