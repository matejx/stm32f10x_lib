#include <stm32f10x.h>
#include <stm32f10x_i2c.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>

#include "i2c.h"

#define i2c_waitfor(x, y) Timeout = 0xffff; while (x) { if (Timeout-- == 0) return y; }

struct I2C_DevDef
{
	uint8_t i2c_apb;
	I2C_TypeDef* i2c;
	uint32_t i2c_clk;
	GPIO_TypeDef* gpio;
	uint32_t gpio_clk;
	uint16_t pin_scl;
	uint16_t pin_sda;
};

struct I2C_DevDef I2C1_PinDef = {1, I2C1, RCC_APB1Periph_I2C1, GPIOB, RCC_APB2Periph_GPIOB, GPIO_Pin_6, GPIO_Pin_7};
struct I2C_DevDef I2C2_PinDef = {1, I2C2, RCC_APB1Periph_I2C2, GPIOB, RCC_APB2Periph_GPIOB, GPIO_Pin_10, GPIO_Pin_11};

struct I2C_DevDef* i2c_get_pdef(uint8_t I2Cy)
{
	if( I2Cy == 1 ) return &I2C1_PinDef;
	if( I2Cy == 2 ) return &I2C2_PinDef;
	
	return 0;
}

void i2c_init(uint8_t I2Cy, const uint32_t clkspd)
{
	struct I2C_DevDef* pdef = i2c_get_pdef(I2Cy);

    // Enable GPIO clocks
    RCC_APB2PeriphClockCmd(pdef->gpio_clk, ENABLE);

	// I2C1 clock enable
	RCC_APB1PeriphClockCmd(pdef->i2c_clk, ENABLE);

	// I2C1 SDA and SCL configuration
	GPIO_InitTypeDef iotd;
	iotd.GPIO_Pin = pdef->pin_sda | pdef->pin_scl;
	iotd.GPIO_Speed = GPIO_Speed_50MHz;
	iotd.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_Init(pdef->gpio, &iotd);

	// I2C1 Reset
	RCC_APB1PeriphResetCmd(pdef->i2c_clk, ENABLE);
	RCC_APB1PeriphResetCmd(pdef->i2c_clk, DISABLE);

	// Configure I2Cx
	I2C_InitTypeDef i2td;
	i2td.I2C_ClockSpeed = clkspd;
	i2td.I2C_Mode = I2C_Mode_I2C;
	i2td.I2C_DutyCycle = I2C_DutyCycle_2;
	i2td.I2C_OwnAddress1 = 0;
	i2td.I2C_Ack = I2C_Ack_Enable;
	i2td.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_Init(pdef->i2c, &i2td);
	
	I2C_Cmd(pdef->i2c, ENABLE); 
}

