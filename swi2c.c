/**
@file		swi2c.c
@brief		Bitbang I2C master routines
@author		Matej Kogovsek (matej@hamradio.si)
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
*/

#include "stm32f10x.h"
/*
#define SDA_PORT GPIOA
#define SDA_PIN GPIO_Pin_9

#define SCL_PORT GPIOA
#define SCL_PIN GPIO_Pin_10
*/
#ifndef SWI2C_DELAY_US
#define SWI2C_DELAY_US 5
#endif

extern void _delay_us(uint32_t);
extern void DDR(GPIO_TypeDef*, uint16_t, GPIOMode_TypeDef);

// ------------------------------------------------------------------

void swi2c_scl(uint8_t b)
{
	if( b ) {
		GPIO_SetBits(SWI2C_SCL_PORT, SWI2C_SCL_PIN);
	} else {
		GPIO_ResetBits(SWI2C_SCL_PORT, SWI2C_SCL_PIN);
	}
}

void swi2c_sda(uint8_t b)
{
	if( b ) {
		GPIO_SetBits(SWI2C_SDA_PORT, SWI2C_SDA_PIN);
	} else {
		GPIO_ResetBits(SWI2C_SDA_PORT, SWI2C_SDA_PIN);
	}
}

uint8_t swi2c_sda_state(void)
{
	return GPIO_ReadInputDataBit(SWI2C_SDA_PORT, SWI2C_SDA_PIN) == Bit_SET;
}

void swi2c_delay(void)
{
	_delay_us(SWI2C_DELAY_US);
}

// ------------------------------------------------------------------

void swi2c_start(void)
{
	swi2c_scl(1);
	swi2c_sda(1);
	swi2c_delay();
	swi2c_sda(0);
	swi2c_delay();
	swi2c_scl(0);
	swi2c_delay();
}  // SCL and SDA are low after swi2c_start

// ------------------------------------------------------------------

void swi2c_stop(void)
{
	swi2c_sda(0);
	swi2c_delay();
	swi2c_scl(1);
	swi2c_delay();
	swi2c_sda(1);
}  // SCL and SDA are high after swi2c_stop

// ------------------------------------------------------------------

uint8_t swi2c_putc(const uint8_t d)
{
	uint8_t i = 0x80;
	while( i ) {
		swi2c_sda(d & i);
		swi2c_delay();
		swi2c_scl(1);
		swi2c_delay();
		swi2c_delay();
		swi2c_scl(0);
		swi2c_delay();
		i >>= 1;
	}
	// get ACK
	swi2c_sda(1);
	swi2c_delay();
	swi2c_scl(1);
	swi2c_delay();
	uint8_t r = swi2c_sda_state();	// read ack
	swi2c_delay();
	swi2c_scl(0);
	swi2c_delay();
	return r;
}  // SCL low, SDA high after swi2c_putc

// ------------------------------------------------------------------

uint8_t swi2c_getc(void)
{
	uint8_t d = 0;
	uint8_t i = 0x80;

	while( i ) {
		swi2c_delay();
		swi2c_scl(1);
		swi2c_delay();
		if( swi2c_sda_state() ) d |= i;	// read bit
		swi2c_delay();
		swi2c_scl(0);
		swi2c_delay();
		i >>= 1;
	}
	// gen ACK
	swi2c_sda(0);
	swi2c_delay();
	swi2c_scl(1);
	swi2c_delay();
	swi2c_delay();
	swi2c_scl(0);
	swi2c_delay();

	return d;
}  // both SCL and SDA are low after swi2c_getc

/** @publicsection */

void i2c_init(uint8_t devnum, const uint32_t clkspd)
{
	DDR(SWI2C_SCL_PORT, SWI2C_SCL_PIN, GPIO_Mode_Out_OD);
	DDR(SWI2C_SDA_PORT, SWI2C_SDA_PIN, GPIO_Mode_Out_OD);
}

uint8_t i2c_rd(uint8_t devnum, uint8_t adr, uint8_t* buf, uint32_t len)
{
	swi2c_start();
	swi2c_putc(adr | 1);
	for( uint8_t i = 0; i < len; ++i ) {
		buf[i] = swi2c_getc();
	}
	swi2c_stop();

	return 0;
}

uint8_t i2c_wr(uint8_t devnum, uint8_t adr, const uint8_t* buf, uint32_t len)
{
	swi2c_start();
	swi2c_putc(adr & 0xfe);
	for( uint8_t i = 0; i < len; ++i ) {
		swi2c_putc(buf[i]);
	}
	swi2c_stop();

	return 0;
}
