#ifndef MAT_FIFO_H
#define MAT_FIFO_H

#include <inttypes.h>

/** Circbuf state struct */
struct fifo_t
{
	void* buf;	/**< pointer to data buffer */
	uint16_t head;	/**< current FIFO head */
	uint16_t tail;	/**< current FIFO tail */
	uint16_t len;	/**< number of elements currently in FIFO */
	uint16_t size;	/**< size of buf */
	uint16_t elem;	/**< single element size */
};

void fifo_clear(volatile struct fifo_t* f, void* const p, const uint16_t s, const uint16_t e);
uint8_t fifo_put(volatile struct fifo_t* f, const void* d);
uint8_t fifo_get(volatile struct fifo_t* f, void* const d);

#endif
