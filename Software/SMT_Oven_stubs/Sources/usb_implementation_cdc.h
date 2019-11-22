/**
 * @file     usb_implementation_cdc.h
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
#ifndef PROJECT_HEADERS_USB_IMPLEMENTATION_CDC_H_
#define PROJECT_HEADERS_USB_IMPLEMENTATION_CDC_H_

/*
 * Under Windows 8, or 10 there is no need to install a driver for
 * the bulk end-points if the MS_COMPATIBLE_ID_FEATURE is enabled.
 * winusb.sys driver will be automatically loaded.
 *
 * Under Windows 10 the usbser.sys driver will be loaded automatically
 * for the CDC (serial) interface
 *
 * Under Linux drivers for bulk and CDC are automatically loaded
 */
//#define MS_COMPATIBLE_ID_FEATURE
#include "RemoteInterface.h"

#define UNIQUE_ID
//#include "configure.h"

namespace USBDM {

//======================================================================
// Customise for each USB device
//

/** Causes a semi-unique serial number to be generated for each USB device */
//#define UNIQUE_ID


#ifdef UNIQUE_ID
#define SERIAL_NO             "SMT-OVEN-%lu"
#else
#define SERIAL_NO             "SMT-OVEN-0001"
#endif
#define PRODUCT_DESCRIPTION   "SMT-Oven"
#define MANUFACTURER          "pgo"

#define VENDOR_ID             (0x16D0)    // Vendor (actually MCS)
#define PRODUCT_ID            (0x8888)    // Product ID
#define VERSION_ID            (1)         // Reported version (via USB)

//======================================================================
// Maximum packet sizes for each endpoint
//
static constexpr uint  CONTROL_EP_MAXSIZE           = 64; //!< Control in/out
/*
 *  TODO Define additional end-point sizes
 */
static constexpr uint  CDC_NOTIFICATION_EP_MAXSIZE  = 16; //!< CDC notification
static constexpr uint  CDC_DATA_OUT_EP_MAXSIZE      = 16; //!< CDC data out
static constexpr uint  CDC_DATA_IN_EP_MAXSIZE       = 16; //!< CDC data in

#ifdef USBDM_USB0_IS_DEFINED
/**
 * Class representing USB0
 */
class Usb0 : public UsbBase_T<Usb0Info, CONTROL_EP_MAXSIZE> {

   friend UsbBase_T<Usb0Info, CONTROL_EP_MAXSIZE>;

public:
   /**
    * String indexes
    *
    * Must agree with stringDescriptors[] order
    */
   enum StringIds {
      /** Language information for string descriptors */
      s_language_index=0,    // Must be zero
      /** Manufacturer */
      s_manufacturer_index,
      /** Product Description */
      s_product_index,
      /** Serial Number */
      s_serial_index,
      /** Configuration Index */
      s_config_index,

      /** Name of CDC interface */
      s_cdc_interface_index,
      /** CDC Control Interface */
      s_cdc_control_interface_index,
      /** CDC Data Interface */
      s_cdc_data_Interface_index,
      /*
       * TODO Add additional String indexes
       */

      /** Marks last entry */
      s_number_of_string_descriptors
   };

   /**
    * Endpoint numbers\n
    * Must be consecutive
    */
   enum EndpointNumbers {
      /** USB Control endpoint number - must be zero */
      CONTROL_ENDPOINT  = 0,

      /* end-points are assumed consecutive */

      /** CDC Control endpoint number */
      CDC_NOTIFICATION_ENDPOINT,
      /** CDC Data out endpoint number */
      CDC_DATA_OUT_ENDPOINT,
      /** CDC Data in endpoint number */
      CDC_DATA_IN_ENDPOINT,

      /*
       * TODO Add additional Endpoint numbers here
       */
      /** Total number of end-points */
      NUMBER_OF_ENDPOINTS,
   };

   /**
    * Configuration numbers, consecutive from 1
    */
   enum Configurations {
      CONFIGURATION_NUM = 1,
      /*
       * Assumes single configuration
       */
      /** Total number of configurations */
      NUMBER_OF_CONFIGURATIONS = CONFIGURATION_NUM,
   };

   /**
    * String descriptor table
    */
   static const uint8_t *const stringDescriptors[];

protected:
   /* end-points */
   /** In end-point for CDC notifications */
   static InEndpoint  <Usb0Info, Usb0::CDC_NOTIFICATION_ENDPOINT, CDC_NOTIFICATION_EP_MAXSIZE>  epCdcNotification;
   
   /** Out end-point for CDC data out */
   static OutEndpoint <Usb0Info, Usb0::CDC_DATA_OUT_ENDPOINT,     CDC_DATA_OUT_EP_MAXSIZE>      epCdcDataOut;
   
