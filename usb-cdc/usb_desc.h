#ifndef USB_DESC_H
#define USB_DESC_H

#define USB_DEVICE_DESCRIPTOR_TYPE              0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE       0x02
#define USB_STRING_DESCRIPTOR_TYPE              0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE           0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE            0x05

#define VCP_DATA_SIZE          64
#define VCP_INT_SIZE           8

extern ONE_DESCRIPTOR Config_Descriptor;
extern ONE_DESCRIPTOR Device_Descriptor;

#define NUM_STRING_DESC 4
extern ONE_DESCRIPTOR String_Descriptor[NUM_STRING_DESC];

#endif
