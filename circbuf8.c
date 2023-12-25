/**
@file		circbuf8.c
@brief		Circular byte buffer routines. Interrupt safe.
@author		Matej Kogovsek
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
*/

#include "stm32f10x.h"
#include "circbuf8.h"

/**
@brief Initializes (clears) circbuf.
@param[in]	cb		Pointer to cbuf_t struct where circbuf state will be kept
@param[in]	p		Pointer to byte array for data
@param[in]	s		sizeof(p)
*/
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

/**
@brief Insert an element.
@param[in]	cb		Pointer to cbuf_t
@param[in]	d		Data to insert
@return True on success, false otherwise (buffer full).
*/
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

/**
@brief Get next element.
@param[in]	cb		Pointer to cbuf_t
@param[out]	d		Pointer to uint8_t where next element is put.
@return True on success (data copied to d), false otherwise (buffer empty).
*/
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
