#ifndef _CONF_USB_H_
#define _CONF_USB_H_

#include "compiler.h"

#define  USB_DEVICE_VENDOR_ID             0x0456
#define  USB_DEVICE_PRODUCT_ID            0xCEE2
#define  USB_DEVICE_MAJOR_VERSION         1
#define  USB_DEVICE_MINOR_VERSION         0
#define  USB_DEVICE_POWER                 500 // Consumption on Vbus line (mA)
#define  USB_DEVICE_ATTR                  (USB_CONFIG_ATTR_BUS_POWERED)

#define  USB_DEVICE_MANUFACTURE_NAME      "Analog Devices, Inc."
#define  USB_DEVICE_PRODUCT_NAME          "M1000"
#define  USB_DEVICE_SERIAL_NAME
#define  USB_DEVICE_GET_SERIAL_NAME_POINTER serial_number
#define  USB_DEVICE_GET_SERIAL_NAME_LENGTH  33
extern uint8_t serial_number[];

#define  USB_DEVICE_HS_SUPPORT

#define  UDC_VBUS_EVENT(b_vbus_high)
#define  UDC_SOF_EVENT()                  main_sof_action()
#define  UDC_SUSPEND_EVENT()              main_suspend_action()
#define  UDC_RESUME_EVENT()               main_resume_action()

#define UDI_VENDOR_ENABLE_EXT()           main_vendor_enable()
#define UDI_VENDOR_DISABLE_EXT()          main_vendor_disable()

#define UDC_GET_EXTRA_STRING()            msft_string_handle()

// why would you want to handle interface control requests?
#define UDI_VENDOR_SETUP_OUT_RECEIVED()   false
#define UDI_VENDOR_SETUP_IN_RECEIVED()    false

// why would you not want to handle everything else?
#define USB_DEVICE_SPECIFIC_REQUEST()     main_setup_handle()

#define UDI_VENDOR_EPS_SIZE_INT_FS    0
#define UDI_VENDOR_EPS_SIZE_BULK_FS   64
#define UDI_VENDOR_EPS_SIZE_ISO_FS    0

#define UDI_VENDOR_EPS_SIZE_INT_HS    0
#define UDI_VENDOR_EPS_SIZE_BULK_HS  512
#define UDI_VENDOR_EPS_SIZE_ISO_HS    0

#define USB_VERSION USB_V2

/// Microsoft WCID descriptor
typedef struct USB_MicrosoftCompatibleDescriptor_Interface {
    uint8_t bFirstInterfaceNumber;
    uint8_t reserved1;
    uint8_t compatibleID[8];
    uint8_t subCompatibleID[8];
    uint8_t reserved2[6];
} __attribute__((packed)) USB_MicrosoftCompatibleDescriptor_Interface;

typedef struct USB_MicrosoftCompatibleDescriptor {
    uint32_t dwLength;
    uint16_t bcdVersion;
    uint16_t wIndex;
    uint8_t bCount;
    uint8_t reserved[7];
    USB_MicrosoftCompatibleDescriptor_Interface interfaces[];
} __attribute__((packed)) USB_MicrosoftCompatibleDescriptor;

#include <udi_vendor_conf.h>
#include "main.h"

#endif // _CONF_USB_H_
