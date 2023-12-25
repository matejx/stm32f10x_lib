/**
@file		lcd_pcf8574.c
@brief		HD44780 via PCF8574, 4 bit (low level)
@author		Matej Kogovsek
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
@note		Implements low level functions required by lcd.c
@note		PCF8574 to HD44780 connection as per LCDproc (http://lcdproc.sourceforge.net/docs/current-user.html#hd44780-i2c.circuit)
*/

#include <inttypes.h>

#include "i2c.h"

// ------------------------------------------------------------------
// --- defines ------------------------------------------------------
// ------------------------------------------------------------------

const uint8_t lcd_busw = 0; /**< Actual LCD bus width = 4 bit */
uint8_t pcfDev = 1; /**< I2C peripheral number to use */

/** @privatesection */

#define LCD_DELAY asm("nop\nnop\nnop\nnop\n") /**< Short delay macro */

#define LCD_BUSY_PCF_BIT 3 /**< Busy flag pin */
#define LCD_BL_PCF_BIT 7 /**< Backlight control pin */

#define LCD_RS_1 pcfBit(4, 1) /**< Set RS macro */
#define LCD_RS_0 pcfBit(4, 0) /**< Clear RS macro */

#define LCD_RW_1 pcfBit(5, 1) /**< Set RW macro */
#define LCD_RW_0 pcfBit(5, 0) /**< Clear RW macro */

#define LCD_E_1 pcfBit(6, 1) /**< Set E macro */
#define LCD_E_0 pcfBit(6, 0) /**< Clear E macro */

#define _BV(x) (1 << x) /**< Bit value macro */

// ------------------------------------------------------------------
// --- private procedures -------------------------------------------
// ------------------------------------------------------------------

static uint8_t pcfLast;
static uint8_t pcfErr = 1;
static uint8_t pcfAdr = 0x40;

void pcfWrite(uint8_t d)
{
	pcfErr = i2c_wr(pcfDev, pcfAdr , &d, 1);
	pcfLast = d;
}

uint8_t pcfRead(void)
{
	uint8_t r;
	pcfErr = i2c_rd(pcfDev, pcfAdr, &r, 1);
	return r;
}

void pcfBit(const uint8_t bit, const uint8_t on)
{
	if( on ) {
		pcfWrite(pcfLast | _BV(bit));
	} else {
		pcfWrite(pcfLast & ~_BV(bit));
	}
}

void lcd_out(const uint8_t data, const uint8_t rs)
{
	LCD_E_1;				// E high
	if( rs ) {LCD_RS_1;} else {LCD_RS_0;}	// select instruction(0) or data(1)
	LCD_RW_0;				// RW low (write)
	pcfWrite((pcfLast & 0xf0) | (data >> 4));		// set data pins
	LCD_DELAY;
	LCD_E_0;				// high to low on E to clock data
}

uint8_t lcd_in(const uint8_t rs)
{
	if( rs ) {LCD_RS_1;} else {LCD_RS_0;}	// select instruction/data
	LCD_RW_1;					// RW high (read)
	LCD_E_1;
	LCD_DELAY;
	uint8_t c = pcfRead() & 0x0f;
	LCD_E_0;

	return c;
}

uint8_t lcd_busy(void)
{
	pcfBit(LCD_BUSY_PCF_BIT, 1);
	return (lcd_in(0) & _BV(LCD_BUSY_PCF_BIT));
}

uint8_t lcd_available(void)	// returns 0(false) on timeout and 1-20(true) on lcd available
{
	uint8_t i = 10;			// wait max 10ms
	while ( lcd_busy() && i ) {
		i--;
	}
	return i;
}

uint8_t lcd_command(const uint8_t cmd)
{
	if( pcfErr ) return 0;

	if( lcd_available() ) {
		lcd_out(cmd, 0);
		lcd_out(cmd << 4, 0);
		return 1;
	}
	return 0;
}

uint8_t lcd_data(const uint8_t data)
{
	if( pcfErr ) return 0;

	if( lcd_available() ) {
		lcd_out(data, 1);
		lcd_out(data << 4, 1);
		return 1;
	}
	return 0;
}

void lcd_bl(uint8_t on)
{
#ifdef LCD_BL_PORT
	if( on ) {
		LCD_BL_PORT |= _BV(LCD_BL_BIT);
	} else {
		LCD_BL_PORT &= ~_BV(LCD_BL_BIT);
	}
#endif

	if( pcfAdr == 0x40 ) on = !on;
	pcfBit(LCD_BL_PCF_BIT, on);
}

void lcd_hwinit(void)
{
	i2c_init(pcfDev, I2C_100K);

	pcfAdr = 0x40;
	if( i2c_rd(pcfDev, pcfAdr, 0, 0) )  pcfAdr = 0x42;

	pcfWrite(0);
	lcd_bl(0);			// backlight off
	LCD_E_0;			// idle E is low
#ifdef LCD_BL_PORT
	// make LCD BL pin an output
	DDR(LCD_BL_PORT) |= _BV(LCD_BL_BIT);
	LCD_BL_PORT &= ~_BV(LCD_BL_BIT);
#endif
	pcfErr = 0;
}
