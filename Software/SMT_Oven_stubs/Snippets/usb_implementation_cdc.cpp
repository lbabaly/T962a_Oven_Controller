/**
 * @file     usb_implementation_cdc.cpp
 * @brief    USB CDC device implementation
 *
 * This module provides an implementation of a USB Composite interface
 * including the following end points:
 *  - EP0 Standard control
 *  - EP1 Interrupt CDC notification
 *  - EP2 CDC data OUT
 *  - EP3 CDC data IN
 *
 * @version  V4.12.1.170
 * @date     2 April 2017
 *
 *  This file provides the implementation specific code for the USB interface.
 *  It will need to be modified to suit an application.
 */
#include <string.h>

#include "usb.h"
#include "usb_implementation_cdc.h"

namespace USBDM {

/**
 * Interface numbers for USB descriptors
 */
enum InterfaceNumbers {
   /** Interface number for CDC Control channel */
   CDC_COMM_INTF_ID,
   /** Interface number for CDC Data channel */
   CDC_DATA_INTF_ID,
   /*
    * TODO Add additional Interface numbers here
    */
   /** Total number of interfaces */
   NUMBER_OF_INTERFACES,
};

/*
 * String descriptors
 */
static const uint8_t s_language[]        = {4, DT_STRING, 0x09, 0x0C};  //!< Language IDs
static const uint8_t s_manufacturer[]    = MANUFACTURER;                //!< Manufacturer
static const uint8_t s_product[]         = PRODUCT_DESCRIPTION;         //!< Product Description
static const uint8_t s_serial[]          = SERIAL_NO;                   //!< Serial Number
static const uint8_t s_config[]          = "Default configuration";     //!< Configuration name

static const uint8_t s_cdc_interface[]   = "CDC Interface";             //!< Interface Association #2
static const uint8_t s_cdc_control[]     = "CDC Control Interface";     //!< CDC Control Interface
static const uint8_t s_cdc_data[]        = "CDC Data Interface";        //!< CDC Data Interface
/*
 * Add additional String descriptors here
 */

/**
 * String descriptor table
 */
const uint8_t *const Usb0::stringDescriptors[] {
      s_language,
      s_manufacturer,
      s_product,
      s_serial,
      s_config,

      s_cdc_interface,
      s_cdc_control,
      s_cdc_data
      /*
       * Add additional String descriptors here
       */
};

/**
 * Device Descriptor
 */
const DeviceDescriptor Usb0::deviceDescriptor {
      /* bLength             */ sizeof(DeviceDescriptor),
      /* bDescriptorType     */ DT_DEVICE,
      /* bcdUSB              */ nativeToLe16(0x0200),           // USB specification release No. [BCD = 2.00]
      /* bDeviceClass        */ 0x02,                           // Device Class code [CDC Device Class]
      /* bDeviceSubClass     */ 0x00,                           // Sub Class code    [none]
      /* bDeviceProtocol     */ 0x00,                           // Protocol          [none]
      /* bMaxPacketSize0     */ CONTROL_EP_MAXSIZE,             // EndPt 0 max packet size
      /* idVendor            */ nativeToLe16(VENDOR_ID),        // Vendor ID
      /* idProduct           */ nativeToLe16(PRODUCT_ID),       // Product ID
      /* bcdDevice           */ nativeToLe16(VERSION_ID),       // Device Release    [BCD = 4.10]
      /* iManufacturer       */ s_manufacturer_index,           // String index of Manufacturer name
      /* iProduct            */ s_product_index,                // String index of product description
      /* iSerialNumber       */ s_serial_index,                 // String index of serial number
      /* bNumConfigurations  */ NUMBER_OF_CONFIGURATIONS        // Number of configurations
};

/**
 * All other descriptors
 */
const Usb0::Descriptors Usb0::otherDescriptors {
      { // configDescriptor
            /* bLength                 */ sizeof(ConfigurationDescriptor),
            /* bDescriptorType         */ DT_CONFIGURATION,
            /* wTotalLength            */ nativeToLe16(sizeof(otherDescriptors)),
            /* bNumInterfaces          */ NUMBER_OF_INTERFACES,
            /* bConfigurationValue     */ CONFIGURATION_NUM,
            /* iConfiguration          */ s_config_index,
            /* bmAttributes            */ 0x80,     //  = Bus powered, no wake-up
            /* bMaxPower               */ USBMilliamps(500)
      },
      /**
       * CDC Control/Communication Interface, 1 end-point
       */
      { // cdc_CCI_Interface
            /* bLength                 */ sizeof(InterfaceDescriptor),
            /* bDescriptorType         */ DT_INTERFACE,
            /* bInterfaceNumber        */ CDC_COMM_INTF_ID,
            /* bAlternateSetting       */ 0,
            /* bNumEndpoints           */ 1,
            /* bInterfaceClass         */ 0x02,      //  CDC Communication
            /* bInterfaceSubClass      */ 0x02,      //  Abstract Control Model
            /* bInterfaceProtocol      */ 0x01,      //  V.25ter, AT Command V.250
            /* iInterface description  */ s_cdc_control_interface_index
      },
      { // cdc_Functional_Header
            /* bFunctionalLength       */ sizeof(CDCHeaderFunctionalDescriptor),
            /* bDescriptorType         */ CS_INTERFACE,
            /* bDescriptorSubtype      */ DST_HEADER,
            /* bcdCDC                  */ nativeToLe16(0x0110),
      },
      { // cdc_CallManagement
            /* bFunctionalLength       */ sizeof(CDCCallManagementFunctionalDescriptor),
            /* bDescriptorType         */ CS_INTERFACE,
            /* bDescriptorSubtype      */ DST_CALL_MANAGEMENT,
            /* bmCapabilities          */ 1,
            /* bDataInterface          */ CDC_DATA_INTF_ID,
      },
      { // cdc_Functional_ACM
            /* bFunctionalLength       */ sizeof(CDCAbstractControlManagementDescriptor),
            /* bDescriptorType         */ CS_INTERFACE,
            /* bDescriptorSubtype      */ DST_ABSTRACT_CONTROL_MANAGEMENT,
            /* bmCapabilities          */ 0x06,
      },
      { // cdc_Functional_Union
            /* bFunctionalLength       */ sizeof(CDCUnionFunctionalDescriptor),
            /* bDescriptorType         */ CS_INTERFACE,
            /* bDescriptorSubtype      */ DST_UNION_MANAGEMENT,
            /* bmControlInterface      */ CDC_COMM_INTF_ID,
            /* bSubordinateInterface0  */ {CDC_DATA_INTF_ID},
      },
      { // cdc_notification_Endpoint - IN,interrupt
            /* bLength                 */ sizeof(EndpointDescriptor),
            /* bDescriptorType         */ DT_ENDPOINT,
            /* bEndpointAddress        */ EP_IN|CDC_NOTIFICATION_ENDPOINT,
            /* bmAttributes            */ ATTR_INTERRUPT,
            /* wMaxPacketSize          */ nativeToLe16(CDC_NOTIFICATION_EP_MAXSIZE),
            /* bInterval               */ USBMilliseconds(255)
      },
      /**
       * CDC Data Interface, 2 end-points
       */
      { // cdc_DCI_Interface
            /* bLength                 */ sizeof(InterfaceDescriptor),
            /* bDescriptorType         */ DT_INTERFACE,
            /* bInterfaceNumber        */ CDC_DATA_INTF_ID,
            /* bAlternateSetting       */ 0,
            /* bNumEndpoints           */ 2,
            /* bInterfaceClass         */ 0x0A,                         //  CDC DATA
            /* bInterfaceSubClass      */ 0x00,                         //  -
            /* bInterfaceProtocol      */ 0x00,                         //  -
            /* iInterface description  */ s_cdc_data_Interface_index
      },
      { // cdc_dataOut_Endpoint - OUT, Bulk
            /* bLength                 */ sizeof(EndpointDescriptor),
            /* bDescriptorType         */ DT_ENDPOINT,
            /* bEndpointAddress        */ EP_OUT|CDC_DATA_OUT_ENDPOINT,
            /* bmAttributes            */ ATTR_BULK,
            /* wMaxPacketSize          */ nativeToLe16(CDC_DATA_OUT_EP_MAXSIZE),
            /* bInterval               */ USBMilliseconds(1)
      },
      { // cdc_dataIn_Endpoint - IN, Bulk
            /* bLength                 */ sizeof(EndpointDescriptor),
            /* bDescriptorType         */ DT_ENDPOINT,
            /* bEndpointAddress        */ EP_IN|CDC_DATA_IN_ENDPOINT,
            /* bmAttributes            */ ATTR_BULK,
            /* wMaxPacketSize          */ nativeToLe16(CDC_DATA_IN_EP_MAXSIZE),
            /* bInterval               */ USBMilliseconds(1)
      },
      /*
       * TODO Add additional Descriptors here
       */
};

/** In end-point for CDC notifications */
InEndpoint  <Usb0Info, Usb0::CDC_NOTIFICATION_ENDPOINT, CDC_NOTIFICATION_EP_MAXSIZE>  Usb0::epCdcNotification;

/** Out end-point for CDC data out */
OutEndpoint <Usb0Info, Usb0::CDC_DATA_OUT_ENDPOINT,     CDC_DATA_OUT_EP_MAXSIZE>      Usb0::epCdcDataOut;

/** In end-point for CDC data in */
InEndpoint  <Usb0Info, Usb0::CDC_DATA_IN_ENDPOINT,      CDC_DATA_IN_EP_MAXSIZE>       Usb0::epCdcDataIn;
/*
 * TODO Add additional end-points here
 */

/**
 * Handler for Start of Frame Token interrupt (~1ms interval)
 */
ErrorCode Usb0::sofCallback() {
   // Activity LED
   // Off                     - no USB activity, not connected
   // On                      - no USB activity, connected
   // Off, flash briefly on   - USB activity, not connected
   // On,  flash briefly off  - USB activity, connected
   if (usb->FRMNUML==0) { // Every ~256 ms
      switch (usb->FRMNUMH&0x03) {
         case 0:
            if (connectionState == USBconfigured) {
               // Activity LED on when USB connection established
//               UsbLed::on();
            }
            else {
               // Activity LED off when no USB connection
//               UsbLed::off();
            }
            break;
         case 1:
         case 2:
            break;
         case 3:
         default :
            if (activityFlag) {
               // Activity LED flashes
//               UsbLed::toggle();
               setActive(false);
            }
            break;
      }
   }
   // Check CDC status
   epCdcSendNotification();
   return E_NO_ERROR;
}

/**
 * Configure epCdcNotification for an IN status transaction [Tx, device -> host, DATA0/1]\n
 * A packet is only sent if there has been a change in status
 */
void Usb0::epCdcSendNotification() {
   const CDCNotification cdcNotification= {CDC_NOTIFICATION, SERIAL_STATE, 0, RT_INTERFACE, nativeToLe16(2)};
   static uint8_t lastStatus = -1;
   uint8_t status = cdcInterface::getSerialState().bits;

   if (status == lastStatus) {
      // No change
      return;
   }
   if (epCdcNotification.getState() != EPIdle) {
      // Busy with previous
      return;
   }
   static_assert(epCdcNotification.BUFFER_SIZE>=sizeof(CDCNotification), "Buffer size insufficient");

   lastStatus = status;

   // Copy the data to Tx buffer
   (void)memcpy(epCdcNotification.getBuffer(), &cdcNotification, sizeof(cdcNotification));
   epCdcNotification.getBuffer()[sizeof(cdcNotification)+0] = status;
   epCdcNotification.getBuffer()[sizeof(cdcNotification)+1] = 0;

   // Set up to Tx packet
//   PRINTF("epCdcSendNotification(0x%2X)\n", epCdcNotification.getBuffer()[sizeof(cdcNotification)+0]);
   epCdcNotification.startTxTransaction(EPDataIn, sizeof(cdcNotification)+2);
}

static uint8_t cdcOutBuff[10] = "Welcome\n";
static int cdcOutByteCount    = 8;

/**
 * Start CDC IN transaction\n
 * A packet is only sent if data is available
 */
void Usb0::startCdcIn() {
   if ((epCdcDataIn.getState() == EPIdle) && (cdcOutByteCount>0)) {
      static_assert(epCdcDataIn.BUFFER_SIZE>sizeof(cdcOutBuff), "Buffer too small");
      memcpy(epCdcDataIn.getBuffer(), cdcOutBuff, cdcOutByteCount);
      epCdcDataIn.setNeedZLP();
      epCdcDataIn.startTxTransaction(EPDataIn, cdcOutByteCount);
      cdcOutByteCount = 0;
   }
}
/**
 * Handler for Token Complete USB interrupts for
 * end-points other than EP0
 */
void Usb0::handleTokenComplete() {

   // Status from Token
   uint8_t   usbStat  = usb->STAT;

   // Endpoint number
   uint8_t   endPoint = ((uint8_t)usbStat)>>4;

   endPoints[endPoint]->flipOddEven(usbStat);
   switch (endPoint) {
      case CDC_NOTIFICATION_ENDPOINT: // Accept IN token
//         PRINTF("CDC_NOTIFICATION_ENDPOINT\n");
         epCdcSendNotification();
         return;
      case CDC_DATA_OUT_ENDPOINT: // Accept OUT token
//         PRINTF("CDC_DATA_OUT_ENDPOINT\n");
         epCdcDataOut.handleOutToken();
         return;
      case CDC_DATA_IN_ENDPOINT:  // Accept IN token
//         PRINTF("CDC_DATA_IN_ENDPOINT\n");
         epCdcDataIn.handleInToken();
         return;
      /*
       * TODO Add additional End-point handling here
       */
   }
}

/**
 * Call-back handling CDC-OUT transaction complete\n
 * Data received is passed to the cdcInterface
 *
 * @param state Current end-point state
 */
void Usb0::cdcOutTransactionCallback(EndpointState state) {
   //   PRINTF("cdc_out\n");
   if (state == EPDataOut) {
      cdcInterface::putData(epCdcDataOut.getDataTransferredSize(), epCdcDataOut.getBuffer());
   }
   // Set up for next transfer
   epCdcDataOut.startRxTransaction(EPDataOut, epCdcDataOut.BUFFER_SIZE);
}

/**
 * Call-back handling CDC-IN transaction complete\n
 * Checks for data and schedules transfer as necessary\n
 * Each transfer will have a ZLP as necessary.
 *
 * @param state Current end-point state
 */
void Usb0::cdcInTransactionCallback(EndpointState state) {
   static uint8_t buffer[20];

   if ((state == EPDataIn)||(state == EPIdle)) {
      int size = cdcInterface::getData(sizeof(buffer), buffer);
      if (size != 0) {
         // Schedules transfer
         epCdcDataIn.setNeedZLP();
         epCdcDataIn.startTxTransaction(EPDataIn, size, buffer);
      }
   }
}

/**
 * Notify IN (device->host) endpoint that data is available
 *
 * @return Not used
 */
bool Usb0::notify() {
   if (epCdcDataIn.getState() == EPIdle) {
      // Have to restart IN transactions
      cdcInTransactionCallback(EPDataIn);
   }
   return true;
}

/**
 * Initialise the USB0 interface
 *
 *  @note Assumes clock set up for USB operation (48MHz)
 */
void Usb0::initialise() {
   // Add extra handling of CDC packets directed to EP0
   setUnhandledSetupCallback(handleUserEp0SetupRequests);

   setSOFCallback(sofCallback);

   UsbBase_T::initialise();

   cdcInterface::setUsbInNotifyCallback(notify);
   cdcInterface::initialise();
   /*
    * TODO Additional initialisation
    */
}

/**
 * CDC Set line coding handler
 */
void Usb0::handleSetLineCoding() {
//   PRINTF("handleSetLineCoding()\n");
   static LineCodingStructure lineCoding;

   // Call-back to do after transaction complete
   static auto callback = []() {
      // The controlEndpoint buffer will contain the LineCodingStructure data at call-back time
      cdcInterface::setLineCoding(&lineCoding);
      setSetupCompleteCallback(nullptr);
   };
   setSetupCompleteCallback(callback);

   // Don't use external buffer - this requires response to fit in internal EP buffer
   static_assert(sizeof(LineCodingStructure) < controlEndpoint.BUFFER_SIZE, "Buffer insufficient size");
   controlEndpoint.startRxTransaction(EPDataOut, sizeof(LineCodingStructure), (uint8_t*)&lineCoding);
}

/**
 * CDC Get line coding handler
 */
void Usb0::handleGetLineCoding() {
//   PRINTF("handleGetLineCoding()\n");
   // Send packet
   ep0StartTxTransaction( sizeof(LineCodingStructure), (const uint8_t*)&cdcInterface::getLineCoding());
}

/**
 * CDC Set line state handler
 */
void Usb0::handleSetControlLineState() {
//   PRINTF("handleSetControlLineState(%X)\n", ep0SetupBuffer.wValue.lo());
   cdcInterface::setControlLineState(ep0SetupBuffer.wValue.lo());
   // Tx empty Status packet
   ep0StartTxTransaction( 0, nullptr );
}

/**
 * CDC Send break handler
 */
void Usb0::handleSendBreak() {
//   PRINTF("handleSendBreak()\n");
   cdcInterface::sendBreak(ep0SetupBuffer.wValue);
   // Tx empty Status packet
   ep0StartTxTransaction( 0, nullptr );
}

/**
 * Handle SETUP requests not handled by base handler
 *
 * @param setup SETUP packet received from host
 *
 * @note Provides CDC extensions
 */
ErrorCode Usb0::handleUserEp0SetupRequests(const SetupPacket &setup) {
   //PRINTF("handleUserEp0SetupRequests()\n");
   switch(REQ_TYPE(setup.bmRequestType)) {
      case REQ_TYPE_CLASS :
         // Class requests
         switch (setup.bRequest) {
            case SET_LINE_CODING :       handleSetLineCoding();       break;
            case GET_LINE_CODING :       handleGetLineCoding();       break;
            case SET_CONTROL_LINE_STATE: handleSetControlLineState(); break;
            case SEND_BREAK:             handleSendBreak();           break;
            default : return E_NO_HANDLER;
         }
         break;
            default : return E_NO_HANDLER;
   }
   return E_NO_ERROR;
}

} // End namespace USBDM

