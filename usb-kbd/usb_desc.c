
#include "usb_lib.h"
#include "usb_desc.h"

// HID Report Descriptor
const uint8_t HidReportDescriptor[46] =
  {
0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
0x09, 0x06,        // Usage (Keyboard)
0xA1, 0x01,        // Collection (Application)
0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
0x19, 0xE0,        //   Usage Minimum (0xE0)
0x29, 0xE7,        //   Usage Maximum (0xE7)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x01,        //   Logical Maximum (1)
0x75, 0x01,        //   Report Size (1)
0x95, 0x08,        //   Report Count (8)
0x81, 0x02,        //   Input (Data,Var,Abs)
0x95, 0x01,        //   Report Count (1)
0x75, 0x08,        //   Report Size (8)
0x81, 0x01,        //   Input (Const,Array)
0x95, 0x0e,        //   Report Count (14)
0x75, 0x08,        //   Report Size (8)
0x15, 0x00,        //   Logical Minimum (0)
0x26, 0xA4, 0x00,  //   Logical Maximum (164)
0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
0x19, 0x00,        //   Usage Minimum (0x00)
0x29, 0xA4,        //   Usage Maximum (0xA4)
0x81, 0x00,        //   Input (Data,Array,Abs)
0xC0               // End Collection
  };

// USB Standard Device Descriptor
const uint8_t DeviceDescriptor[] =
  {
    18,     // bLength
    USB_DEVICE_DESCRIPTOR_TYPE,     // bDescriptorType
    0x10,0x01,   // bcdUSB = 2.00
    0x00,   // bDeviceClass: defined at interface level
    0x00,   // bDeviceSubClass
    0x00,   // bDeviceProtocol
    EP0_DATA_SIZE,   // bMaxPacketSize0
    0x83,0x04,   // idVendor = 0x0483
    0x4c,0x5a,   // idProduct
    0x00,0x01,   // bcdDevice = 1.00
    1,      // Index of string descriptor describing manufacturer
    2,      // Index of string descriptor describing product
    0,      // Index of string descriptor describing the device's serial number
    1       // bNumConfigurations
  };

// USB Standard Configuration Descriptor
const uint8_t ConfigDescriptor[34] =
  {
    //Configuration Descriptor
    9,      // bLength: Configuration Descriptor size
    USB_CONFIGURATION_DESCRIPTOR_TYPE,      // bDescriptorType: Configuration
    sizeof(ConfigDescriptor),0x00,       // wTotalLength:no of returned bytes
    1,      // bNumInterfaces:
    1,      // bConfigurationValue: Configuration value
    0,      // iConfiguration: Index of string descriptor describing the configuration
    0x80,   // bmAttributes:
    50,     // MaxPower in 2 mA units
    //Interface Descriptor
    9,      // bLength: Interface Descriptor size
    USB_INTERFACE_DESCRIPTOR_TYPE,  // bDescriptorType: Interface
    // Interface descriptor type
    0,      // bInterfaceNumber: Number of Interface
    0,      // bAlternateSetting: Alternate setting
    1,      // bNumEndpoints: One endpoint used
    3,      // bInterfaceClass: HID
    0,      // bInterfaceSubClass: 1 = boot
    0,      // bInterfaceProtocol: 0 = nonstd, 1 = keyb, 2 = mouse, ...
    0,      // iInterface:
	//HID descriptor (shall be interleaved between interface and endpoint desc)
	9,      // bLength
	USB_HID_DESCRIPTOR_TYPE,	// bDescriptorType: HID
	0x10,0x01,	// bcdHID: 1.10
	0,	    // bCountryCode
	1,      // bNumDescriptors
	USB_REPORT_DESCRIPTOR_TYPE,	// bDescriptorType
	sizeof(HidReportDescriptor),0x00, // wDescriptorLength:
    //Endpoint 1 Descriptor
    7,      // bLength: Endpoint Descriptor size
    USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType: Endpoint
    0x81,   // bEndpointAddress: (IN1)
    0x03,   // bmAttributes: Interrupt
    EP1_DATA_SIZE,0x00,      // wMaxPacketSize:
    10      // bInterval: in 1ms units
  };

// USB String Descriptors
const uint8_t StringLangID[4] =
  {
    sizeof(StringLangID),
    USB_STRING_DESCRIPTOR_TYPE,
    0x09,0x04 // LangID = 0x0409: U.S. English
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

const uint8_t StringProduct[38] =
  {
    sizeof(StringProduct),          // bLength
    USB_STRING_DESCRIPTOR_TYPE,        // bDescriptorType
    // Product name: "STM32 Virtual COM Port"
    'S', 0, 'T', 0, 'M', 0, '3', 0, '2', 0, ' ', 0, 'H', 0, 'I', 0,
    'D', 0, ' ', 0, 'K', 0, 'e', 0, 'y', 0, 'b', 0, 'o', 0, 'a', 0,
	'r', 0, 'd', 0
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

ONE_DESCRIPTOR HID_Report_Descriptor =
  {
    (uint8_t*)HidReportDescriptor,
    sizeof(HidReportDescriptor)
  };

ONE_DESCRIPTOR HID_Hid_Descriptor =
  {
    (uint8_t*)ConfigDescriptor + 9 + 9, // hid is after config and interface
    9
  };

ONE_DESCRIPTOR String_Descriptor[NUM_STRING_DESC] =
  {
    {(uint8_t*)StringLangID, sizeof(StringLangID)},
    {(uint8_t*)StringVendor, sizeof(StringVendor)},
    {(uint8_t*)StringProduct, sizeof(StringProduct)},
  };
