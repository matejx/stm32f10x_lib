
#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_desc.h"

#include <string.h>

uint8_t kbd_keybuf[16];
uint8_t kbd_keybuf_last[sizeof(kbd_keybuf)];

void EP1_IN_Callback(void); // forward decl

// ----------------------------------------------------------------------------
// KBD public methods
// ----------------------------------------------------------------------------

void kbd_init(void)
{
	memset(kbd_keybuf, 0, sizeof(kbd_keybuf));
	memcpy(kbd_keybuf_last, kbd_keybuf, sizeof(kbd_keybuf));
}

void kbd_tx(void)
{
	if( GetEPTxStatus(ENDP1) == EP_TX_VALID ) return;

	EP1_IN_Callback();
}

uint8_t kbd_down(uint8_t key)
{
	// modifier keys
	if( (key >= 0xe0) && (key <= 0xe7) ) {
		key -= 0xe0;
		kbd_keybuf[0] |= (1 << key);
		return 0;
	}

	// normal keys
	uint8_t i;

	for( i = 2; i < sizeof(kbd_keybuf); ++i ) {
		if( kbd_keybuf[i] == key ) return 0; // already in buffer
	}

	for( i = 2; i < sizeof(kbd_keybuf); ++i ) {
		if( kbd_keybuf[i] == 0 ) { // free space
			kbd_keybuf[i] = key; // put in buffer
			return 0;
		}
	}

	return 1; // no space
}

uint8_t kbd_up(uint8_t key)
{
	// modifier keys
	if( (key >= 0xe0) && (key <= 0xe7) ) {
		key -= 0xe0;
		kbd_keybuf[0] &= ~(1 << key);
		return 0;
	}

	// normal keys
	uint8_t i;

	for( i = 2; i < sizeof(kbd_keybuf); ++i ) {
		if( kbd_keybuf[i] == key ) {
			kbd_keybuf[i] = 0; // remove from buffer
			return 0;
		}
	}

	return 1; // not found
}

// ----------------------------------------------------------------------------
// USB endpoint callbacks
// ----------------------------------------------------------------------------

void EP1_IN_Callback(void)
{
	if( memcmp(kbd_keybuf_last, kbd_keybuf, sizeof(kbd_keybuf)) == 0 ) return; // no change

	memcpy(kbd_keybuf_last, kbd_keybuf, sizeof(kbd_keybuf)); // remember last

	UserToPMABufferCopy(kbd_keybuf, ENDP1_TXADDR, sizeof(kbd_keybuf));

	SetEPTxCount(ENDP1, sizeof(kbd_keybuf));
	SetEPTxValid(ENDP1);
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
    NOP_Process,
    NOP_Process,
    NOP_Process,
    NOP_Process,
    NOP_Process
  };
