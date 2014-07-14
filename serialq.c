// ------------------------------------------------------------------
// --- serialq.c                                                  ---
// --- routines for sending data over USART                       ---
// ---                         Matej Kogovsek (matej@hamradio.si) ---
// ------------------------------------------------------------------

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "stm32f10x.h"
#include "circbuf8.h"
#include "itoa.h"

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

struct USART_DevDef USART1_PinDef = {2, USART1, RCC_APB2Periph_USART1, USART1_IRQn, GPIOA, RCC_APB2Periph_GPIOA, GPIO_Pin_9};
struct USART_DevDef USART2_PinDef = {1, USART2, RCC_APB1Periph_USART2, USART2_IRQn, GPIOA, RCC_APB2Periph_GPIOA, GPIO_Pin_2};
struct USART_DevDef USART3_PinDef = {1, USART3, RCC_APB1Periph_USART3, USART3_IRQn, GPIOB, RCC_APB2Periph_GPIOB, GPIO_Pin_10};

struct USART_DevDef* usart_get_pdef(uint8_t USARTy)
{
	if( USARTy == 1 ) return &USART1_PinDef;
	if( USARTy == 2 ) return &USART2_PinDef;
	if( USARTy == 3 ) return &USART3_PinDef;
	
	return 0;
}

// VOLATILE QUEUES !!! VERY IMPORTANT !!!
volatile struct cbuf8_t uart_rxq[3];
volatile struct cbuf8_t uart_txq[3];

// ------------------------------------------------------------------
// initialize serial interface

void ser_init(const uint8_t n, const uint32_t br, uint8_t* txb, uint8_t txs, uint8_t* rxb, uint8_t rxs)
{
	cbuf8_clear(&uart_rxq[n-1], rxb, rxs);
	cbuf8_clear(&uart_txq[n-1], rxb, rxs);
	
	struct USART_DevDef* pdef = usart_get_pdef(n);

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

// ------------------------------------------------------------------
// shutdown serial interface

void ser_deinit(const uint8_t n)
{
	// TBI
}

// ------------------------------------------------------------------
// flush rx buffer

void ser_flush_rxbuf(const uint8_t n)
{
	cbuf8_clear(&uart_rxq[n-1], uart_rxq[n-1].buf, uart_rxq[n-1].size);
}

// ------------------------------------------------------------------
// get a byte from the serial queue

uint8_t ser_getc(const uint8_t n, uint8_t* const d)
{
	uint8_t r = cbuf8_get(&uart_rxq[n-1], d);
	return r;
}

// ------------------------------------------------------------------
// send a byte over the serial intf

void ser_putc(const uint8_t n, const char a)
{
	while( 1 ) {
		if( cbuf8_put(&uart_txq[n-1], a) ) { break; }
	}

	USART_ITConfig(usart_get_pdef(n)->usart, USART_IT_TXE, ENABLE); // enable data register empty interrupt
/*	
	struct USART_DevDef* pdef = usart_get_pdef(n);

	while( RESET == USART_GetFlagStatus(pdef->usart, USART_FLAG_TXE) ) {}
	
	USART_SendData(pdef->usart, a);*/
}

// ------------------------------------------------------------------
// send a string from memory

void ser_puts(const uint8_t n, const char* s)
{
	while( *s )	{
		ser_putc(n, *s);
		s++;
	}
}

void ser_putsn(const uint8_t n, const char* s, uint8_t l)
{
	while( l-- ) {
		if( *s ) {
			ser_putc(n, *s);
		} else {
			ser_putc(n, ' ');
		}
		s++;
	}
}

// ------------------------------------------------------------------
// send a number in the specified radix

void ser_puti_lc(const uint8_t n, const int32_t a, const uint8_t r, uint8_t w, char c)
{
	char s[16];
	
	itoa(a, s, r);

	while( w-- > strlen(s) ) { ser_putc(n, c); }

	ser_puts(n, s);
}

// ------------------------------------------------------------------
// send a float with precision

void ser_putf(const uint8_t n, float f, uint8_t prec)
{
	ser_puti_lc(n, f, 10, 0, '0');
	ser_putc(n, '.');
	while( prec-- ) {
		f = f - (int)f;
		f = f * 10; 
		ser_puti_lc(n, f, 10, 0, '0');
	}
}

// ------------------------------------------------------------------
// simplified implementation of printf
uint8_t ser_printf_n = 1;

int ser_printf(const char* s, ...)
{
	uint8_t n = ser_printf_n;
	va_list vl;
	va_start(vl, s);
	uint8_t esc = 0;
	
	while( *s ) {
		if( *s == '%' ) {
			if( esc ) {	ser_putc(n, '%');	}
			esc = !esc;
		} else
		if( esc ) {
			switch( *s ) {
				case 'd':
				case 'i':
					ser_puti_lc(n, va_arg(vl, int), 10, 0, '0');
					break;
				case 'x':
				case 'X':
					ser_puti_lc(n, va_arg(vl, int), 16, 0, '0');
					break;
				case 'f':
				case 'F':
					ser_putf(n, va_arg(vl, double), 3);
					break;
				case 'c':
					ser_putc(n, va_arg(vl, int));
					break;
				case 's':
					ser_puts(n, va_arg(vl, char*));
					break;
				default:
					ser_putc(n, '?');
					break;
			}
			esc = 0;
		} else {
			ser_putc(n, *s);
		}
		++s;
	}
	
	va_end(vl);
	return 1;
}

// ------------------------------------------------------------------
// INTERRUPTS
// ------------------------------------------------------------------

void ser_rxtx(const uint8_t n)
{
	struct USART_DevDef* pdef = usart_get_pdef(n);

	if( USART_GetITStatus(pdef->usart, USART_IT_RXNE) != RESET ) {
		// read one byte from the receive data register
		uint8_t d = USART_ReceiveData(pdef->usart);
		// put byte to queue
		cbuf8_put(&uart_rxq[n-1], d);
	}

	if( USART_GetITStatus(pdef->usart, USART_IT_TXE) != RESET ) {   
		uint8_t d;
		if( cbuf8_get(&uart_txq[n-1], &d) ) {
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
