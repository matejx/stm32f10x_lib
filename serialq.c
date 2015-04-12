/**

All USART data transmission is interrupt driven. Received data is put into a FIFO (provided by circbuf8.c).
Data to be transmitted is likewise put into a FIFO. Memory for both FIFOs is provided by the caller on init.

@file		serialq.c
@brief		Buffered USART routines
@author		Matej Kogovsek (matej@hamradio.si)
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
*/

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "stm32f10x.h"
#include "circbuf8.h"
#include "itoa.h"

/** @privatesection */

struct USART_DevDef
{
	uint8_t usart_apb;
	USART_TypeDef* usart;
	uint32_t usart_clk;
	uint8_t usart_irqn;
	GPIO_TypeDef* gpio;
	uint32_t gpio_clk;
	uint16_t pin_tx;
};

/** Register and pin defs for USART1 */
struct USART_DevDef USART1_PinDef = {2, USART1, RCC_APB2Periph_USART1, USART1_IRQn, GPIOA, RCC_APB2Periph_GPIOA, GPIO_Pin_9};
/** Register and pin defs for USART2 */
struct USART_DevDef USART2_PinDef = {1, USART2, RCC_APB1Periph_USART2, USART2_IRQn, GPIOA, RCC_APB2Periph_GPIOA, GPIO_Pin_2};
/** Register and pin defs for USART3 */
struct USART_DevDef USART3_PinDef = {1, USART3, RCC_APB1Periph_USART3, USART3_IRQn, GPIOB, RCC_APB2Periph_GPIOB, GPIO_Pin_10};

struct USART_DevDef* usart_get_pdef(uint8_t devnum)
{
	if( devnum == 1 ) return &USART1_PinDef;
	if( devnum == 2 ) return &USART2_PinDef;
	if( devnum == 3 ) return &USART3_PinDef;

	return 0;
}

// VOLATILE QUEUES !!! VERY IMPORTANT !!!
/** RX circbuf defs for all 3 USARTs */
volatile struct cbuf8_t uart_rxq[3];
/** TX circbuf defs for all 3 USARTs */
volatile struct cbuf8_t uart_txq[3];

/** @publicsection */

/**
@brief Init USART.
@param[in]	devnum		USART peripheral number (1..3)
@param[in]	br			Baudrate (i.e. 115200 for 115.2k), F_CPU independent
@param[in]	txb			Pointer to caller allocated TX buffer
@param[in]	txs			sizeof(txb)
@param[in]	rxb			Pointer to caller allocated RX buffer
@param[in]	rxs			sizeof(rxs)
*/
void ser_init(const uint8_t devnum, const uint32_t br, uint8_t* txb, uint8_t txs, uint8_t* rxb, uint8_t rxs)
{
	cbuf8_clear(&uart_rxq[devnum-1], rxb, rxs);
	cbuf8_clear(&uart_txq[devnum-1], rxb, rxs);

	struct USART_DevDef* pdef = usart_get_pdef(devnum);

	// clock config
	if( pdef->usart_apb == 1 ) {
		RCC_APB1PeriphClockCmd(pdef->usart_clk, ENABLE);
	} else {
		RCC_APB2PeriphClockCmd(pdef->usart_clk, ENABLE);
	}
	RCC_APB2PeriphClockCmd(pdef->gpio_clk, ENABLE);

	// GPIO config
	GPIO_InitTypeDef iotd;
	iotd.GPIO_Pin = pdef->pin_tx;
	iotd.GPIO_Speed = GPIO_Speed_50MHz;
	iotd.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(pdef->gpio, &iotd);

	// USART config
	USART_InitTypeDef uatd;
	uatd.USART_BaudRate = br;
	uatd.USART_WordLength = USART_WordLength_8b;
	uatd.USART_StopBits = USART_StopBits_1;
	uatd.USART_Parity = USART_Parity_No;
	uatd.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	uatd.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(pdef->usart, &uatd);
	USART_ITConfig(pdef->usart, USART_IT_RXNE, ENABLE);
	// USART_ITConfig(pdef->usart, USART_IT_TXE, ENABLE);
	USART_Cmd(pdef->usart, ENABLE);

	// USART interrupt setup
	NVIC_InitTypeDef ictd;
	ictd.NVIC_IRQChannel = pdef->usart_irqn;
	ictd.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&ictd);
}

/**
@brief Deinit USART.
@param[in]	devnum		USART peripheral number (1..3)
*/
void ser_shutdown(const uint8_t devnum)
{
	// TBI
}

/**
@brief Flush rx buffer.
@param[in]	devnum		USART peripheral number (1..3)
*/
void ser_flush_rxbuf(const uint8_t devnum)
{
	cbuf8_clear(&uart_rxq[devnum-1], uart_rxq[devnum-1].buf, uart_rxq[devnum-1].size);
}

/**
@brief Wait for output queue to be transmitted
@param[in]	devnum		USART peripheral number (1..3)
*/
void ser_wait_txe(const uint8_t devnum)
{
	struct USART_DevDef* pdef = usart_get_pdef(devnum);

	while( USART_GetFlagStatus(pdef->usart, USART_FLAG_TC) == RESET );
}

