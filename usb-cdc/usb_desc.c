
#include "usb_lib.h"
#include "usb_desc.h"

// USB Standard Device Descriptor
const uint8_t DeviceDescriptor[] =
  {
    18,   // bLength
    USB_DEVICE_DESCRIPTOR_TYPE,     // bDescriptorType
    0x10,0x01,   // bcdUSB = 1.10
    0x02,   // bDeviceClass: CDC
    0x00,   // bDeviceSubClass
    0x00,   // bDeviceProtocol
    64,     // bMaxPacketSize0
    0x83,0x04,   // idVendor = 0x0483
    0x40,0x57,   // idProduct = 0x5740
    0x00,0x02,   // bcdDevice = 2.00
    1,      // Index of string descriptor describing manufacturer
    2,      // Index of string descriptor describing product
    3,      // Index of string descriptor describing the device's serial number
    1       // bNumConfigurations
  };

const uint8_t ConfigDescriptor[67] =
  {
    //Configuration Descriptor
    9,      // bLength: Configuration Descriptor size
    USB_CONFIGURATION_DESCRIPTOR_TYPE,      // bDescriptorType: Configuration
    sizeof(ConfigDescriptor),0x00,       // wTotalLength:no of returned bytes
    2,      // bNumInterfaces: 2 interface
    1,      // bConfigurationValue: Configuration value
    0,      // iConfiguration: Index of string descriptor describing the configuration
    0x80,   // bmAttributes: self powered
    50,     // MaxPower 100 mA
    //Interface Descriptor
    9,      // bLength: Interface Descriptor size
    USB_INTERFACE_DESCRIPTOR_TYPE,  // bDescriptorType: Interface
    // Interface descriptor type
    0,      // bInterfaceNumber: Number of Interface
    0,      // bAlternateSetting: Alternate setting
    1,      // bNumEndpoints: One endpoints used
    0x02,   // bInterfaceClass: Communication Interface Class
    0x02,   // bInterfaceSubClass: Abstract Control Model
    0x01,   // bInterfaceProtocol: Common AT commands
    0x00,   // iInterface:
    //Header Functional Descriptor
    5,      // bLength: Endpoint Descriptor size
    0x24,   // bDescriptorType: CS_INTERFACE
    0x00,   // bDescriptorSubtype: Header Func Desc
    0x10,0x01,   // bcdCDC: spec release number
    //Call Management Functional Descriptor
    5,      // bFunctionLength
    0x24,   // bDescriptorType: CS_INTERFACE
    0x01,   // bDescriptorSubtype: Call Management Func Desc
    0x00,   // bmCapabilities: D0+D1
    1,      // bDataInterface: 1
    //ACM Functional Descriptor
    4,      // bFunctionLength
    0x24,   // bDescriptorType: CS_INTERFACE
    0x02,   // bDescriptorSubtype: Abstract Control Management desc
    0x02,   // bmCapabilities
    //Union Functional Descriptor
    5,      // bFunctionLength
    0x24,   // bDescriptorType: CS_INTERFACE
    0x06,   // bDescriptorSubtype: Union func desc
    0x00,   // bMasterInterface: Communication class interface
    0x01,   // bSlaveInterface0: Data Class Interface
    //Endpoint 2 Descriptor
    7,      // bLength: Endpoint Descriptor size
    USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType: Endpoint
    0x82,   // bEndpointAddress: (IN2)
    0x03,   // bmAttributes: Interrupt
    VCP_INT_SIZE,      // wMaxPacketSize:
    0x00,
    0xff,   // bInterval:
    //Data class interface descriptor
    9,      // bLength: Endpoint Descriptor size
    USB_INTERFACE_DESCRIPTOR_TYPE,  // bDescriptorType:
    1,      // bInterfaceNumber: Number of Interface
    0,      // bAlternateSetting: Alternate setting
    2,      // bNumEndpoints: Two endpoints used
    0x0A,   // bInterfaceClass: CDC
    0x00,   // bInterfaceSubClass:
    0x00,   // bInterfaceProtocol:
    0x00,   // iInterface:
    //Endpoint 3 Descriptor
    7,      // bLength: Endpoint Descriptor size
    USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType: Endpoint
    0x03,   // bEndpointAddress: (OUT3)
    0x02,   // bmAttributes: Bulk
    VCP_DATA_SIZE,             // wMaxPacketSize:
    0x00,
    0x00,   // bInterval: ignore for Bulk transfer
    //Endpoint 1 Descriptor
    7,      // bLength: Endpoint Descriptor size
    USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType: Endpoint
    0x81,   // bEndpointAddress: (IN1)
    0x02,   // bmAttributes: Bulk
    VCP_DATA_SIZE,             // wMaxPacketSize:
    0x00,
    0x00    // bInterval
  };

// USB String Descriptors
const uint8_t StringLangID[4] =
  {
    sizeof(StringLangID),
    USB_STRING_DESCRIPTOR_TYPE,
    0x09,
    0x04 // LangID = 0x0409: U.S. English
  };

const uint8_t StringVendor[38] =
  {
    sizeof(StringVendor),     // Size of Vendor string
    USB_STRING_DESCRIPTOR_TYPE,             // bDescriptorType
    // Manufacturer: "STMicroelectronics"
    'S', 0, 'T', 0, 'M', 0, 'i', 0, 'c', 0, 'r', 0, 'o', 0, 'e', 0,
    'l', 0, 'e', 0, 'c', 0, 't', 0, 'r', 0, 'o', 0, 'n', 0, 'i', 0,
    'c', 0, 's', 0
  };

const uint8_t StringProduct[50] =
  {
    sizeof(StringProduct),          // bLength
    USB_STRING_DESCRIPTOR_TYPE,        // bDescriptorType
    // Product name: "STM32 Virtual COM Port"
    'S', 0, 'T', 0, 'M', 0, '3', 0, '2', 0, ' ', 0, 'V', 0, 'i', 0,
    'r', 0, 't', 0, 'u', 0, 'a', 0, 'l', 0, ' ', 0, 'C', 0, 'O', 0,
    'M', 0, ' ', 0, 'P', 0, 'o', 0, 'r', 0, 't', 0, ' ', 0, ' ', 0
  };

uint8_t StringSerial[26] =
  {
    sizeof(StringSerial),           // bLength
    USB_STRING_DESCRIPTOR_TYPE,                   // bDescriptorType
    'S', 0, 'T', 0, 'M', 0, '3', 0, '2', 0
  };

// ------------------------------------------------------------------

ONE_DESCRIPTOR Device_Descriptor =
  {
    (uint8_t*)DeviceDescriptor,
    sizeof(DeviceDescriptor)
  };

ONE_DESCRIPTOR Config_Descriptor =
  {
    (uint8_t*)ConfigDescriptor,
    sizeof(ConfigDescriptor)
  };

ONE_DESCRIPTOR String_Descriptor[NUM_STRING_DESC] =
  {
    {(uint8_t*)StringLangID, sizeof(StringLangID)},
    {(uint8_t*)StringVendor, sizeof(StringVendor)},
    {(uint8_t*)StringProduct, sizeof(StringProduct)},
    {(uint8_t*)StringSerial, sizeof(StringSerial)}
  };
