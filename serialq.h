#ifndef MAT_SERIALQ_H
#define MAT_SERIALQ_H

#include <inttypes.h>

void ser_init(const uint8_t devnum, const uint32_t br, uint8_t* txb, uint16_t txs, uint8_t* rxb, uint16_t rxs);
void ser_shutdown(const uint8_t devnum);

void ser_flush_rxbuf(const uint8_t devnum);
void ser_wait_txe(const uint8_t devnum);
uint8_t ser_getc(const uint8_t devnum, uint8_t* const d);

void ser_putc(const uint8_t devnum, const char a);
void ser_puts(const uint8_t devnum, const char* s);
void ser_putsn(const uint8_t devnum, const char* s, uint16_t n);
void ser_puti_lc(const uint8_t devnum, const int32_t a, const uint8_t r, uint8_t w, char c);
void ser_putf(const uint8_t devnum, float f, uint8_t prec);
#define ser_puti(a, b, c) ser_puti_lc(a, b, c, 0, 'x')
#define ser_puti_lz(a, b, c, d) ser_puti_lc(a, b, c, d, '0')

extern uint8_t ser_printf_devnum;
int ser_printf(const char* s, ...);

#endif
