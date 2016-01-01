#ifndef MAT_DAC_H
#define MAT_DAC_H

#include <inttypes.h>

void dac_init(uint8_t n);
void dac_set(uint8_t n, uint16_t v);

#endif
