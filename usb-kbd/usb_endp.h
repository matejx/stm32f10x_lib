#ifndef USB_ENDP_H
#define USB_ENDP_H

#include <inttypes.h>

void kbd_init(void);
uint8_t kbd_down(uint8_t key);
uint8_t kbd_up(uint8_t key);
uint8_t kbd_tx(void);

#endif