/**
@brief Get a byte from the serial queue.
@param[in]	devnum		USART peripheral number (1..3)
@param[out]	d			Pointer to uint8_t where received data is put
@return Same as cbuf8_get
*/
uint8_t ser_getc(const uint8_t devnum, uint8_t* const d)
{
	uint8_t r = cbuf8_get(&uart_rxq[devnum-1], d);
	return r;
}

/**
@brief Enqueue a byte to the serial queue for transmission.
@param[in]	devnum		USART peripheral number (1..3)
@param[in]	a			Byte to transmit
*/
void ser_putc(const uint8_t devnum, const char a)
{
	while( 1 ) {
		if( cbuf8_put(&uart_txq[devnum-1], a) ) { break; }
	}

	USART_ITConfig(usart_get_pdef(devnum)->usart, USART_IT_TXE, ENABLE); // enable data register empty interrupt
/*
	struct USART_DevDef* pdef = usart_get_pdef(n);

	while( RESET == USART_GetFlagStatus(pdef->usart, USART_FLAG_TXE) ) {}

	USART_SendData(pdef->usart, a);
*/
}

/**
@brief Send a string.
@param[in]	devnum		USART peripheral number (1..3)
@param[in]	s			Zero terminated string to send
*/
void ser_puts(const uint8_t devnum, const char* s)
{
	while( *s )	{
		ser_putc(devnum, *s);
		s++;
	}
}

/**
@brief Send n chars of string.
@param[in]	devnum		USART peripheral number (1..3)
@param[in]	s			String, zeros are sent as spaces
@param[in]	n			Number of chars to send
*/
void ser_putsn(const uint8_t devnum, const char* s, uint8_t n)
{
	while( n-- ) {
		if( *s ) {
			ser_putc(devnum, *s);
		} else {
			ser_putc(devnum, ' ');
		}
		s++;
	}
}

/**
@brief Send int in the specified radix r of minlen w prepended by char c.
@param[in]	devnum		USART peripheral number (1..3)
@param[in]	a			int
@param[in]	r			Radix
@param[in]	w			Min width
@param[in]	c			Prepending char to achieve min width
*/
void ser_puti_lc(const uint8_t devnum, const int32_t a, const uint8_t r, uint8_t w, char c)
{
	char s[16];

	itoa(a, s, r);

	while( w-- > strlen(s) ) { ser_putc(devnum, c); }

	ser_puts(devnum, s);
}

/**
@brief Send float with specified precision.
@param[in]	devnum		USART peripheral number (1..3)
@param[in]	f			float
@param[in]	prec		Number of decimals
*/
void ser_putf(const uint8_t devnum, float f, uint8_t prec)
{
	ser_puti_lc(devnum, f, 10, 0, '0');
	ser_putc(devnum, '.');
	while( prec-- ) {
		f = f - (int)f;
		f = f * 10;
		ser_puti_lc(devnum, f, 10, 0, '0');
	}
}

uint8_t ser_printf_devnum = 1; /**< USART peripheral number used by ser_printf */

/**
@brief Simplified implementation of printf (without dynamic malloc).
@return Always returns 1.

Variable ser_printf_devnum has to be set to the wanted USART peripheral number (1..3) prior to calling.
*/
int ser_printf(const char* s, ...)
{
	uint8_t devnum = ser_printf_devnum;
	va_list vl;
	va_start(vl, s);
	uint8_t esc = 0;

	while( *s ) {
		if( *s == '%' ) {
			if( esc ) {	ser_putc(devnum, '%');	}
			esc = !esc;
		} else
		if( esc ) {
			switch( *s ) {
				case 'd':
				case 'i':
					ser_puti_lc(devnum, va_arg(vl, int), 10, 0, '0');
					break;
				case 'x':
				case 'X':
					ser_puti_lc(devnum, va_arg(vl, int), 16, 0, '0');
					break;
				case 'f':
				case 'F':
					ser_putf(devnum, va_arg(vl, double), 3);
					break;
				case 'c':
					ser_putc(devnum, va_arg(vl, int));
					break;
				case 's':
					ser_puts(devnum, va_arg(vl, char*));
					break;
				default:
					ser_putc(devnum, '?');
					break;
			}
			esc = 0;
		} else {
			ser_putc(devnum, *s);
		}
		++s;
	}

	va_end(vl);
	return 1;
}

/** @privatesection */

void ser_rxtx(const uint8_t devnum)
{
	struct USART_DevDef* pdef = usart_get_pdef(devnum);

	if( USART_GetITStatus(pdef->usart, USART_IT_RXNE) != RESET ) {
		// read one byte from the receive data register
		uint8_t d = USART_ReceiveData(pdef->usart);
		// put byte to queue
		cbuf8_put(&uart_rxq[devnum-1], d);
	}

	if( USART_GetITStatus(pdef->usart, USART_IT_TXE) != RESET ) {
		uint8_t d;
		if( cbuf8_get(&uart_txq[devnum-1], &d) ) {
			// send next byte from buffer
			USART_SendData(pdef->usart, d);
		} else {
			// no more data to send, disable UDR empty int
			USART_ITConfig(pdef->usart, USART_IT_TXE, DISABLE);
		}
	}
}

void USART1_IRQHandler(void)
{
	ser_rxtx(1);
}

void USART2_IRQHandler(void)
{
	ser_rxtx(2);
}

void USART3_IRQHandler(void)
{
	ser_rxtx(3);
}