uint8_t i2c_rd(uint8_t I2Cy, uint8_t adr, uint8_t *buf, uint32_t nbyte)
{
	I2C_TypeDef* I2Cx = i2c_get_pdef(I2Cy)->i2c;
	__IO uint32_t Timeout = 0;

	//    I2Cx->CR2 |= I2C_IT_ERR;  interrupts for errors 

	if (!nbyte) { return 1; }

	// Wait for idle I2C interface
	i2c_waitfor(I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY), 1);

	// Enable Acknowledgement, clear POS flag
	I2C_AcknowledgeConfig(I2Cx, ENABLE);
	I2C_NACKPositionConfig(I2Cx, I2C_NACKPosition_Current);

	// Intiate Start Sequence (wait for EV5)
	I2C_GenerateSTART(I2Cx, ENABLE);
	i2c_waitfor(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT), 2);

	// Send Address
	I2C_Send7bitAddress(I2Cx, adr, I2C_Direction_Receiver);

	// EV6
	i2c_waitfor(!I2C_GetFlagStatus(I2Cx, I2C_FLAG_ADDR), 3);

	if (nbyte == 1) {
		// Clear Ack bit      
		I2C_AcknowledgeConfig(I2Cx, DISABLE);       

		// EV6_1 -- must be atomic -- Clear ADDR, generate STOP
		__disable_irq();
		(void) I2Cx->SR2;                           
		I2C_GenerateSTOP(I2Cx,ENABLE);      
		__enable_irq();

		// Receive data   EV7
		i2c_waitfor(!I2C_GetFlagStatus(I2Cx, I2C_FLAG_RXNE), 4);
		*buf++ = I2C_ReceiveData(I2Cx);
		} else 
		if (nbyte == 2) {
		// Set POS flag
		I2C_NACKPositionConfig(I2Cx, I2C_NACKPosition_Next);

		// EV6_1 -- must be atomic and in this order
		__disable_irq();
		(void) I2Cx->SR2;                           // Clear ADDR flag
		I2C_AcknowledgeConfig(I2Cx, DISABLE);       // Clear Ack bit
		__enable_irq();

		// EV7_3  -- Wait for BTF, program stop, read data twice
		i2c_waitfor(!I2C_GetFlagStatus(I2Cx, I2C_FLAG_BTF), 5);

		__disable_irq();
		I2C_GenerateSTOP(I2Cx,ENABLE);
		*buf++ = I2Cx->DR;
		__enable_irq();

		*buf++ = I2Cx->DR;
	} else {
		(void) I2Cx->SR2;                           // Clear ADDR flag
		while (nbyte-- != 3) {
			// EV7 -- cannot guarantee 1 transfer completion time, wait for BTF 
			//        instead of RXNE
			i2c_waitfor(!I2C_GetFlagStatus(I2Cx, I2C_FLAG_BTF), 6); 
			*buf++ = I2C_ReceiveData(I2Cx);
		}

		i2c_waitfor(!I2C_GetFlagStatus(I2Cx, I2C_FLAG_BTF), 7);

		// EV7_2 -- Figure 1 has an error, doesn't read N-2 !
		I2C_AcknowledgeConfig(I2Cx, DISABLE);           // clear ack bit

		__disable_irq();
		*buf++ = I2C_ReceiveData(I2Cx);             // receive byte N-2
		I2C_GenerateSTOP(I2Cx,ENABLE);                  // program stop
		__enable_irq();

		*buf++ = I2C_ReceiveData(I2Cx);             // receive byte N-1

		// wait for byte N
		i2c_waitfor(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED), 8); 
		*buf++ = I2C_ReceiveData(I2Cx);

		nbyte = 0;
	}

	// Wait for stop
	i2c_waitfor(I2C_GetFlagStatus(I2Cx, I2C_FLAG_STOPF), 9);
	return 0;
}

/*
 * Read buffer of bytes -- AN2824 Figure 3
 */

uint8_t i2c_wr(uint8_t I2Cy, uint8_t adr, const uint8_t* buf,  uint32_t nbyte)
{
	I2C_TypeDef* I2Cx = i2c_get_pdef(I2Cy)->i2c;
	__IO uint32_t Timeout = 0;

	/* Enable Error IT (used in all modes: DMA, Polling and Interrupts */
	// I2Cx->CR2 |= I2C_IT_ERR;

	if (nbyte) {
		i2c_waitfor(I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY), 1);

		// Intiate Start Sequence
		I2C_GenerateSTART(I2Cx, ENABLE);
		i2c_waitfor(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT), 2);

		// Send Address  EV5
		I2C_Send7bitAddress(I2Cx, adr, I2C_Direction_Transmitter);
		i2c_waitfor(!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED), 3);

		// EV6
		// Write first byte EV8_1
		I2C_SendData(I2Cx, *buf++);

		while (--nbyte) {
			// wait on BTF
			i2c_waitfor(!I2C_GetFlagStatus(I2Cx, I2C_FLAG_BTF), 4);
			I2C_SendData(I2Cx, *buf++);
		}

		i2c_waitfor(!I2C_GetFlagStatus(I2Cx, I2C_FLAG_BTF), 5);
		I2C_GenerateSTOP(I2Cx, ENABLE);
		i2c_waitfor(I2C_GetFlagStatus(I2C1, I2C_FLAG_STOPF), 6);
	}
	
	return 0;
}
