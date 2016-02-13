/**
@file		adc.c
@brief		ADC routines
@author		Matej Kogovsek (matej@hamradio.si)
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
*/

#include <stm32f10x.h>
#include <stm32f10x_adc.h>

#define ADC_NCH 18

static volatile uint16_t adc_res[ADC_NCH]; /**< Buffer of averaged results for all possible channels */
static volatile uint8_t adc_curch; /**< Channel currently converting */
static uint32_t adc_ench; /**< Enabled channel */
static uint8_t adc_freerun = 0; /**< Freerun or not (bool) */
static uint8_t adc_navg = 16; /**< how many ADC samples to average */

/**
@brief Init ADC.
@param[in]	ench		Bitmask of enabled channels (bits 0 to 17)
@param[in]	navg		Number of samples to average (keep this a power of two, i.e. 1,2,4,8,16,...)
*/
void adc_init(uint32_t ench, uint8_t navg)
{
	adc_ench = ench;
	adc_curch = 255;	// set to invalid value
	adc_navg = navg;
	if( adc_navg == 0 ) adc_navg = 1;

	// PCLK2 is the APB2 clock, ADCCLK = PCLK2/6 = 72/6 = 12MHz
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);

	// Enable ADC1 clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	// ADC1 Configuration
	ADC_InitTypeDef  adis;
	adis.ADC_Mode = ADC_Mode_Independent;
	adis.ADC_ScanConvMode = DISABLE;
	adis.ADC_ContinuousConvMode = DISABLE;
	adis.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	adis.ADC_DataAlign = ADC_DataAlign_Right;
	adis.ADC_NbrOfChannel = 1;

	ADC_Init(ADC1, &adis);
	ADC_Cmd(ADC1, ENABLE);

	ADC_ResetCalibration(ADC1);
	while( ADC_GetResetCalibrationStatus(ADC1) );
	ADC_StartCalibration(ADC1);
	while( ADC_GetCalibrationStatus(ADC1) );

	ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);

	// ADC interrupt setup
	NVIC_InitTypeDef ictd;
#ifdef ADC1_IRQn
	ictd.NVIC_IRQChannel = ADC1_IRQn;
#else
	ictd.NVIC_IRQChannel = ADC1_2_IRQn;
#endif
	ictd.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&ictd);
}

/**
@brief Start next conversion.

Use if you want to control when conversions are started. Do not call if free running.
*/
void adc_startnext(void)
{
	//if( ADC_GetSoftwareStartConvStatus(ADC1) == SET ) return;

	if( adc_ench ) { // sanity check
		do {
			adc_curch++;
			if( adc_curch >= ADC_NCH ) adc_curch = 0;
		} while( (adc_ench & (1 << adc_curch)) == 0 );

		ADC_RegularChannelConfig(ADC1, adc_curch, 1, ADC_SampleTime_13Cycles5);
		ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	}
}

/**
@brief Start free running ADC conversions.

After a conversion is finished, a new conversion is automatically started.
*/
void adc_startfree(void)
{
	adc_freerun = 1;
	adc_startnext();
}

/**
@brief Stop free running ADC conversions.

*/
void adc_stopfree(void)
{
	adc_freerun = 0;
}

/**
@brief Get a channel's averaged ADC value.
@param[in]	ch		Channel to get
@return Averaged ADC value for channel
*/
uint16_t adc_get(const uint8_t ch)
{
	if( ch >= ADC_NCH ) return 0;

	uint32_t g = __get_PRIMASK();
	__disable_irq();

	uint16_t r = adc_res[ch];

	__set_PRIMASK(g);
	return r;
}

/** @privatesection */

void ADC1_2_IRQHandler(void)
{
	static uint16_t sum = 0;
	static uint8_t samp = 0;

	sum += ADC_GetConversionValue(ADC1); // add new conversion result to sum
	samp++;						    // increase sample counter

	if( samp == adc_navg ) {
		sum /= adc_navg;	    // calc average
		adc_res[adc_curch] = sum;	// store averaged conversion result
		samp = 0;				    // reset variables
		sum = 0;

		if( adc_freerun ) adc_startnext();
	} else {
		ADC_SoftwareStartConvCmd(ADC1, ENABLE);
	}
}
