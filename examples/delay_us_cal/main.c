#include "stm32f10x.h"

#include "mat/serialq.h"
#include "mat/misc.h"

//-----------------------------------------------------------------------------
//  Defines
//-----------------------------------------------------------------------------

#define SER_PORT 1
#define SER_BAUD 115200

//-----------------------------------------------------------------------------
//  Global variables
//-----------------------------------------------------------------------------

static uint8_t uart1rxbuf[64];
static uint8_t uart1txbuf[64];

volatile uint32_t msTicks;	// counts SysTicks

//-----------------------------------------------------------------------------
//  newlib required functions
//-----------------------------------------------------------------------------

void _exit(int status)
{
	//ser_printf("_exit called!\r\n");
	while(1) {}
}

//-----------------------------------------------------------------------------
//  SysTick handler
//-----------------------------------------------------------------------------

void SysTick_Handler(void)
{
	msTicks++;			// increment counter necessary in _delay_ms()
}

//-----------------------------------------------------------------------------
//  delays functions
//-----------------------------------------------------------------------------

void _delay_ms (uint32_t ms)
{
	uint32_t curTicks = msTicks;
	while ((msTicks - curTicks) < ms);
}

void _delay_us(uint32_t us)
{
	us *= 8;

	asm volatile(
		"mov r0, %[us]             \n\t"
		"1: subs r0, #1            \n\t"
		"nop                       \n\t" // 72MHz
		"nop                       \n\t" // 72MHz
		"bhi 1b                    \n\t"
		:
		: [us] "r" (us)
		: "r0"
	);
}

//-----------------------------------------------------------------------------
//  utility functions
//-----------------------------------------------------------------------------

uint32_t waitmschg(void)
{
	uint32_t x = msTicks;
	while( msTicks == x );
	return x + 1;
}

//-----------------------------------------------------------------------------
//  MAIN function
//-----------------------------------------------------------------------------

int main(void)
{
	if( SysTick_Config(SystemCoreClock / 1000) ) { // setup SysTick Timer for 1 msec interrupts
		while( 1 );                                  // capture error
	}

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); // disable preemption

	ser_init(SER_PORT, SER_BAUD, uart1txbuf, sizeof(uart1txbuf), uart1rxbuf, sizeof(uart1rxbuf));

	uint32_t us = 1000;
	while( 1 ) {
		uint32_t x = waitmschg(); // wait for ms counter change
		_delay_us(us);
		if( msTicks == x ) {
			++us;
		} else {
			--us;
		}
		ser_puti(1, us, 10);
		ser_puts(1, "\r\n");
		_delay_ms(20);
	}
}
