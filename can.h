#ifndef MAT_CAN_H
#define MAT_CAN_H

// valid for HSE_VALUE 8 MHz
#define CAN_BR_1000 6
#define CAN_BR_500 12
#define CAN_BR_250 24
#define CAN_BR_125 48
#define CAN_BD_100 60
#define CAN_BR_50 120
#define CAN_BR_20 300
#define CAN_BR_10 600

void can_init(uint16_t br, uint8_t md);
void can_filter(uint32_t id, uint32_t msk);
uint8_t can_tx(CanTxMsg* msg);
uint8_t can_rx(CanRxMsg* msg);

#endif
