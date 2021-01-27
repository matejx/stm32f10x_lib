/**
@file		lcd_io.c
@brief		HD44780 via IO pins, 8 bit (low level)
@author		Matej Kogovsek (matej@hamradio.si)
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
@note		Implements low level functions required by lcd.c
@note		Pin mapping is defined in lcd_io_pindef.h
*/

#include <inttypes.h>

#include <stm32f10x_gpio.h>
#include "misc.h"

// ------------------------------------------------------------------
// --- defines ------------------------------------------------------
// ------------------------------------------------------------------

#include "lcd_io_pindef.h"

static GPIO_TypeDef* LCD_DATA_PORTS[] = {LCD_D0_PORT, LCD_D1_PORT, LCD_D2_PORT, LCD_D3_PORT, LCD_D4_PORT, LCD_D5_PORT, LCD_D6_PORT, LCD_D7_PORT};
static uint16_t LCD_DATA_BITS[] = {LCD_D0_BIT, LCD_D1_BIT, LCD_D2_BIT, LCD_D3_BIT, LCD_D4_BIT, LCD_D5_BIT, LCD_D6_BIT, LCD_D7_BIT};

const uint8_t lcd_busw = 0x10; /**< Actual LCD bus width = 4 bit */
extern void _delay_us(uint32_t); /**< @brief extern */

/** @privatesection */

#define LCD_RS_1 GPIO_WriteBit(LCD_RS_PORT, LCD_RS_BIT, Bit_SET) /**< Set RS macro */
#define LCD_RS_0 GPIO_WriteBit(LCD_RS_PORT, LCD_RS_BIT, Bit_RESET) /**< Clear RS macro */

#define LCD_RW_1 GPIO_WriteBit(LCD_RW_PORT, LCD_RW_BIT, Bit_SET) /**< Set RW macro */
#define LCD_RW_0 GPIO_WriteBit(LCD_RW_PORT, LCD_RW_BIT, Bit_RESET) /**< Clear RW macro */

#define LCD_EN_1 GPIO_WriteBit(LCD_EN_PORT, LCD_EN_BIT, Bit_SET) /**< Set EN macro */
#define LCD_EN_0 GPIO_WriteBit(LCD_EN_PORT, LCD_EN_BIT, Bit_RESET) /**< Clear EN macro */

#define _BV(x) (1 << x) /**< Bit value macro */

// ------------------------------------------------------------------
// --- private procedures -------------------------------------------
// ------------------------------------------------------------------

void lcd_ddir(GPIOMode_TypeDef mode)
{
	uint8_t i;
	for( i = 0; i < 8; ++i ) {
		misc_gpio_config(LCD_DATA_PORTS[i], LCD_DATA_BITS[i], mode);
	}
}

void lcd_dout(uint8_t data)
{
	uint8_t i;
	for( i = 0; i < 8; ++i ) {
		GPIO_WriteBit(LCD_DATA_PORTS[i], LCD_DATA_BITS[i], data & _BV(i) ? Bit_SET : Bit_RESET);
	}
}

uint8_t lcd_din(void)
{
	uint8_t r = 0;
	uint8_t i;
	for( i = 0; i < 8; ++i ) {	if( Bit_SET == GPIO_ReadInputDataBit(LCD_D0_PORT, LCD_D0_BIT) ) r |= _BV(0);
		if( Bit_SET == GPIO_ReadInputDataBit(LCD_DATA_PORTS[i], LCD_DATA_BITS[i]) ) r |= _BV(i);
	}
	return r;
}

// writes instruction or data to LCD
void lcd_out(uint8_t data, uint8_t rs)
{
	if( rs ) {LCD_RS_1;} else {LCD_RS_0;}	// select instruction(0) or data(1)
	LCD_RW_0;				// RW low (write)
	lcd_ddir(GPIO_Mode_Out_PP);
	lcd_dout(data);
	LCD_EN_1;				// E high
	_delay_us(2);
	LCD_EN_0;				// high to low on E to clock data
	_delay_us(2);
}

// reads instruction or data from LCD
uint8_t lcd_in(uint8_t rs)
{
	lcd_ddir(GPIO_Mode_IN_FLOATING);
	lcd_dout(0xff);	// pullups on
	if( rs ) {LCD_RS_1;} else {LCD_RS_0;}	// select instruction/data
	LCD_RW_1;		// RW high (read)
	LCD_EN_1;
	_delay_us(2);
	uint8_t r = lcd_din();
	LCD_EN_0;
	_delay_us(2);

	return r;
}

// reads busy flag
uint8_t lcd_busy(void)
{
	return( lcd_in(0) & _BV(7) );
}

// returns 0(false) on timeout and 1-20(true) on lcd available
uint8_t lcd_available(void)
{
	uint8_t i = 100;			// wait max 10ms
	while( lcd_busy() && i ) {
		_delay_us(100);
		i--;
	}
	return i;
}

// write command to lcd
uint8_t lcd_command(uint8_t data)
{
	if( lcd_available() ) {
		lcd_out(data, 0);
		return 1;
	}

	return 0;
}

// write data to lcd
uint8_t lcd_data(uint8_t data)
{
	if( lcd_available() ) {
		lcd_out(data, 1);
		return 1;
	}

	return 0;
}

void lcd_bl(uint8_t on)
{
	GPIO_WriteBit(LCD_BL_PORT, LCD_BL_BIT, on ? Bit_SET : Bit_RESET);
}

void lcd_hwinit(void)
{
	LCD_EN_0;	// idle EN is low

	// configure the 3 LCD control pins as outputs
	misc_gpio_config(LCD_RS_PORT, LCD_RS_BIT, GPIO_Mode_Out_PP);
	misc_gpio_config(LCD_RW_PORT, LCD_RW_BIT, GPIO_Mode_Out_PP);
	misc_gpio_config(LCD_EN_PORT, LCD_EN_BIT, GPIO_Mode_Out_PP);

	// make LCD BL pin an output
	misc_gpio_config(LCD_BL_PORT, LCD_BL_BIT, GPIO_Mode_Out_PP);
	lcd_bl(0);
}
