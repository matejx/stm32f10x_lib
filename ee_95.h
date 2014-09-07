#ifndef MAT_EE95_H
#define MAT_EE95_H

#include <inttypes.h>

void ee95_init(uint8_t n);
uint8_t ee95_rd(uint8_t n, uint32_t adr, uint8_t* buf, uint16_t len);
void ee95_wren(uint8_t n);
uint8_t ee95_wr(uint8_t n, uint32_t adr, uint8_t* buf, uint16_t len);

#endif
