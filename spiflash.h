// ------------------------------------------------------------------
// --- spiflash.c - serial flash routines                         ---
// ---                                                            ---
// ---                                 4.mar.2011, Matej Kogovsek ---
// ------------------------------------------------------------------

#ifndef MAT_SPIFLASH_H
#define MAT_SPIFLASH_H

#include <inttypes.h>

void fls_init(const uint8_t n);

uint8_t fls_rd(const uint8_t n, const uint32_t adr, uint8_t* s, uint16_t len);

void fls_we(const uint8_t n);
uint8_t fls_wr(const uint8_t n, const uint32_t adr, uint8_t* s, uint16_t len);

uint8_t fls_secterase(const uint8_t n, const uint16_t adr);
uint8_t fls_chiperase(const uint8_t n);

uint32_t fls_id(const uint8_t n);

#endif
