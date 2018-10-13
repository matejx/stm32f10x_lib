
#include "usb_lib.h"
#include "usb_conf.h"
#include "usb_desc.h"

__IO uint16_t wIstr;  // ISTR register last read value

DEVICE Device_Table =
{
  EP_NUM,
  1
};

// ------------------------------------------------------------------
// VCP specific functionality
// ------------------------------------------------------------------

#define SET_COMM_FEATURE            0x02
#define SET_LINE_CODING             0x20
#define GET_LINE_CODING             0x21
#define SET_CONTROL_LINE_STATE      0x22

typedef struct {
  uint32_t bitrate;
  uint8_t format;
  uint8_t paritytype;
  uint8_t datatype;
} LINE_CODING;

LINE_CODING linecoding = { 115200, 0x00, 0x00, 0x08 };

uint8_t* CopyData_linecoding(uint16_t len)
{
  if( len == 0 ) {
    pInformation->Ctrl_Info.Usb_wLength = sizeof(linecoding);
    return 0;
  }
  return (uint8_t*)&linecoding;
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
  SetEPType(ENDP1, EP_BULK);
  SetEPTxAddr(ENDP1, ENDP1_TXADDR);
  SetEPRxStatus(ENDP1, EP_RX_DIS);
  SetEPTxStatus(ENDP1, EP_TX_NAK);

  // Initialize Endpoint 2
  SetEPType(ENDP2, EP_INTERRUPT);
  SetEPTxAddr(ENDP2, ENDP2_TXADDR);
  SetEPRxStatus(ENDP2, EP_RX_DIS);
  SetEPTxStatus(ENDP2, EP_TX_NAK);

  // Initialize Endpoint 3
  SetEPType(ENDP3, EP_BULK);
  SetEPRxAddr(ENDP3, ENDP3_RXADDR);
  SetEPRxCount(ENDP3, VCP_DATA_SIZE);
  SetEPRxStatus(ENDP3, EP_RX_VALID);
  SetEPTxStatus(ENDP3, EP_TX_DIS);

  // Set this device to respond on default address
  SetDeviceAddress(0);
}

void DP_Status_In(void)
{
  //ser_puts(1, "USB stat in\r\n");
}

void DP_Status_Out(void)
{
  //ser_puts(1, "USB stat out\r\n");
}

RESULT DP_Data_Setup(uint8_t RequestNo)
{
  if( Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT) )
  {
    if( (RequestNo == GET_LINE_CODING) || (RequestNo == SET_LINE_CODING) ) {
      pInformation->Ctrl_Info.CopyData = CopyData_linecoding;
      pInformation->Ctrl_Info.Usb_wOffset = 0;
      CopyData_linecoding(0); // sets pInformation->Ctrl_Info.Usb_wLength
      return USB_SUCCESS;
    }
  }

  return USB_UNSUPPORT;
}

RESULT DP_NoData_Setup(uint8_t RequestNo)
{
  if( Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT) )
  {
    if( RequestNo == SET_COMM_FEATURE ) return USB_SUCCESS;
    if( RequestNo == SET_CONTROL_LINE_STATE ) return USB_SUCCESS;
  }

  return USB_UNSUPPORT;
}

uint8_t *DP_GetDeviceDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &Device_Descriptor);
}

uint8_t *DP_GetConfigDescriptor(uint16_t Length)
{
  return Standard_GetDescriptorData(Length, &Config_Descriptor);
}

uint8_t *DP_GetStringDescriptor(uint16_t Length)
{
  uint8_t wValue0 = pInformation->USBwValue0;

  if( wValue0 > NUM_STRING_DESC ) {
    return 0;
  }

  return Standard_GetDescriptorData(Length, &String_Descriptor[wValue0]);
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
    DP_GetDeviceDescriptor,
    DP_GetConfigDescriptor,
    DP_GetStringDescriptor,
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
