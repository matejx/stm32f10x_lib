/**
@file		fifo.c
@brief		Circular buffer routines. Interrupt safe.
@author		Matej Kogovsek
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
*/

#include "stm32f10x.h"
#include "fifo.h"
#include <string.h>

/**
@brief Initializes (clears) fifo.
@param[in]	f		Pointer to fifo_t struct where fifo state will be kept
@param[in]	p		Pointer to element array for data
@param[in]	s		sizeof(p)
@param[in]	e		sizeof(element)
*/
void fifo_clear(volatile struct fifo_t* f, void* const p, const uint16_t s, const uint16_t e)
{
	uint32_t g = __get_PRIMASK();
	__disable_irq();

	f->buf = p;
	f->size = s/e;
	f->elem = e;
	f->head = 0;
	f->tail = 0;
	f->len = 0;

	__set_PRIMASK(g);
}

/**
@brief Insert an element.
@param[in]	f		Pointer to fifo_t
@param[in]	d		Data to insert
@return True on success, false otherwise (buffer full).
*/
uint8_t fifo_put(volatile struct fifo_t* f, const void* d)
{
	uint32_t g = __get_PRIMASK();
	__disable_irq();

	if (f->len == f->size) {
		__set_PRIMASK(g);
		return 0;
	}

	memcpy(f->buf+(f->tail*f->elem), d, f->elem);
	f->tail++;
	if(f->tail == f->size) { f->tail = 0; }
	f->len++;

	__set_PRIMASK(g);
	return 1;
}

/**
@brief Get next element.
@param[in]	f		Pointer to fifo_t
@param[out]	d		Pointer to buffer where next element is put.
@return True on success (data copied to d), false otherwise (buffer empty).
*/
uint8_t fifo_get(volatile struct fifo_t* f, void* const d)
{
	uint32_t g = __get_PRIMASK();
	__disable_irq();

	if (f->len == 0) {
		__set_PRIMASK(g);
		return 0;
	}

	if( d ) {	// if d is null, cbuf_get can be used to check for data in buffer
		memcpy(d, f->buf+(f->head*f->elem), f->elem);
		f->head++;
		if(f->head == f->size) { f->head = 0; }
		f->len--;
	}

	__set_PRIMASK(g);
	return 1;
}
