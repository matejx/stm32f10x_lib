#ifndef MAT_LCD_H
#define MAT_LCD_H

#include <inttypes.h>

#define lcd_puti_lz(par1,par2) lcd_puti_lc(par1, 10, par2, '0')
#define lcd_puti(par1) lcd_puti_lc(par1, 10, 0, 0)
#define lcd_puth(par1) lcd_puti_lc(par1, 16, 0, 0)

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

// fill rest of the line with spaces
void lcd_endl(void);

// write a string of length n from mem to lcd
void lcd_putsn(const char* s, uint8_t n);

// write an integer to lcd, prepending with leading character
void lcd_puti_lc(const uint32_t a, uint8_t r, uint8_t l, char c);

// write a float to the lcd with prec decimals
void lcd_putf(float f, uint8_t prec);

// lcd backlight
void lcd_bl(uint8_t on);

#endif
