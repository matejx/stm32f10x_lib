/**
@file		adc.c
@brief		ADC routines
@author		Matej Kogovsek (matej@hamradio.si)
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
*/

#include <stm32f10x.h>
#include <stm32f10x_adc.h>

static volatile uint16_t adc[16]; /**< Buffer of averaged results for all possible channels */
static volatile uint8_t adc_curch = 0; /**< Channel currently converting */
static uint8_t adc_sch; /**< Start channel */
static uint8_t adc_ech; /**< End channel */
static uint8_t adc_freerun = 0; /**< Freerun or not (bool) */

static const uint8_t ADC_AVG_SAMP = 16;		/**< how many ADC samples to average */

/**
@brief Init ADC.
@param[in]	sch		Start channel
@param[in]	ech		End channel
*/
void adc_init(uint8_t sch, uint8_t ech)
{
	if( ech > 15 ) ech = 15;
	if( sch > ech ) sch = ech;
	adc_sch = sch;
	adc_ech = ech;

	ADC_InitTypeDef  adis;
	// PCLK2 is the APB2 clock
	// ADCCLK = PCLK2/6 = 72/6 = 12MHz
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);

	// Enable ADC1 clock so that we can talk to it
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	// Put everything back to power-on defaults
	//ADC_DeInit(ADC1);

	// ADC1 Configuration
	adis.ADC_Mode = ADC_Mode_Independent;
	// Disable the scan conversion so we do one at a time
	adis.ADC_ScanConvMode = DISABLE;
	// Don't do contimuous conversions - do them on demand
	adis.ADC_ContinuousConvMode = DISABLE;
	// Start conversion by software, not an external trigger
	adis.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	// Conversions are 12 bit - put them in the lower 12 bits of the result
	adis.ADC_DataAlign = ADC_DataAlign_Right;
	// Say how many channels would be used by the sequencer
	adis.ADC_NbrOfChannel = 1;

	// Now do the setup
	ADC_Init(ADC1, &adis);
	// Enable ADC1
	ADC_Cmd(ADC1, ENABLE);

	// Enable ADC1 reset calibaration register
	ADC_ResetCalibration(ADC1);
	// Check the end of ADC1 reset calibration register
	while( ADC_GetResetCalibrationStatus(ADC1) );
	// Start ADC1 calibaration
	ADC_StartCalibration(ADC1);
	// Check the end of ADC1 calibration
	while( ADC_GetCalibrationStatus(ADC1) );

	adc_curch = adc_sch;

	ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);

	// ADC interrupt setup
	NVIC_InitTypeDef ictd;
	ictd.NVIC_IRQChannel = ADC1_IRQn;
	ictd.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&ictd);
}

/**
@brief Start next conversion.

Use if you want to control when conversions are started. Do not call if free running.
*/
void adc_startnext(void)
{
  ADC_RegularChannelConfig(ADC1, adc_curch, 1, ADC_SampleTime_1Cycles5);
  // Start the conversion
  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
  // Wait until conversion completion
  //while( ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET );
  // Get the conversion value
  //return ADC_GetConversionValue(ADC1);
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
@brief Get a channel's averaged ADC value.
@param[in]	ch		Channel to get
@return Averaged ADC value for channel
*/
uint16_t adc_get(const uint8_t ch)
{
	uint32_t g = __get_PRIMASK();
	__disable_irq();

	uint16_t r = adc[ch];

	__set_PRIMASK(g);
	return r;
}

/** @privatesection */

void ADC1_IRQHandler(void)
{
	static uint16_t sum = 0;
	static uint8_t samp = 0;

	sum += ADC_GetConversionValue(ADC1); // add new conversion result to sum
	samp++;						    // increase sample counter

	if( samp == ADC_AVG_SAMP ) {
		sum /= ADC_AVG_SAMP;	    // calc average
		adc[adc_curch] = sum;	    // store averaged conversion result
		if( ++adc_curch > adc_ech ) adc_curch = adc_sch;	  // advance channel
		samp = 0;				    // reset variables
		sum = 0;
	}

	if( adc_freerun ) adc_startnext();
}
