#ifndef MAT_ADC_H
#define MAT_ADC_H

#include <inttypes.h>

void adc_init(uint32_t ench, uint8_t navg);
void adc_startnext(void);
void adc_startfree(void);
void adc_stopfree(void);
uint16_t adc_get(const uint8_t ch);

#endif
