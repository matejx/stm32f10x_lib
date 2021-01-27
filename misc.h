#ifndef MAT_MISC_H
#define MAT_MISC_H

#include "stm32f10x.h"

uint8_t misc_gpio_num(GPIO_TypeDef* port);
void misc_gpio_config(GPIO_TypeDef* port, uint16_t pin, GPIOMode_TypeDef mode);
void misc_exti_setup(GPIO_TypeDef* port, uint16_t pin, EXTITrigger_TypeDef trg);

#endif
