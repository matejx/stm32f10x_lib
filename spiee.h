#ifndef MAT_SPIEE_H
#define MAT_SPIEE_H

#include <inttypes.h>

void spiee_init(uint8_t n);

uint8_t spiee_rd(uint8_t n, uint32_t adr, uint8_t* buf, uint16_t len);

void spiee_wren(uint8_t n);

uint8_t spiee_wr(uint8_t n, uint32_t adr, uint8_t* buf, uint16_t len);

#endif
