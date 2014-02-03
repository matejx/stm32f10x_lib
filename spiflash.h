// ------------------------------------------------------------------
// --- spiflash.c - serial flash routines                         ---
// ---                                                            ---
// ---                                 4.mar.2011, Matej Kogovsek ---
// ------------------------------------------------------------------

#ifndef MAT_SPIFLASH_H
#define MAT_SPIFLASH_H

#include <inttypes.h>

void fls_init(const uint8_t n);
void fls_gets(const uint8_t n, const uint32_t adr, char* s, uint16_t len);
uint8_t fls_getc(const uint8_t n, const uint32_t adr);
void fls_we(const uint8_t n);
uint8_t fls_puts(const uint8_t n, const uint32_t adr, char* s, uint16_t len);
void fls_putc(const uint8_t n, const uint32_t adr, char c);
uint8_t fls_secterase(const uint8_t n, const uint16_t adr);
uint8_t fls_chiperase(const uint8_t n);
uint32_t fls_id(const uint8_t n);
uint16_t fls_status(const uint8_t n);
void fls_clrstat(const uint8_t n);
uint8_t fls_empty(const uint8_t n, uint32_t fs);

#endif
