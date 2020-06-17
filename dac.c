/**
@file		dac.c
@brief		DAC routines
@author		Matej Kogovsek (matej@hamradio.si)
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
*/

#include <stm32f10x.h>
#include <stm32f10x_dac.h>

/**
@brief Set DAC value
@param[in]	n		DAC channel
@param[in]	v		Value
*/
void dac_set(uint8_t n, uint16_t v)
{
	if( n == 1 ) {
		DAC_SetChannel1Data(DAC_Align_12b_R, v);
	} else {
		DAC_SetChannel2Data(DAC_Align_12b_R, v);
	}
}

/**
@brief Init DAC.
@param[in]	n		DAC channel
*/
void dac_init(uint8_t n)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	DAC_InitTypeDef daci;
	DAC_StructInit(&daci);
	DAC_Init(n == 1 ? DAC_Channel_1 : DAC_Channel_2, &daci);
	DAC_Cmd(n == 1 ? DAC_Channel_1 : DAC_Channel_2, ENABLE);

	GPIO_InitTypeDef iotd;
	GPIO_StructInit(&iotd);
	iotd.GPIO_Pin = n == 1 ? GPIO_Pin_4 : GPIO_Pin_5;
	iotd.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &iotd);
}
