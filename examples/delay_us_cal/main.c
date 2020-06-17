#include "stm32f10x.h"

#include "mat/serialq.h"

//-----------------------------------------------------------------------------
//  Defines
//-----------------------------------------------------------------------------

#define SER_PORT 1
#define SER_BAUD 115200

#define LED_PORT GPIOC
#define LED_PIN GPIO_Pin_13

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

void DDR(GPIO_TypeDef* port, uint16_t pin, GPIOMode_TypeDef mode)
{
	if( port == GPIOA ) { RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); }
	if( port == GPIOB ) { RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); }
	if( port == GPIOC ) { RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); }
	if( port == GPIOD ) { RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE); }
	if( port == GPIOE ) { RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE); }
	if( port == GPIOF ) { RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE); }
	if( port == GPIOG ) { RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE); }

	GPIO_InitTypeDef iotd;
	iotd.GPIO_Pin = pin;
	iotd.GPIO_Speed = GPIO_Speed_50MHz;
	iotd.GPIO_Mode = mode;
	GPIO_Init(port, &iotd);
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
/*
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
*/
	DDR(LED_PORT, LED_PIN, GPIO_Mode_Out_PP);

	while( 1 ) {
		LED_PORT->BSRR = LED_PIN;
		_delay_us(1000);
		LED_PORT->BRR = LED_PIN;
		_delay_us(1000);
	}

}