   /** In end-point for CDC data in */
   static InEndpoint  <Usb0Info, Usb0::CDC_DATA_IN_ENDPOINT,      CDC_DATA_IN_EP_MAXSIZE>       epCdcDataIn;
   /*
    * TODO Add additional End-points here
    */
	
   using cdcInterface = RemoteInterface;
   static cdcInterface::Response  *response;

public:

   /**
    * Initialise the USB0 interface
    *
    *  @note Assumes clock set up for USB operation (48MHz)
    */
   static void initialise();

   /**
    * CDC Transmit
    *
    * @param data Pointer to data to transmit
    * @param size Number of bytes to transmit
    */
   static void sendCdcData(const uint8_t *data, unsigned size);

   /**
    * CDC Receive
    *
    * @param data    Pointer to data to receive
    * @param maxSize Maximum number of bytes to receive
    *
    * @return Number of bytes received
    */
   static int receiveCdcData(uint8_t *data, unsigned maxSize);

   /**
    * Notify IN (device->host) endpoint that data is available
    *
    * @return Not used
    */
   static bool notify();

   /**
    * Device Descriptor
    */
   static const DeviceDescriptor deviceDescriptor;

   /**
    * Other descriptors type
    */
   struct Descriptors {
      ConfigurationDescriptor                  configDescriptor;

      InterfaceDescriptor                      cdc_CCI_Interface;
      CDCHeaderFunctionalDescriptor            cdc_Functional_Header;
      CDCCallManagementFunctionalDescriptor    cdc_CallManagement;
      CDCAbstractControlManagementDescriptor   cdc_Functional_ACM;
      CDCUnionFunctionalDescriptor             cdc_Functional_Union;
      EndpointDescriptor                       cdc_notification_Endpoint;

      InterfaceDescriptor                      cdc_DCI_Interface;
      EndpointDescriptor                       cdc_dataOut_Endpoint;
      EndpointDescriptor                       cdc_dataIn_Endpoint;
      /*
       * TODO Add additional Descriptors here
       */
   };

   /**
    * All other descriptors
    */
   static const Descriptors otherDescriptors;

protected:
   /**
    * Initialises all end-points
    */
   static void initialiseEndpoints(void) {
      epCdcNotification.initialise();
      addEndpoint(&epCdcNotification);

      epCdcDataOut.initialise();
      addEndpoint(&epCdcDataOut);
      epCdcDataOut.setCallback(cdcOutTransactionCallback);

      // Make sure epCdcDataOut is ready for polling (OUT)
      epCdcDataOut.startRxTransaction(EPDataOut, epCdcDataOut.BUFFER_SIZE);

      epCdcDataIn.initialise();
      addEndpoint(&epCdcDataIn);
      epCdcDataIn.setCallback(cdcInTransactionCallback);

      // Start CDC status transmission
      epCdcSendNotification();

      // Connect notify callback
      cdcInterface::setUsbInNotifyCallback(notify);
      /*
       * TODO Initialise additional End-points here
       */
   }

   /**
    * Callback for SOF tokens
    */
   static ErrorCode sofCallback();

   /**
    * Call-back handling CDC-IN transaction complete\n
    * Checks for data and schedules transfer as necessary\n
    * Each transfer will have a ZLP as necessary.
    *
    * @param state Current end-point state
    */
   static void cdcInTransactionCallback(EndpointState state);

   /**
    * Call-back handling CDC-OUT transaction complete\n
    * Data received is passed to the cdcInterface
    *
    * @param state Current end-point state
    */
   static void cdcOutTransactionCallback(EndpointState state);

   /**
    * Handler for Token Complete USB interrupts for\n
    * end-points other than EP0
    */
   static void handleTokenComplete(void);

   /**
    * Start CDC IN transaction\n
    * A packet is only sent if data is available
    */
   static void startCdcIn();

   /**
    * Configure epCdcNotification for an IN transaction [Tx, device -> host, DATA0/1]
    */
   static void epCdcSendNotification();

   /**
    * Handle SETUP requests not handled by base handler
    *
    * @param setup SETUP packet received from host
    *
    * @note Provides CDC extensions
    */
   static ErrorCode handleUserEp0SetupRequests(const SetupPacket &setup);

   /**
    * CDC Set line coding handler
    */
   static void handleSetLineCoding();

   /**
    * CDC Get line coding handler
    */
   static void handleGetLineCoding();

   /**
    * CDC Set line state handler
    */
   static void handleSetControlLineState();

   /**
    * CDC Send break handler
    */
   static void handleSendBreak();

};

using UsbImplementation = Usb0;

#endif // USBDM_USB0_IS_DEFINED

} // End namespace USBDM

#endif /* PROJECT_HEADERS_USB_IMPLEMENTATION_CDC_H_ */
