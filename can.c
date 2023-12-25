/**
@file		can.c
@brief		CAN bus routines
@author		Matej Kogovsek
@copyright	LGPL 2.1
@note		This file is part of mat-stm32f1-lib
*/

#include <stm32f10x.h>
#include <stm32f10x_can.h>

/**
If defined CAN_RX_INT, implement can_rx_callback(CanRxMsg*) to receive messages (preferred).
If not defined, call can_rx(CanRxMsg*) periodically.
*/
#ifdef CAN_RX_INT
/**
@brief Extern. Implement to receive CAN messages.

The function is called by an interrupt when CAN_RX_INT is defined.
@param[in]	msg		Pointer to interrupt local variable. Copy to keep after callback exits.
*/
extern void can_rx_callback(CanRxMsg* msg);
#endif

static uint8_t canfilnum; /*< number of CAN filters defined. */

/**
@brief Add a filter to CAN reception logic. ID bits masked with 1 have to match.
@param[in]	id		Filter ID bits
@param[in]	msk		Filter mask bits
@return Number of filters defined (max 13) on success, 0 otherwise.
*/
uint8_t can_filter(uint32_t id, uint32_t msk)
{
	if( canfilnum >= 14 ) return 0;

	// CAN1 filter init
	CAN_FilterInitTypeDef fitd;
	fitd.CAN_FilterNumber = canfilnum++;;
	fitd.CAN_FilterMode = CAN_FilterMode_IdMask;
	fitd.CAN_FilterScale = CAN_FilterScale_32bit;
	fitd.CAN_FilterIdHigh = id >> 16;
	fitd.CAN_FilterIdLow = 0;
	fitd.CAN_FilterMaskIdHigh = msk >> 16;
	fitd.CAN_FilterMaskIdLow = 0;
	fitd.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;
	fitd.CAN_FilterActivation = ENABLE;
	CAN_FilterInit(&fitd);

	return canfilnum;
}

/**
@brief Init CAN.
@param[in]	brps	Prescaler, PCLK1 dependent
@param[in]	bs1		BS1 time quanta (1..16)
@param[in]	bs2		BS2 time quanta (1..8)
@param[in]	md		Mode (CAN_Mode_Normal, CAN_Mode_Silent, CAN_Mode_LoopBack)
*/
void can_init(uint16_t brps, uint8_t bs1, uint8_t bs2, uint8_t md)
{
	canfilnum = 0;

	GPIO_InitTypeDef  iotd;

	// configure CAN1 IOs
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB, ENABLE);

	// configure CAN1 RX pin
	iotd.GPIO_Pin = GPIO_Pin_8;
	iotd.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB, &iotd);

	// configure CAN1 TX pin
	iotd.GPIO_Pin = GPIO_Pin_9;
	iotd.GPIO_Mode = GPIO_Mode_AF_PP;
	iotd.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOB, &iotd);

	// remap CAN1 GPIOs to GPIOB
	GPIO_PinRemapConfig(GPIO_Remap1_CAN1, ENABLE);

	// CAN1 periph clocks enable
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

	// CAN1 register init
	CAN_DeInit(CAN1);

	// init CAN1
	CAN_InitTypeDef cnis;
	CAN_StructInit(&cnis);
	cnis.CAN_TTCM = DISABLE;
	cnis.CAN_ABOM = DISABLE;
	cnis.CAN_AWUM = DISABLE;
	cnis.CAN_NART = ENABLE;
	cnis.CAN_RFLM = DISABLE;
	cnis.CAN_TXFP = ENABLE;
	cnis.CAN_Mode = md;
	cnis.CAN_SJW = CAN_SJW_1tq;
	cnis.CAN_BS1 = bs1-1;
	cnis.CAN_BS2 = bs2-1;
	cnis.CAN_Prescaler = brps;
	CAN_Init(CAN1, &cnis);

#ifdef CAN_RX_INT
	// enable RX interrupt
	CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);

	NVIC_InitTypeDef ictd;
#ifdef STM32F10X_CL
  ictd.NVIC_IRQChannel = CAN1_RX0_IRQn;
#else
	ictd.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
#endif
	ictd.NVIC_IRQChannelPreemptionPriority = 0x0;
	ictd.NVIC_IRQChannelSubPriority = 0x0;
	ictd.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&ictd);
#endif
#ifdef CAN_TX_INT
	CAN_ITConfig(CAN1, CAN_IT_TME, ENABLE);

	NVIC_InitTypeDef ictd2;
#ifdef STM32F10X_CL
	ictd2.NVIC_IRQChannel = CAN1_TX_IRQn;
#else
	ictd2.NVIC_IRQChannel = USB_HP_CAN1_TX_IRQn;
#endif
	ictd2.NVIC_IRQChannelPreemptionPriority = 0x0;
	ictd2.NVIC_IRQChannelSubPriority = 0x0;
	ictd2.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&ictd2);
#endif
}

/**
@brief Deinit CAN.
*/
void can_shutdown(void)
{
	CAN_DeInit(CAN1);
}

/**
@brief Transmit CAN message.
@param[in]	msg		Message to transmit
@return True on success, false otherwise.
*/
uint8_t can_tx(CanTxMsg* msg)
{
	return CAN_TxStatus_NoMailBox != CAN_Transmit(CAN1, msg);
}

/**
@brief Check if CAN TX fifo is full.
*/
uint8_t can_tx_full(void)
{
	uint32_t m = CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2;
	return ((CAN1->TSR & m) == 0);
}

/**
@brief Receive CAN message.

Use only if CAN_RX_INT not defined. Call periodically.
@param[out]	msg		Pointer to caller allocated CanRxMsg
@return True on success, false otherwise.
*/
uint8_t can_rx(CanRxMsg* msg)
{
	if( CAN_MessagePending(CAN1, CAN_FIFO0) ) {
		CAN_Receive(CAN1, CAN_FIFO0, msg);
		return 1;
	}
	return 0;
}

/** @privatesection */

#ifdef CAN_RX_INT
#ifdef STM32F10X_CL
void CAN1_RX0_IRQHandler(void)
#else
void USB_LP_CAN1_RX0_IRQHandler(void)
#endif
{
	CanRxMsg rxmsg;
	CAN_Receive(CAN1, CAN_FIFO0, &rxmsg);
	can_rx_callback(&rxmsg);
}
#endif

#ifdef CAN_TX_INT
extern void can_tx_callback(void);
#ifdef STM32F10X_CL
void CAN1_TX0_IRQHandler(void)
#else
void USB_HP_CAN1_TX_IRQHandler(void)
#endif
{
	can_tx_callback();
}
#endif
