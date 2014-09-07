#ifndef MAT_CIRCBUF8_H
#define MAT_CIRCBUF8_H

#include <inttypes.h>

/** Circbuf state struct */
struct cbuf8_t
{
	uint8_t* buf;	/**< pointer to data buffer */
	uint16_t head;	/**< current FIFO head */
	uint16_t tail;	/**< current FIFO tail */
	uint16_t len;	/**< number of bytes currently in FIFO */
	uint16_t size;	/**< size of buf */
};

void cbuf8_clear(volatile struct cbuf8_t* cb, uint8_t* const p, const uint16_t s);
uint8_t cbuf8_put(volatile struct cbuf8_t* cb, const uint8_t d);
uint8_t cbuf8_get(volatile struct cbuf8_t* cb, uint8_t* const d);

#endif
