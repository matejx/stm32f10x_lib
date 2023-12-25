/**
@file		hmc5883l.c
@brief		Honeywell HMC5883L 3-Axis digital compass functions
@author		Matej Kogovsek
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
@warning	Not thoroughly tested.
*/

#include "stm32f10x.h"
#include "i2c.h"
#include "hmc5883l.h"
#include <math.h>

const uint8_t HMC_I2C_ADDR = 0x3c; /**< HMC5883L I2C address */
uint8_t hmcDev = 1; /**< I2C peripheral number to use */

/**
@brief Init HMC5883L.
@param[in]	cra		Control register A
@param[in]	crb		Control register B
@param[in]	mode	Mode
@return Same as i2c_wr
*/
uint8_t hmc_init(uint8_t cra, uint8_t crb, uint8_t mode)
{
	uint8_t b[4] = {0, cra, crb, mode};

	return i2c_wr(hmcDev, HMC_I2C_ADDR, b, 4);
}

/**
@brief Read acc. values.
@param[out]	x	Pointer to x
@param[out]	y	Pointer to y
@param[out]	z	Pointer to z
@return True on success, false otherwise
*/
uint8_t hmc_read(int16_t* x, int16_t* y, int16_t* z)
{
	uint8_t b[6] = {3};

	if( i2c_wr(hmcDev, HMC_I2C_ADDR, b, 1) ) { return 0; }

	if( i2c_rd(hmcDev, HMC_I2C_ADDR, b, 6) ) { return 0; }

	*x = (((int16_t)b[0]) << 8) | b[1];
	*z = (((int16_t)b[2]) << 8) | b[3];
	*y = (((int16_t)b[4]) << 8) | b[5];

	return 1;
}

/**
@brief Convert x,y to heading.
@param[in]	x	x from HMC
@param[in]	y	y from HMC
@return Heading
*/
float hmc_heading(int16_t x, int16_t y)
{
	float heading = atan2(y, x);

	if( heading < 0 ) { heading += 2 * M_PI; }

	return heading * 180 / M_PI;
}

/**
@brief Check for HMC5883L presence on I2C
@return True on present, false otherwise
*/
uint8_t hmc_present(void)
{
	uint8_t b[3] = {10};

	if( i2c_wr(hmcDev, HMC_I2C_ADDR, b, 1) ) { return 0; }

	if( i2c_rd(hmcDev, HMC_I2C_ADDR, b, 3) ) { return 0; }

	return (b[0] == 'H') && (b[1] == '4') && (b[2] == '3');
}
