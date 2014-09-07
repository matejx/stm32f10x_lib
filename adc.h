#ifndef MAT_ADC_H
#define MAT_ADC_H

#include <inttypes.h>

void adc_init(uint8_t sch, uint8_t ech);
void adc_startnext(void);
void adc_startfree(void);
uint16_t adc_get(const uint8_t ch);

#endif
