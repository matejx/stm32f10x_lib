#ifndef I2C_H
#define I2C_H

#include <stm32f10x_i2c.h>

#define I2C_100K 100000

void i2c_init(uint8_t devnum, const uint32_t clkspd);
uint8_t i2c_rd(uint8_t devnum, uint8_t adr, uint8_t* buf, uint32_t len);
uint8_t i2c_wr(uint8_t devnum, uint8_t adr, const uint8_t* buf, uint32_t len);

#endif
