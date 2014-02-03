// ------------------------------------------------------------------
// --- circbuf.c - circular buffer routines                       ---
// ---                                                            ---
// ---             7.nov.2010, Matej Kogovsek (matej@hamradio.si) ---
// ------------------------------------------------------------------

#include "stm32f10x.h"
#include "circbuf8.h"

// ------------------------------------------------------------------
// initializes (clears) circ buf

void cbuf8_clear(volatile struct cbuf8_t* cb, uint8_t* const p, const uint16_t s)
{
	uint32_t g = __get_PRIMASK();
	__disable_irq();

	cb->buf = p;
	cb->size = s;
	cb->head = 0;
	cb->tail = 0;
	cb->len = 0;
	
	__set_PRIMASK(g);
}

// ------------------------------------------------------------------
// inserts an element

uint8_t cbuf8_put(volatile struct cbuf8_t* cb, const uint8_t d)
{
	uint32_t g = __get_PRIMASK();
	__disable_irq();
	
	if (cb->len == cb->size) {
		__set_PRIMASK(g);
		return 0; 
	}

	cb->buf[cb->tail] = d;
	cb->tail++;
	if(cb->tail == cb->size) { cb->tail = 0; }
	cb->len++;
	
	__set_PRIMASK(g);
	return 1;
}

// ------------------------------------------------------------------
// gets the next element

uint8_t cbuf8_get(volatile struct cbuf8_t* cb, uint8_t* const d)
{
	uint32_t g = __get_PRIMASK();
	__disable_irq();

	if (cb->len == 0) {
		__set_PRIMASK(g);
		return 0;
	}

	if( d ) {	// if d is null, cbuf_get can be used to check for data in buffer
		*d = cb->buf[cb->head];
		cb->head++;
		if(cb->head == cb->size) { cb->head = 0; }
		cb->len--;
	}

	__set_PRIMASK(g);
	return 1;
}

// ------------------------------------------------------------------
