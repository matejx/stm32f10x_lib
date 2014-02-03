// ------------------------------------------------------------------
// --- spi.c - Basic routines for sending data over SPI           ---
// ---                                                            ---
// ---            24.jul.2013, Matej Kogovsek (matej@hamradio.si) ---
// ------------------------------------------------------------------

#include <inttypes.h>
#include "stm32f10x.h"

struct SPI_DevDef
{
	uint8_t spi_apb;
	SPI_TypeDef* spi;
	uint32_t spi_clk;
	GPIO_TypeDef* gpio;
	uint32_t gpio_clk;
	uint16_t pin_nss;
	uint16_t pin_sck;
	uint16_t pin_miso;
	uint16_t pin_mosi;
};

struct SPI_DevDef SPI1_PinDef = {2, SPI1, RCC_APB2Periph_SPI1, GPIOA, RCC_APB2Periph_GPIOA, GPIO_Pin_4, GPIO_Pin_5, GPIO_Pin_6, GPIO_Pin_7};
struct SPI_DevDef SPI2_PinDef = {1, SPI2, RCC_APB1Periph_SPI2, GPIOB, RCC_APB2Periph_GPIOB, GPIO_Pin_12, GPIO_Pin_13, GPIO_Pin_14, GPIO_Pin_15};

struct SPI_DevDef* spi_get_pdef(uint8_t SPIy)
{
	if( SPIy == 1 ) return &SPI1_PinDef;
	if( SPIy == 2 ) return &SPI2_PinDef;
	
	return 0;
}

// ------------------------------------------------------------------
// SPI NSS control

void spi_cs(uint8_t SPIy, uint8_t cs)
{
	struct SPI_DevDef* pdef = spi_get_pdef(SPIy);

	if( cs ) {
		GPIO_WriteBit(pdef->gpio, pdef->pin_nss, Bit_SET);
	} else {
		GPIO_WriteBit(pdef->gpio, pdef->pin_nss, Bit_RESET);
	}
}

// ------------------------------------------------------------------
// initialize SPI interface

void spi_init(uint8_t SPIy, uint16_t brps)
{
	struct SPI_DevDef* pdef = spi_get_pdef(SPIy);

	// clock config
	if( pdef->spi_apb == 1 ) {
		RCC_APB1PeriphClockCmd(pdef->spi_clk, ENABLE);
	} else {
		RCC_APB2PeriphClockCmd(pdef->spi_clk, ENABLE);
	}
	RCC_APB2PeriphClockCmd(pdef->gpio_clk, ENABLE);

	// GPIO config
	GPIO_InitTypeDef iotd;
	
	iotd.GPIO_Pin = pdef->pin_sck | pdef->pin_mosi;
	iotd.GPIO_Speed = GPIO_Speed_50MHz;
    iotd.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(pdef->gpio, &iotd);
	
	iotd.GPIO_Pin = pdef->pin_nss;
	iotd.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(pdef->gpio, &iotd);
	
	iotd.GPIO_Pin = pdef->pin_miso;
    iotd.GPIO_Mode = GPIO_Mode_IN_FLOATING;	
	GPIO_Init(pdef->gpio, &iotd);
	
	// NSS high
    spi_cs(SPIy, 1);

	// SPI config
	SPI_InitTypeDef sptd;
	sptd.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	sptd.SPI_Mode = SPI_Mode_Master;
	sptd.SPI_DataSize = SPI_DataSize_8b;
	sptd.SPI_CPOL = SPI_CPOL_Low;
	sptd.SPI_CPHA = SPI_CPHA_1Edge;
	sptd.SPI_NSS = SPI_NSS_Soft;
	sptd.SPI_BaudRatePrescaler = brps;
	sptd.SPI_FirstBit = SPI_FirstBit_MSB;
	sptd.SPI_CRCPolynomial = 7;
	SPI_Init(pdef->spi, &sptd);
	
	SPI_Cmd(pdef->spi, ENABLE);
}

// ------------------------------------------------------------------
// send and receive byte - raw

uint8_t spi_rw(uint8_t SPIy, const uint8_t d)
{
	struct SPI_DevDef* pdef = spi_get_pdef(SPIy);
	
    while (SPI_I2S_GetFlagStatus(pdef->spi, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(pdef->spi, d);
    while (SPI_I2S_GetFlagStatus(pdef->spi, SPI_I2S_FLAG_RXNE) == RESET);
	return SPI_I2S_ReceiveData(pdef->spi);
}

// ------------------------------------------------------------------
// send and receive byte

void spi_putc(uint8_t SPIy, uint8_t* d)
{
	spi_cs(SPIy, 0);
	*d = spi_rw(SPIy, *d);
	spi_cs(SPIy, 1);
}

// ------------------------------------------------------------------
// send and receive string

void spi_puts(uint8_t SPIy, char* s)
{
    spi_cs(SPIy, 0);
	while( *s ) {
		*s = spi_rw(SPIy, *s);
		s++;
	}
	spi_cs(SPIy, 1);
}

// ------------------------------------------------------------------
// send and receive string of length n

void spi_putsn(uint8_t SPIy, char* s, uint8_t n)
{
	spi_cs(SPIy, 0);
	while( n-- ) {
		*s = spi_rw(SPIy, *s);
		s++;
	}
	spi_cs(SPIy, 1);
}
