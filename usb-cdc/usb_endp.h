#ifndef USB_ENDP_H
#define USB_ENDP_H

#include <inttypes.h>

void cdc_init(uint8_t* txb, uint16_t txs, uint8_t* rxb, uint16_t rxs);

void cdc_putc(const char a);
void cdc_puts(const char* s);
void cdc_putsn(const char* s, uint8_t n);
void cdc_puti_lc(const int32_t a, const uint8_t r, uint8_t w, char c);
void cdc_putf(float f, uint8_t prec);
#define cdc_puti(a, b) cdc_puti_lc(a, b, 0, 'x')
#define cdc_puti_lz(a, b, c) cdc_puti_lc(a, b, c, '0')

uint8_t cdc_getc(uint8_t* const d);

#endif
