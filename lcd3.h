// ------------------------------------------------------------------
// --- lcd3.c                                                     ---
// --- library for controlling the HD44780 via PCF8574            ---
// ---                                    coded by Matej Kogovsek ---
// ------------------------------------------------------------------

#ifndef MAT_LCD3_H
#define MAT_LCD3_H

#include <inttypes.h>

#define lcd_puti_lz(par1, par2) lcd_puti_lc(par1, par2, '0')
#define lcd_puth_lz(par1, par2) lcd_puth_lc(par1, par2, '0')

// initialize lcd
void lcd_init(void);

// clear lcd
void lcd_clear(void);

// write a char to lcd
void lcd_putc(const char c);

// write a string from mem to lcd
uint8_t lcd_puts(const char* s);

// position lcd cursor to line l
void lcd_line(const uint8_t l);

// write a string of length n from mem to lcd
void lcd_putsn(const char* s, uint8_t n);

// write a string from progmem to lcd
//void lcd_puts_P(PGM_P s);

// write an integer to lcd
void lcd_puth(const uint32_t a);

// write an integer to lcd in hex, prepending with leading zeros
void lcd_puth_lc(const uint32_t a, uint8_t l, char c);

// write an integer to lcd
void lcd_puti(const uint32_t a);

// write an integer to lcd, prepending with leading characters
void lcd_puti_lc(const uint32_t a, uint8_t l, char c);

// write a float to the lcd with prec decimals
void lcd_putf(float f, uint8_t prec);

// lcd backlight
void lcd_bl(uint8_t on);

#endif
