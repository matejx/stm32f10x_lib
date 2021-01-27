#ifndef MAT_CAN_H
#define MAT_CAN_H

/*
Some baudrate: brps,bs1,bs2 values for can_init:

PCLK1=36M
100: 18,14,5 (75%)

PCLK1=8M
100: 5,11,4 (75%)

Visit www.bittiming.can-wiki.info to get more.
*/

void can_init(uint16_t brps, uint8_t bs1, uint8_t bs2, uint8_t md);
void can_shutdown(void);
uint8_t can_filter(uint32_t id, uint32_t msk);
uint8_t can_tx(CanTxMsg* msg);
uint8_t can_rx(CanRxMsg* msg);
uint8_t can_tx_full(void);

#endif
