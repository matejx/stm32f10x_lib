#ifndef USB_CONF_H
#define USB_CONF_H

#define EP_NUM              (4)

#define BTABLE_ADDRESS      (0x000)

#define ENDP0_RXADDR        (0x040)
#define ENDP0_TXADDR        (0x080)

#define ENDP1_TXADDR        (0x0C0)
#define ENDP2_TXADDR        (0x100)
#define ENDP3_RXADDR        (0x140)

#define IMR_MSK (CNTR_CTRM | CNTR_WKUPM | CNTR_SUSPM | CNTR_ERRM | CNTR_SOFM | CNTR_ESOFM | CNTR_RESETM )

//#define IMR_MSK (CNTR_CTRM  | CNTR_RESETM )

#endif
