#include "stm32f10x.h"

uint8_t misc_gpio_num(GPIO_TypeDef* port)
{
	static GPIO_TypeDef* ports[] = {GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG};

	uint8_t i;
	for( i = 0; i < 7; ++i ) {
		if( port == ports[i] ) break;
	}
	return i;
}

void misc_gpio_config(GPIO_TypeDef* port, uint16_t pin, GPIOMode_TypeDef mode)
{
	static uint32_t abp2gpio[] = {RCC_APB2Periph_GPIOA, RCC_APB2Periph_GPIOB,
		RCC_APB2Periph_GPIOC, RCC_APB2Periph_GPIOD, RCC_APB2Periph_GPIOE,
		RCC_APB2Periph_GPIOF, RCC_APB2Periph_GPIOG};

	RCC_APB2PeriphClockCmd(abp2gpio[misc_gpio_num(port)], ENABLE);

	GPIO_InitTypeDef iotd;
	iotd.GPIO_Pin = pin;
	iotd.GPIO_Speed = GPIO_Speed_2MHz;
	iotd.GPIO_Mode = mode;
	GPIO_Init(port, &iotd);
}

void misc_exti_setup(GPIO_TypeDef* port, uint16_t pin, EXTITrigger_TypeDef trg)
{
	static uint8_t irqns[16] = {EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn, EXTI4_IRQn,
		EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn,
		EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn};

	uint8_t i;
	for( i = 0; i < 16; ++i ) {
		if( pin & (1 << i) ) break;
	}
	uint8_t pinsrc = i;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	GPIO_EXTILineConfig(misc_gpio_num(port), pinsrc);

	EXTI_InitTypeDef exi;
	exi.EXTI_Line = pin; // line numbers equal pin numbers
	exi.EXTI_Mode = EXTI_Mode_Interrupt;
	exi.EXTI_Trigger = trg;
	exi.EXTI_LineCmd = ENABLE;
	EXTI_Init(&exi);

	NVIC_InitTypeDef nvi;
	nvi.NVIC_IRQChannel = irqns[pinsrc];
	nvi.NVIC_IRQChannelPreemptionPriority = 0x0f;
	nvi.NVIC_IRQChannelSubPriority = 0x0f;
	nvi.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvi);
}

//-----------------------------------------------------------------------------
//  Interrupts
//-----------------------------------------------------------------------------

#ifdef MISC_EXTI_INT

extern void exti_int(uint8_t i);

void EXTI0_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line0);
	exti_int(0);
}

void EXTI1_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line1);
	exti_int(1);
}

void EXTI2_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line2);
	exti_int(2);
}

void EXTI3_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line3);
	exti_int(3);
}

void EXTI4_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line4);
	exti_int(4);
}

void EXTI9_5_IRQHandler(void)
{
	uint16_t i;
	for( i = 5; i < 10; ++i ) {
		if( EXTI_GetITStatus(1 << i) ) {
			EXTI_ClearITPendingBit(i << i);
			exti_int(i);
		}
	}
}

void EXTI15_10_IRQHandler(void)
{
	uint16_t i;
	for( i = 10; i < 16; ++i ) {
		if( EXTI_GetITStatus(1 << i) ) {
			EXTI_ClearITPendingBit(i << i);
			exti_int(i);
		}
	}
}
#endif
