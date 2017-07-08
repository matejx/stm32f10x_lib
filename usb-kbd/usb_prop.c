
#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_desc.h"

#include <string.h>

//extern int ser_printf(const char* s, ...);

__IO uint16_t wIstr;  // ISTR register last read value

DEVICE Device_Table =
{
  EP_NUM,
  1
};

// ------------------------------------------------------------------
// tbuf copyroutine helper
// ------------------------------------------------------------------

uint8_t tbuf8[64];
uint8_t tbuf8len;

uint8_t *CR_tbuf8(uint16_t Length)
{
  if( Length == 0 ) {
    pInformation->Ctrl_Info.Usb_wLength = tbuf8len;
    return 0;
  } else {
    return tbuf8;
  }
}

// ------------------------------------------------------------------
// User_Standard_Requests callback impl.
// ------------------------------------------------------------------

USER_STANDARD_REQUESTS User_Standard_Requests =
{
  NOP_Process,
  NOP_Process,
  NOP_Process,
  NOP_Process,
  NOP_Process,
  NOP_Process,
  NOP_Process,
  NOP_Process,
  NOP_Process
};

// ------------------------------------------------------------------
// Forward declarations
// ------------------------------------------------------------------

uint8_t *CR_GetHidReportDescriptor(uint16_t);
uint8_t *CR_GetHidHidDescriptor(uint16_t);
//uint8_t *CR_HID_GetReport(uint16_t);
//uint8_t *CR_HID_GetProtocol(uint16_t);
//uint8_t *CR_HID_GetIdle(uint16_t);

// ------------------------------------------------------------------
// Device_Property impl.
// ------------------------------------------------------------------

void DP_Init(void)
{
  // Update the serial number string descriptor with the data from the unique ID
  //Get_SerialNum();

  pInformation->Current_Configuration = 0;

  // Perform basic device initialization operations
  USB_SIL_Init();
}

void DP_Reset(void)
{
  pInformation->Current_Configuration = 0;
  pInformation->Current_Feature = 0; //ConfigDescriptor[7];
  pInformation->Current_Interface = 0;

  SetBTABLE(BTABLE_ADDRESS);

  // Initialize Endpoint 0
  SetEPType(ENDP0, EP_CONTROL);
  SetEPTxStatus(ENDP0, EP_TX_STALL);
  SetEPRxAddr(ENDP0, ENDP0_RXADDR);
  SetEPTxAddr(ENDP0, ENDP0_TXADDR);
  Clear_Status_Out(ENDP0);
  SetEPRxCount(ENDP0, Device_Property.MaxPacketSize);
  SetEPRxValid(ENDP0);

  // Initialize Endpoint 1
  SetEPType(ENDP1, EP_INTERRUPT);
  SetEPTxAddr(ENDP1, ENDP1_TXADDR);
  SetEPTxStatus(ENDP1, EP_TX_NAK);
  SetEPRxStatus(ENDP1, EP_RX_DIS);

  // Set this device to respond on default address
  SetDeviceAddress(0);
}

void DP_Status_In(void)
{
//  ser_printf("stat in\r\n");
}

void DP_Status_Out(void)
{
//  ser_printf("stat out\r\n");
}

