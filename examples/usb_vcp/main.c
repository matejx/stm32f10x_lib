
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_endp.h"

volatile uint32_t msTicks;

static uint8_t cdcrxbuf[64];
static uint8_t cdctxbuf[64];

void usb_hwinit(void);

void _exit(int status)
{
	while(1) {}
}

void SysTick_Handler(void)
{
	msTicks++;			// increment counter necessary in _delay_ms()
}

void _delay_ms (uint32_t dlyTicks)
{
	uint32_t curTicks = msTicks;
	while ((msTicks - curTicks) < dlyTicks);
}

int main(void)
{
  usb_hwinit();

  if( SysTick_Config(SystemCoreClock / 1000) ) { // setup SysTick Timer for 1 msec interrupts
    while( 1 );                                  // capture error
  }

//  ser_init(1, 115200, uart1txbuf, sizeof(uart1txbuf), uart1rxbuf, sizeof(uart1rxbuf));

  cdc_init(cdctxbuf, sizeof(cdctxbuf), cdcrxbuf, sizeof(cdcrxbuf));

  USB_Init(); // suspends device if no USB connected

  while (1) {
      uint8_t buf[VCP_DATA_SIZE];
	  uint8_t len = 0;
	  uint8_t d;
      while( cdc_getc(&d) ) {
	    buf[len] = d;
		len++;
	  }
	  if( len ) {
		cdc_putsn((char*)buf, len);
	  }
  }
}
