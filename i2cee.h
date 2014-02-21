#ifndef MAT_I2CEE_H
#define MAT_I2CEE_H

#include <stm32f10x_i2c.h>

uint8_t i2cee_rd(const uint8_t n, uint32_t adr, uint8_t* buf, uint16_t len);
uint8_t i2cee_wr(const uint8_t n, uint32_t adr, uint8_t* buf, uint16_t len);

#endif
