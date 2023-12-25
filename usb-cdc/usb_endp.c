
#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_desc.h"

#include "mat/serialq.h"
#include "mat/circbuf8.h"
#include "mat/itoa.h"

#include <string.h>

volatile struct cbuf8_t cdc_rxq;
volatile struct cbuf8_t cdc_txq;

volatile uint8_t ep1_in_transferring = 0;

uint8_t EP1_IN_PrepareTX(void); // forward decl

// ----------------------------------------------------------------------------
// CDC public methods
// ----------------------------------------------------------------------------

void cdc_init(uint8_t* txb, uint16_t txs, uint8_t* rxb, uint16_t rxs)
{
	cbuf8_clear(&cdc_txq, txb, txs);
	cbuf8_clear(&cdc_rxq, rxb, rxs);
}

void cdc_tx(void)
{
  if( ep1_in_transferring ) return;
	ep1_in_transferring = 1;
	EP1_IN_PrepareTX();
}

void cdc_putc_(const char a)
{
	while( 1 ) {
		if( cbuf8_put(&cdc_txq, a) ) {
			break;
		} else {
			cdc_tx();
		}
	}
}

void cdc_putc(const char a)
{
	cdc_putc_(a);
	cdc_tx();
}

void cdc_putsn(const char* s, uint8_t n)
{
	while( n-- ) {
		cdc_putc_(*s);
		++s;
	}
	cdc_tx();
}

void cdc_puts(const char* s)
{
	while( *s )	{
		cdc_putc_(*s);
		++s;
	}
	cdc_tx();
}

void cdc_puti_lc(const int32_t a, const uint8_t r, uint8_t w, char c)
{
	char s[16];

	itoa(a, s, r);

	while( w-- > strlen(s) ) { cdc_putc_(c); }

	cdc_puts(s);
}

void cdc_putf(float f, uint8_t prec)
{
	if( f < 0 ) {
		f = -f;
		cdc_putc('-');
	}
	cdc_puti_lc(f, 10, 0, 0);
	cdc_putc('.');
	f = f - (int)f;
	uint8_t i = prec;
	while( i-- ) f *= 10;
	cdc_puti_lc(f, 10, prec, '0');
}

uint8_t cdc_getc(uint8_t* const d)
{
	uint8_t r = cbuf8_get(&cdc_rxq, d);
	return r;
}

// ----------------------------------------------------------------------------
// USB endpoint callbacks
// ----------------------------------------------------------------------------

uint8_t EP1_IN_PrepareTX(void)
{
	if( GetEPTxStatus(ENDP1) == EP_TX_VALID ) return 0;

	uint8_t len = 0;
	uint8_t dh,dl;
	uint32_t* pma = (uint32_t*)(PMAAddr + (2 * ENDP1_TXADDR));

	while( cbuf8_get(&cdc_txq, &dl) ) {
		++len;
		dh = 0;
		if( cbuf8_get(&cdc_txq, &dh) ) ++len;
		*pma++ = dh * 0x100 + dl;
		if( len >= VCP_DATA_SIZE ) break;
	}

	if( len ) {
		SetEPTxCount(ENDP1, len);
		SetEPTxValid(ENDP1);
		return 1;
	}

	return 0;
}

void EP1_IN_Callback(void)
{
	ep1_in_transferring = EP1_IN_PrepareTX();
}

void EP3_OUT_Callback(void)
{
	uint8_t len = GetEPRxCount(ENDP3);
	uint16_t d;
	uint32_t* pma = (uint32_t*)(PMAAddr + (2 * ENDP3_RXADDR));

	while( len ) {
		d = *pma++;
		cbuf8_put(&cdc_rxq, d);
		len--;
		if( len ) {
			cbuf8_put(&cdc_rxq, d >> 8);
			len--;
		}
	}

	// flag EP as ready for next reception
	SetEPRxValid(ENDP3);
}

void (*pEpInt_IN[7])(void) =
  {
    EP1_IN_Callback,
    NOP_Process,
    NOP_Process,
    NOP_Process,
    NOP_Process,
    NOP_Process,
    NOP_Process
  };

void (*pEpInt_OUT[7])(void) =
  {
    NOP_Process,
    NOP_Process,
    EP3_OUT_Callback,
    NOP_Process,
    NOP_Process,
    NOP_Process,
    NOP_Process
  };
