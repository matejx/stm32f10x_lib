
#ifndef HMC8883L_H
#define HMC8883L_H

#include <stdbool.h>

uint8_t hmc_init(uint8_t cra, uint8_t crb, uint8_t mode);

uint8_t hmc_read(int16_t* x, int16_t* y, int16_t* z);

float hmc_heading(int16_t x, int16_t y);

bool hmc_present(void);

#endif