RESULT DP_Data_Setup(uint8_t RequestNo)
{
  uint8_t *(*CopyRoutine)(uint16_t) = 0;

//  ser_printf("Setup %x %x %x\r\n", pInformation->USBbmRequestType, RequestNo, pInformation->USBwValues);

  if( Type_Recipient == (STANDARD_REQUEST | INTERFACE_RECIPIENT) ) {
    if( RequestNo == GET_DESCRIPTOR ) {
      if( pInformation->USBwValue1 == USB_REPORT_DESCRIPTOR_TYPE ) {
        CopyRoutine = CR_GetHidReportDescriptor;
      } else
	  if( pInformation->USBwValue1 == USB_HID_DESCRIPTOR_TYPE ) {
        CopyRoutine = CR_GetHidHidDescriptor;
      }
	}
  } else
  if( Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT) ) {
	if( RequestNo == GET_REPORT ) {
	  // fake empty report
	  tbuf8len = pInformation->Ctrl_Info.Usb_wLength;
	  if( tbuf8len > sizeof(tbuf8) ) tbuf8len = sizeof(tbuf8);
	  memset(tbuf8, 0, tbuf8len);
      CopyRoutine = CR_tbuf8;
    } else
    if( RequestNo == GET_PROTOCOL ) {
	  // report that report protocol is being used
	  tbuf8len = 1;
	  tbuf8[0] = 1;
      CopyRoutine = CR_tbuf8;
	}
  }

  if( CopyRoutine == 0 ) return USB_UNSUPPORT;

  pInformation->Ctrl_Info.CopyData = CopyRoutine;
  pInformation->Ctrl_Info.Usb_wOffset = 0;
  (*CopyRoutine)(0);
  return USB_SUCCESS;
}

RESULT DP_NoData_Setup(uint8_t RequestNo)
{
  if( Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT) ) {
	if( RequestNo == SET_IDLE ) {
	  // Report success, but ignore. USB is fun! :/
	  return USB_SUCCESS;
    } else
	if( RequestNo == SET_PROTOCOL ) {
	  return USB_SUCCESS;
	}
  }

  return USB_UNSUPPORT;
}

uint8_t *CR_GetDeviceDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &Device_Descriptor);
}

uint8_t *CR_GetConfigDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &Config_Descriptor);
}

uint8_t *CR_GetStringDescriptor(uint16_t Length)
{
  uint8_t wValue0 = pInformation->USBwValue0;

  if( wValue0 > NUM_STRING_DESC ) {
    return 0;
  }

  return Standard_GetDescriptorData(Length, &String_Descriptor[wValue0]);
}

uint8_t *CR_GetHidReportDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &HID_Report_Descriptor);
}

uint8_t *CR_GetHidHidDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &HID_Hid_Descriptor);
}

RESULT DP_Get_Interface_Setting(uint8_t Interface, uint8_t AlternateSetting)
{
  if( AlternateSetting > 0 ) {
    return USB_UNSUPPORT;
  }

  if( Interface > 1 ) {
    return USB_UNSUPPORT;
  }

  return USB_SUCCESS;
}

DEVICE_PROP Device_Property =
  {
    DP_Init,
    DP_Reset,
    DP_Status_In,
    DP_Status_Out,
    DP_Data_Setup,
    DP_NoData_Setup,
    DP_Get_Interface_Setting,
    CR_GetDeviceDescriptor,
    CR_GetConfigDescriptor,
    CR_GetStringDescriptor,
    0,
    64 // MAX PACKET SIZE
  };

// ------------------------------------------------------------------
// USB ISR
// ------------------------------------------------------------------

void USB_ISR(void)
{
  wIstr = _GetISTR();

  if (wIstr & ISTR_CTR) {
    // CTR flag is read only
    CTR_LP();
  }

  if (wIstr & ISTR_DOVR) {
    _SetISTR((uint16_t)CLR_DOVR);
  }

  if (wIstr & ISTR_ERR) {
    _SetISTR((uint16_t)CLR_ERR);
  }

  if (wIstr & ISTR_WKUP) {
    _SetISTR((uint16_t)CLR_WKUP);
  }

  if (wIstr & ISTR_SUSP) {
    _SetISTR((uint16_t)CLR_SUSP);
  }

  if (wIstr & ISTR_RESET) {
    _SetISTR((uint16_t)CLR_RESET);
    Device_Property.Reset();
  }

  if (wIstr & ISTR_SOF) {
    _SetISTR((uint16_t)CLR_SOF);
  }

  if (wIstr & ISTR_ESOF) {
    _SetISTR((uint16_t)CLR_ESOF);
  }
}
