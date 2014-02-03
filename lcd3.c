// ------------------------------------------------------------------
// --- lcd3.c                                                     ---
// --- library for controlling the HD44780 via PCF8574            ---
// ---                                    coded by Matej Kogovsek ---
// ------------------------------------------------------------------

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "i2c.h"
#include "itoa.h"

#define LCD_I2C I2C1
#define LCD_DELAY
// asm("nop\nnop\nnop\nnop\n")
#define LCD_PCF_I2C_ADR 0x40
#define _BV(x) (1 << x)

extern void _delay_ms (uint32_t d);

uint8_t pcfLast;

void pcfWrite(const uint8_t d)
{
	i2c_wr(LCD_I2C, &d, 1, LCD_PCF_I2C_ADR);
	pcfLast = d;
}

uint8_t pcfRead(void)
{
	uint8_t d;
	return i2c_rd(LCD_I2C, &d, 1, LCD_PCF_I2C_ADR);
}

void pcfBit(const uint8_t bit, const uint8_t on)
{
	if( on ) {
		pcfWrite(pcfLast | _BV(bit));
	} else {
		pcfWrite(pcfLast & ~_BV(bit));
	}
}

#define LCD_BUSY_PCF_BIT 3
#define LCD_BL_PCF_BIT 7

#define LCD_RS_1 pcfBit(4, 1)
#define LCD_RS_0 pcfBit(4, 0)

#define LCD_RW_1 pcfBit(5, 1)
#define LCD_RW_0 pcfBit(5, 0)

#define LCD_E_1 pcfBit(6, 1)
#define LCD_E_0 pcfBit(6, 0)

#define LCD_BL_1 pcfBit(7, 1)
#define LCD_BL_0 pcfBit(7, 0)

void lcd_out(const uint8_t data, const uint8_t rs)
{
	LCD_E_1;				// E high
	if (rs) {LCD_RS_1;} else {LCD_RS_0;}	// select instruction(0) or data(1)
	LCD_RW_0;				// RW low (write)
	pcfWrite((pcfLast & 0xf0) | (data >> 4));		// set data pins
	LCD_DELAY;
	LCD_E_0;				// high to low on E to clock data
}

uint8_t lcd_in(const uint8_t rs)
{
	if (rs) {LCD_RS_1;} else {LCD_RS_0;}	// select instruction/data
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
	if (lcd_available()) {
		lcd_out(cmd, 0);
		lcd_out(cmd << 4, 0);
		return 1;
	}
	return 0;
}

uint8_t lcd_data(const uint8_t data)
{
	if (lcd_available()) {
		lcd_out(data, 1);
		lcd_out(data << 4, 1);
		return 1;
	}
	return 0;
}

// ------------------------------------------------------------------
// --- public procedures ------------------------------------------------------
// ------------------------------------------------------------------

// initialize lcd
void lcd_init(void)
{
	pcfWrite(_BV(LCD_BL_PCF_BIT));
	//LCD_BL_1;			// off is 1
	LCD_E_0;			// idle E is low

	// make LCD BL pin an output
	#ifdef LCD_BL_PORT
	DDR(LCD_BL_PORT) |= _BV(LCD_BL_BIT);
	LCD_BL_PORT &= ~_BV(LCD_BL_BIT);
	#endif

	lcd_out(0x30, 0);	// 8 bit interface
	_delay_ms(5);
	lcd_out(0x30, 0);
	_delay_ms(1);
	lcd_out(0x30, 0);
	lcd_out(0x20, 0);

	lcd_command(0x28);	// 4 bit interface, 2 lines, 5x7 dots
	lcd_command(0x08);
	lcd_command(0x01);	// clear display
	lcd_command(0x06);
	lcd_command(0x08 + 0x04); // display on
}

// clear lcd
void lcd_clear(void)
{
	lcd_command(1);
}

// write a char to lcd
void lcd_putc(const char c)
{
	lcd_data(c);
}

// write a string from mem to lcd
uint8_t lcd_puts(const char* s)
{
  uint8_t len = 0;
  
	while( s[len] )	{
		lcd_data(s[len]);
		len++;
	}
  
  return len;
}

// position lcd cursor to line l
void lcd_line(const uint8_t l)
{
  if( l == 2 ) {
    lcd_command(0x80 + 0x40);
  } else {
    lcd_command(0x80);
  }
}

// write a string of length n from mem to lcd
void lcd_putsn(const char* s, uint8_t n)
{
	while(n--) {
		if (*s) {
			lcd_data(*s);
		} else {
			lcd_data(' ');
		}
		s++;
	}
}

// write an integer to lcd
void lcd_puth(const uint32_t a)
{
	char s[10];
	
	itoa(a, s, 16);

	lcd_puts(s);
}

// write an integer to lcd
void lcd_puth_lc(const uint32_t a, uint8_t l, char c)
{
	char s[10];
	
	itoa(a, s, 16);

	while( l > strlen(s) ) {
		lcd_data(c);
		l--;
	}
	
	lcd_puts(s);
}

// write an integer to lcd
void lcd_puti(const uint32_t a)
{
	char s[10];
	
	itoa(a, s, 10);
	
	lcd_puts(s);
}

// write an integer to lcd, prepending with leading zeros until minlen
void lcd_puti_lc(const uint32_t a, uint8_t l, char c)
{
	char s[10];
	
	itoa(a, s, 10);
	
	while( l > strlen(s) ) {
		lcd_data(c);
		l--;
	}
	
	lcd_puts(s);
}

// write a float to the lcd with prec decimals
void lcd_putf(float f, uint8_t prec)
{
	lcd_puti(f);
	lcd_data('.');
	while( prec-- ) {
		f = f - (int)f;
		f = f * 10; 
		lcd_puti(f);
	}
}

// lcd backlight
void lcd_bl(uint8_t on)
{
	if( on ) {
		#ifdef LCD_BL_PORT
		LCD_BL_PORT |= _BV(LCD_BL_BIT);
		#endif
		LCD_BL_0;
	} else {
		#ifdef LCD_BL_PORT
		LCD_BL_PORT &= ~_BV(LCD_BL_BIT);
		#endif
		LCD_BL_1;
	}
}
