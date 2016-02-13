//-----------------------------------------------------------------------------
// seradc v1.0
//
//                               30.jan.2014, Matej Kogovsek, matej@hamradio.si
//-----------------------------------------------------------------------------

#include "stm32f10x.h"

#include "mat/serialq.h"
#include "mat/adc.h"

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

	static uint8_t adc_ticks = 0;
	if( ++adc_ticks >= 20 ) {
		adc_ticks = 0;
		adc_startnext();
	}
}

//-----------------------------------------------------------------------------
//  delay functions
//-----------------------------------------------------------------------------

void _delay_ms (uint32_t dlyTicks)
{
	uint32_t curTicks = msTicks;
	while ((msTicks - curTicks) < dlyTicks);
}

void _delay_us(uint32_t us)
{
    us *= 8;

    asm volatile("mov r0, %[us]             \n\t"
                 "1: subs r0, #1            \n\t"
                 "bhi 1b                    \n\t"
                 :
                 : [us] "r" (us)
                 : "r0"
	);
}

//-----------------------------------------------------------------------------
//  utility functions
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  MAIN function
//-----------------------------------------------------------------------------

int main(void)
{
	if( SysTick_Config(SystemCoreClock / 1000) ) { // setup SysTick Timer for 1 msec interrupts
		while( 1 );                                  // capture error
	}

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); // disable preemption

	ser_init(1, 115200, uart1txbuf, sizeof(uart1txbuf), uart1rxbuf, sizeof(uart1rxbuf));
	adc_init(0x0f, 8);	// enable 4 AD channels (0..3), average 8 samples
	adc_startfree();

	while( 1 ) {
		_delay_ms(200);

		for( uint8_t i = 0; i < 3; ++i ) {
			ser_puti(1, i, 10);
			ser_puts(1, " : ");
			ser_puti(1, adc_get(i), 10);
			ser_puts(1, "\r\n");
		}
		ser_puts(1, "---\r\n");
	}
}
