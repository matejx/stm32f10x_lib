
#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_desc.h"

#include "mat/serialq.h"
#include "mat/circbuf8.h"

#include <string.h>
#include "../itoa.h"

volatile struct cbuf8_t cdc_rxq;
volatile struct cbuf8_t cdc_txq;

void EP1_IN_Callback(void); // forward decl

void cdc_init(uint8_t* txb, uint8_t txs, uint8_t* rxb, uint8_t rxs)
{
	cbuf8_clear(&cdc_txq, txb, txs);
	cbuf8_clear(&cdc_rxq, rxb, rxs);
}

void cdc_tx(void)
{
	if( GetEPTxStatus(ENDP1) == EP_TX_VALID ) return; // previous packet not yet sent

	EP1_IN_Callback();
}

void cdc_putc_(char a)
{
	while( 1 ) {
		if( cbuf8_put(&cdc_txq, a) ) {
			break;
		} else {
			cdc_tx();
		}
	}
}

void cdc_putc(char a)
{
	cdc_putc_(a);
	cdc_tx();
}

void cdc_putsn(char* s, uint8_t n)
{
	while( n-- ) {
		cdc_putc_(*s);
		s++;
	}
	cdc_tx();
}

void cdc_puts(char* s)
{
	while( *s )	{
		cdc_putc_(*s);
		s++;
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
	cdc_puti_lc(f, 10, 0, '0');
	cdc_putc_('.');
	while( prec-- ) {
		f = f - (int)f;
		f = f * 10;
		cdc_puti_lc(f, 10, 0, '0');
	}
}

uint8_t cdc_getc(uint8_t* const d)
{
	uint8_t r = cbuf8_get(&cdc_rxq, d);
	return r;
}

void EP1_IN_Callback(void)
{
	uint8_t len = 0;
	uint16_t d;
	uint16_t* pma = (uint16_t*)(PMAAddr + (2 * ENDP1_TXADDR));

	while( cbuf8_get(&cdc_txq, (uint8_t*)&d) && (len < VCP_DATA_SIZE) ) {
		++len;
		if( cbuf8_get(&cdc_txq, 1+(uint8_t*)&d) ) ++len;
		*pma++ = d;
		pma++;
	}

	if( len == 0 ) return;

	SetEPTxCount(ENDP1, len);
	SetEPTxValid(ENDP1);
}

void EP3_OUT_Callback(void)
{
	uint8_t len = GetEPRxCount(ENDP3);
	uint16_t d;
	uint16_t* pma = (uint16_t*)(PMAAddr + (2 * ENDP3_RXADDR));

	while( len ) {
		d = *pma++;
		pma++;
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
