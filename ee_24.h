#ifndef MAT_EE24_H
#define MAT_EE24_H

#include <stm32f10x_i2c.h>

uint8_t ee24_rd(const uint8_t n, uint32_t adr, uint8_t* buf, uint16_t len);
uint8_t ee24_wr(const uint8_t n, uint32_t adr, uint8_t* buf, uint16_t len);

#endif
