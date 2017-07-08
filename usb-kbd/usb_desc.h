#ifndef USB_DESC_H
#define USB_DESC_H

#define USB_DEVICE_DESCRIPTOR_TYPE              0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE       0x02
#define USB_STRING_DESCRIPTOR_TYPE              0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE           0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE            0x05

#define USB_HID_DESCRIPTOR_TYPE                 0x21
#define USB_REPORT_DESCRIPTOR_TYPE              0x22

#define EP0_DATA_SIZE          					64
#define EP1_DATA_SIZE          					64

extern ONE_DESCRIPTOR Config_Descriptor;
extern ONE_DESCRIPTOR Device_Descriptor;
extern ONE_DESCRIPTOR HID_Report_Descriptor;
extern ONE_DESCRIPTOR HID_Hid_Descriptor;

#define NUM_STRING_DESC 3
extern ONE_DESCRIPTOR String_Descriptor[NUM_STRING_DESC];

typedef enum _HID_REQUESTS
{
  GET_REPORT = 1,
  GET_IDLE,
  GET_PROTOCOL,

  SET_REPORT = 9,
  SET_IDLE,
  SET_PROTOCOL
} HID_REQUESTS;

#endif
