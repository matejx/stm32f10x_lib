#ifndef MAT_SPIFLASH_H
#define MAT_SPIFLASH_H

#include <inttypes.h>

void fls25_init(const uint8_t n);

uint8_t fls25_rd(const uint8_t n, const uint32_t adr, uint8_t* buf, uint16_t len);

void fls25_we(const uint8_t n);
uint8_t fls25_wr(const uint8_t n, const uint32_t adr, uint8_t* buf, uint16_t len);

uint8_t fls25_secterase(const uint8_t n, const uint16_t adr);
uint8_t fls25_chiperase(const uint8_t n);

uint32_t fls25_id(const uint8_t n);
void fls25_clrstat(const uint8_t n);

#endif
