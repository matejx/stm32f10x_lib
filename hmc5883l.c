
#include "stm32f10x.h"
#include "i2c.h"
#include "hmc5883l.h"
#include <math.h>

const uint8_t HMC_I2C_ADDR = 0x3c;

uint8_t hmc_init(uint8_t cra, uint8_t crb, uint8_t mode)
{
	uint8_t b[4] = {0, cra, crb, mode};

	return i2c_wr(I2C1, b, 4, HMC_I2C_ADDR);
}

uint8_t hmc_read(int16_t* x, int16_t* y, int16_t* z)
{
	uint8_t b[6] = {3};

	if( i2c_wr(I2C1, b, 1, HMC_I2C_ADDR) ) { return 1; }

	if( i2c_rd(I2C1, b, 6, HMC_I2C_ADDR) ) { return 1; }
	
	*x = (((int16_t)b[0]) << 8) | b[1];
	*z = (((int16_t)b[2]) << 8) | b[3];
	*y = (((int16_t)b[4]) << 8) | b[5];
	
	return 0;
}

float hmc_heading(int16_t x, int16_t y)
{
	float heading = atan2(y, x);
	
	if( heading < 0 ) { heading += 2 * M_PI; }
	
	return heading * 180 / M_PI;
}

bool hmc_present(void)
{
	uint8_t b[3] = {10};
	
	if( i2c_wr(I2C1, b, 1, HMC_I2C_ADDR) ) { return false; }
	
	if( i2c_rd(I2C1, b, 3, HMC_I2C_ADDR) ) { return false; }
	
	return (b[0] == 'H') && (b[1] == '4') && (b[2] == '3');
}
