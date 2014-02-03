#ifndef I2C_H
#define I2C_H

#include <stm32f10x_i2c.h>

void i2c_init(uint8_t I2Cy, const uint32_t clkspd);
uint8_t i2c_rd(uint8_t I2Cy, uint8_t adr, uint8_t* buf, uint32_t nbuf);
uint8_t i2c_wr(uint8_t I2Cy, uint8_t adr, const uint8_t* buf, uint32_t nbuf);

#endif
