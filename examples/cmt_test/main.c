
#include "stm32f10x.h"

#include "mat/cmt.h"
#include "mat/serialq.h"

//-----------------------------------------------------------------------------
//  Global variables
//-----------------------------------------------------------------------------

volatile uint32_t msTicks;	// counts 1ms timeTicks

static uint8_t uart1rxbuf[64];
static uint8_t uart1txbuf[64];

//-----------------------------------------------------------------------------
// newlib required functions
//-----------------------------------------------------------------------------

void _exit(int status)
{
	while(1) {}
}

//-----------------------------------------------------------------------------
//  SysTick_Handler
//-----------------------------------------------------------------------------

void SysTick_Handler(void)
{
	msTicks++;			// increment counter necessary in Delay()
	cmt_tick();
}

//-----------------------------------------------------------------------------
//  delay functions
//-----------------------------------------------------------------------------

void _delay_ms (uint32_t ms)
{
	uint32_t curTicks = msTicks;
	while ((msTicks - curTicks) < ms);
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
//  MAIN function
//-----------------------------------------------------------------------------

void task1(void)
{
	while( 1 ) {
		cmt_delay_ticks(300);
		GPIO_WriteBit(GPIOC, GPIO_Pin_9, Bit_SET);
		cmt_delay_ticks(300);
		GPIO_WriteBit(GPIOC, GPIO_Pin_9, Bit_RESET);
	}
}

void task2(void)
{
	while( 1 ) {
		ser_puts(1, "hello\r\n");
		cmt_delay_ticks(1000);
	}
}

int main(void)	// = task0
{
	if( SysTick_Config(SystemCoreClock / 1000) ) { // setup SysTick Timer for 1 msec interrupts
		while( 1 );                                  // capture error
	}

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0); // disable preemption
	ser_init(1, 38400, uart1txbuf, sizeof(uart1txbuf), uart1rxbuf, sizeof(uart1rxbuf));

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	// Configure LED1, LED2 output pins
	GPIO_InitTypeDef iotd;
	iotd.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_8;
	iotd.GPIO_Speed = GPIO_Speed_2MHz;
	iotd.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &iotd);

	cmt_setup_task(task1, __get_MSP()-1024);
	cmt_setup_task(task2, __get_MSP()-2048);

	while( 1 ) {
		cmt_delay_ticks(500);
		GPIO_WriteBit(GPIOC, GPIO_Pin_8, Bit_RESET);
		cmt_delay_ticks(500);
		GPIO_WriteBit(GPIOC, GPIO_Pin_8, Bit_SET);
	}
}

//-----------------------------------------------------------------------------
//  INTERRUPTS
//-----------------------------------------------------------------------------
