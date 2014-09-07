/**
@file		lcd.c
@brief		HD44780 LCD routines (high level)
@author		Matej Kogovsek (matej@hamradio.si)
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
*/

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "itoa.h"

/** @brief LCD width */
#define LCD_WIDTH 20
/** @brief LCD height (number of lines) */
#define LCD_HEIGHT 2
/** @brief Define to use framebuffer. Skips unnecessary LCD writes. Recommended. */
#define LCD_USE_FB

// ------------------------------------------------------------------
// --- procedures implemented by lcd_[intf].c -----------------------
// ------------------------------------------------------------------

/** @privatesection */

extern uint8_t lcd_busw; /**< Extern variable defined by "driver" specifying used LCD bus width (0 = 4 bit, 0x10 = 8 bit) */
void lcd_hwinit(void);
void lcd_out(const uint8_t data, const uint8_t rs);
uint8_t lcd_command(const uint8_t cmd);
uint8_t lcd_data(const uint8_t data);
void _delay_ms(uint32_t d);


#ifdef LCD_USE_FB
static char lcd_fb[LCD_HEIGHT*(LCD_WIDTH+1)]; /**< frame buffer */
static uint8_t lcd_fbc; /**< frame buffer cursor */
#endif

static uint8_t lcd_cur; /**< LCD cursor */

/** @publicsection */

/**
@brief Clear LCD.
*/
void lcd_clear(void)
{
	lcd_command(1);
	lcd_cur = 0;

#ifdef LCD_USE_FB
	memset(lcd_fb, ' ', sizeof(lcd_fb));
	lcd_fbc = lcd_cur;
#endif
}

/**
@brief Init LCD.
*/
void lcd_init(void)
{
	lcd_hwinit();
#ifndef LCD_SIMPLE_INIT
	lcd_out(0x30, 0);	// 8 bit interface
	_delay_ms(5);
	lcd_out(0x30, 0);
	_delay_ms(1);
	lcd_out(0x30, 0);
	_delay_ms(1);
	lcd_out(0x20 | lcd_busw, 0);
#endif
	lcd_command(0x28 | lcd_busw);	// 2 lines, 5x7 dots
	lcd_command(0x08);  // display off, cursor off, blink off
	lcd_clear();
	lcd_command(0x06);  // cursor increment
	lcd_command(0x08 | 0x04); // display on
}

/**
@brief Position LCD cursor.
@param[in]	x	character (0 based)
@param[in]	y	line (0 based)
*/
void lcd_goto(uint8_t x, uint8_t y)
{
#ifdef LCD_USE_FB
	lcd_fbc = (0x40 * y) + x;
#else
	lcd_cur = (0x40 * y) + x;
	lcd_command(0x80 + lcd_cur);
#endif

}

/**
@brief Position LCD cursor to the beginning of line y.
@param[in]	y	line (1 based)
*/
void lcd_line(const uint8_t y)
{
	lcd_goto(0, y-1);
}

/**
@brief Write a char.
@param[in]	c	character to write
*/
void lcd_putc(const char c)
{
#ifdef LCD_USE_FB
	if( (lcd_fbc & 0x3f) >= LCD_WIDTH )
		return;

	uint8_t i = (lcd_fbc & 0x3f) + (lcd_fbc & 0x40 ? LCD_WIDTH+1 : 0);

	if( c != lcd_fb[i] ) {

		if( lcd_fbc != lcd_cur ) {
			lcd_command(0x80 + lcd_fbc);
			lcd_cur = lcd_fbc;
		}

		lcd_data(c);
		++lcd_cur;
		lcd_fb[i] = c;
	}
	++lcd_fbc;
#else
	if( (lcd_cur & 0x3f) >= LCD_WIDTH )
		return;

	lcd_data(c);
	++lcd_cur;
#endif
}

/**
@brief Emulate endl by printing spaces until LCD width is reached.
*/
void lcd_endl(void)
{
	#ifdef LCD_USE_FB
	uint8_t* cur = &lcd_fbc;
	#else
	uint8_t* cur = &lcd_cur;
	#endif

	while( (0x3f & *cur) < LCD_WIDTH ) {
		lcd_putc(' ');
	}

	if( 0 == (0x40 & *cur) ) {
		lcd_goto(0, 1);
	}
}

/**
@brief Write a string.
@param[in]	s	Zero terminated string
@return Number of chars written.
*/
uint8_t lcd_puts(const char* s)
{
  uint8_t len = 0;

	while( s[len] )	{
		lcd_putc(s[len]);
		len++;
	}

  return len;
}

/**
@brief Write n chars of string.
@param[in]	s	String (zeros are printed as spaces)
@param[in]	n	Number of characters to write
*/
void lcd_putsn(const char* s, uint8_t n)
{
	while(n--) {
		if (*s) {
			lcd_putc(*s);
		} else {
			lcd_putc(' ');
		}
		s++;
	}
}

/**
@brief Write int in the specified radix r of minlen w prepended by char c.
@param[in]	a	int
@param[in]	r	Radix
@param[in]	l	Min length
@param[in]	c	Prepending char
*/
void lcd_puti_lc(const uint32_t a, uint8_t r, uint8_t l, char c)
{
	char s[10];

	itoa(a, s, r);

	while( l > strlen(s) ) {
		lcd_putc(c);
		l--;
	}

	lcd_puts(s);
}

/**
@brief Write float with specified precision.
@param[in]	f		float
@param[in]	prec	Number of decimals
*/
void lcd_putf(float f, uint8_t prec)
{
	lcd_puti_lc(f, 10, 0, 0);
	lcd_putc('.');
	while( prec-- ) {
		f = f - (int)f;
		f = f * 10;
		lcd_puti_lc(f, 10, 0, 0);
	}
}
