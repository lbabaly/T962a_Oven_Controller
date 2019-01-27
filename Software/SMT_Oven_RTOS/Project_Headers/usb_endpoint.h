/**
 * @file     usb_endpoint.h (180.ARM_Peripherals/Project_Headers/usb_endpoint.h)
 * @brief    Universal Serial Bus Endpoint
 *
 * @version  V4.12.1.210
 * @date     13 April 2016
 *      Author: podonoghue
 */

#ifndef HEADER_USB_ENDPOINT_H
#define HEADER_USB_ENDPOINT_H
/*
 * *****************************
 * *** DO NOT EDIT THIS FILE ***
 * *****************************
 *
 * This file is generated automatically.
 * Any manual changes will be lost.
 */
#include "usb_defs.h"
#include "derivative.h"

namespace USBDM {

/**
 * @addtogroup USB_Group USB, USB OTG Controller
 * @brief Abstraction for USB OTG Controller
 * @{
 */

/** BDTs organised by endpoint, odd/even, transmit/receive */
extern EndpointBdtEntry endPointBdts[];

/** BDTs as simple array */
constexpr BdtEntry *bdts() { return (BdtEntry *)endPointBdts; }

/** Endpoint state values */
enum EndpointState {
   EPIdle = 0,  //!< Idle
   EPDataIn,    //!< Doing a sequence of IN packets
   EPDataOut,   //!< Doing a sequence of OUT packets
   EPStatusIn,  //!< Doing an IN packet as a status handshake
   EPStatusOut, //!< Doing an OUT packet as a status handshake
   EPThrottle,  //!< Doing OUT packets but no buffers available (NAKed)
   EPStall,     //!< Endpoint is stalled
   EPComplete,  //!< Used for command protocol - new command available
};

/**
 * Class for generic endpoint
 */
class Endpoint {

public:
   /**
    * Get name of USB state
    *
    * @param[in]  state USB state
    *
    * @return Pointer to static string
    */
   static const char *getStateName(EndpointState state){
      switch (state) {
         default         : return "Unknown";
         case EPIdle     : return "EPIdle";
         case EPDataIn   : return "EPDataIn";
         case EPDataOut  : return "EPDataOut,";
         case EPStatusIn : return "EPStatusIn";
         case EPStatusOut: return "EPStatusOut";
         case EPThrottle : return "EPThrottle";
         case EPStall    : return "EPStall";
         case EPComplete : return "EPComplete";
      }
   }

protected:
   /** Data 0/1 Transmit flag */
   volatile DataToggle fDataToggle;

   /** Odd/Even Transmit buffer flag */
   volatile BufferToggle fTxOdd;

   /** Odd/Even Receive buffer flag */
   volatile BufferToggle fRxOdd;

   /** End-point state */
   volatile EndpointState fState;

   /**
    *  Indicates that the IN transaction needs to be
    *  terminated with ZLP if size is a multiple of EP_MAXSIZE
    */
   volatile bool fNeedZLP;

   /**
    *  Callback used on completion of transaction
    *
    * @param[in]  endpointState State of endpoint before completion \n
    * e.g. EPDataOut, EPStatusIn, EPLastIn, EPStatusOut\n
    * The endpoint is in EPIdle state now.
    */
   void (*fCallback)(EndpointState endpointState);

   /**
    *  Dummy callback used to catch use of unset callback
    *
    * @param[in]  endpointState State of endpoint before completion
    */
   static void unsetHandlerCallback(EndpointState endpointState) {
      (void)endpointState;
      //      setAndCheckErrorCode(E_NO_HANDLER);
   }

   volatile USB_Type &fUsb;

   /** Hardware instance pointer */
   __attribute__((always_inline)) volatile USB_Type &usb() { return fUsb; }

public:
   /** End point number */
   const int fEndpointNumber;

   /**
    * Constructor
    *
    * @param[in]  endpointNumber End-point number
    */
   constexpr Endpoint(int endpointNumber, volatile USB_Type &usb) :
            fDataToggle(DataToggle_0),
            fTxOdd(BufferToggle_Even),
            fRxOdd(BufferToggle_Even),
            fState(EPIdle),
            fNeedZLP(false),
            fCallback(0),
            fUsb(usb),
            fEndpointNumber(endpointNumber) {
   }

   virtual ~Endpoint() {}

   /**
    * Set endpoint state
    *
    * @param[in] state
    */
   void setState(EndpointState state) {
      fState = state;
   }

   /**
    * Return end-point state
    *
    * @return Endpoint state
    */
   EndpointState getState() {
      return fState;
   }

   /**
    * Stall endpoint
    */
   void stall() {
//      console.WRITELN("EpX.stall");
      fState = EPStall;
      usb().ENDPOINT[fEndpointNumber].ENDPT |= USB_ENDPT_EPSTALL_MASK;
   }

   /**
    * Clear Stall on endpoint
    */
   void clearStall() {
//      console.WRITELN("EpX.clearStall");
      usb().ENDPOINT[fEndpointNumber].ENDPT &= ~USB_ENDPT_EPSTALL_MASK;
      fState       = EPIdle;
      fDataToggle  = DataToggle_0;
   }

   /**
    * Set Data toggle
    *
    * @param[in] dataToggle
    */
   void setDataToggle(DataToggle dataToggle) {
      fDataToggle = dataToggle;
   }

   /**
    * Flip active odd/even buffer state
    *
    * @param[in]  usbStat Value from USB_STAT
    */
   void flipOddEven(const UsbStat usbStat) {
      usbdm_assert(fEndpointNumber == usbStat.endp, "Wrong end point!");

      if (usbStat.tx) {
         // Flip Transmit buffer
         fTxOdd = !usbStat.odd;
      }
      else {
         // Flip Receive buffer
         fRxOdd = !usbStat.odd;
      }
   }

   /**
    * Set callback to execute at end of transaction
    *
    * @param[in]  callback The call-back function to execute\n
    *                      May be nullptr to remove callback
    */
   void setCallback(void (*callback)(EndpointState)) {
      if (callback == nullptr) {
         callback = unsetHandlerCallback;
      }
      fCallback = callback;
   }

   /**
    *  Indicates that the next IN transaction needs to be
    *  terminated with ZLP if modulo endpoint size
    *
    *  @param[in]  needZLP True to indicate need for ZLPs.
    *
    *  @note This flag is cleared on each transaction
    */
   void setNeedZLP(bool needZLP=true) {
      fNeedZLP = needZLP;
   }

};

/**
 * Class for generic endpoint
 *
 * @tparam Info         Class describing associated USB hardware
 * @tparam ENDPOINT_NUM Endpoint number
 * @tparam EP_MAXSIZE   Maximum size of packet
 */
template<class Info, int ENDPOINT_NUM, int EP_MAXSIZE>
class Endpoint_T : public Endpoint {

   using Endpoint::setDataToggle;

public:
   /** Size of end-point buffer */
   static constexpr int BUFFER_SIZE = EP_MAXSIZE;

protected:
   /** Buffer for Transmit & Receive data */
   uint8_t fDataBuffer[EP_MAXSIZE];

   /** Pointer to external data buffer for transmit/receive */
   volatile uint8_t* fDataPtr;

   /** Count of remaining bytes in external data buffer to transmit/receive */
   volatile uint16_t fDataRemaining;

   /** Count of data bytes transferred to/from data buffer */
   volatile uint16_t fDataTransferred;

   /**
    * Get Receive BDT entry to be used for next OUT transaction
    *
    * @return
    */
   BdtEntry *getFreeBdtReceiveEntry() {
      return fRxOdd?&endPointBdts[ENDPOINT_NUM].rxOdd:&endPointBdts[ENDPOINT_NUM].rxEven;
   }

   /**
    * Get Receive BDT entry used for last OUT transaction
    *
    * @return
    */
   BdtEntry *getCompleteBdtReceiveEntry() {
      return (!fRxOdd)?&endPointBdts[ENDPOINT_NUM].rxOdd:&endPointBdts[ENDPOINT_NUM].rxEven;
   }

   /**
    * Get Transmit BDT entry to be used for next IN transaction
    *
    * @return
    */
   BdtEntry *getFreeBdtTransmitEntry() {
      return fTxOdd?&endPointBdts[ENDPOINT_NUM].txOdd:&endPointBdts[ENDPOINT_NUM].txEven;
   }

   /**
    * Get Transmit BDT entry used for last IN transaction
    *
    * @return
    */
   BdtEntry *getCompleteBdtTransmitEntry() {
      return (!fTxOdd)?&endPointBdts[ENDPOINT_NUM].txOdd:&endPointBdts[ENDPOINT_NUM].txEven;
   }

public:
   /**
    * Constructor
    */
   Endpoint_T() :
      Endpoint(ENDPOINT_NUM, Info::usb()) {
      initialise();
   }

   /**
    * Gets pointer to USB data buffer
    *
    * @return Pointer to buffer
    */
   uint8_t *getBuffer() {
      return fDataBuffer;
   }

   /**
    * Gets size of last completed transfer
    *
    * @return Size of transfer
    */
   uint16_t getDataTransferredSize() {
      return fDataTransferred;
   }

   /**
    * Initialise endpoint
    *  - Internal state
    *  - BDTs
    */
   void initialise() {
      fTxOdd            = BufferToggle_Even;
      fRxOdd            = BufferToggle_Even;
      fState            = EPIdle;
      fDataToggle       = DataToggle_0;

      fDataPtr          = nullptr;
      fDataTransferred  = 0;
      fDataRemaining    = 0;
      fNeedZLP          = false;
      fCallback         = unsetHandlerCallback;

      // Assumes single shared buffer
      endPointBdts[ENDPOINT_NUM].rxEven.setAddress(nativeToLe32((uint32_t)fDataBuffer));
      endPointBdts[ENDPOINT_NUM].rxOdd.setAddress(nativeToLe32((uint32_t)fDataBuffer));
      endPointBdts[ENDPOINT_NUM].txEven.setAddress(nativeToLe32((uint32_t)fDataBuffer));
      endPointBdts[ENDPOINT_NUM].txOdd.setAddress(nativeToLe32((uint32_t)fDataBuffer));
   }

   /**
    * Start IN transaction [Transmit, device -> host, DATA0/1]
    *
    * @param[in]  state   State to adopt for transaction e.g. EPDataIn, EPStatusIn
    * @param[in]  bufSize Size of buffer to send (may be zero)
    * @param[in]  bufPtr  Pointer to buffer (may be NULL to indicate fDatabuffer is being used directly)
    */
   void startTxTransaction(EndpointState state, uint8_t bufSize=0, const uint8_t *bufPtr=nullptr) {
      // Pointer to data
      fDataPtr = (uint8_t*)bufPtr;

      // Count of bytes transferred
      fDataTransferred = 0;

      // Count of remaining bytes
      fDataRemaining = bufSize;

      // State for this transaction
      fState = state;

      // Configure the BDT for transfer
      initialiseBdtTx();
   }

   /**
    * Configure the BDT for next IN [Transmit, device -> host]
    */
   void initialiseBdtTx() {

      // Get BDT to use
      BdtEntry *bdt = getFreeBdtTransmitEntry();

      if ((endPointBdts[ENDPOINT_NUM].txEven.own == BdtOwner_SIE) ||
            (endPointBdts[ENDPOINT_NUM].txOdd.own  == BdtOwner_SIE)) {
         console.WRITELN("Opps-Tx");
         return;
      }
      uint16_t size = fDataRemaining;
      if (size > EP_MAXSIZE) {
         size = EP_MAXSIZE;
      }
      // No ZLP needed if sending undersize packet
      if (size<EP_MAXSIZE) {
         setNeedZLP(false);
      }
      // fDataBuffer may be nullptr to indicate using fDataBuffer directly
      if (fDataPtr != nullptr) {
         // Copy the Transmit data to EP buffer
         (void) memcpy(fDataBuffer, (void*)fDataPtr, size);

         // Pointer to _next_ data
         fDataPtr += size;
      }
      // Count of transferred bytes
      fDataTransferred += size;

      // Count of remaining bytes
      fDataRemaining   -= size;

      // Set up to Transmit packet
      bdt->setByteCount((uint8_t)size);
      if (fDataToggle == DataToggle_1) {
         bdt->setControl(BDTEntry_OWN_MASK|BDTEntry_DATA1_MASK|BDTEntry_DTS_MASK);
      }
      else {
         bdt->setControl(BDTEntry_OWN_MASK|BDTEntry_DATA0_MASK|BDTEntry_DTS_MASK);
      }
      // console.WRITE("BdtTx(s=").WRITE(size).WRITE(",").WRITE(fDataToggle?"D1,":"D0,").WRITE(fTxOdd?"Odd),":"Even),");;
   }

   /**
    *  Start an OUT transaction [Receive, device <- host, DATA0/1]
    *
    *   @param[in]  state   - State to adopt for transaction e.g. EPIdle, EPDataOut, EPStatusOut
    *   @param[in]  bufSize - Size of data to transfer (may be zero)
    *   @param[in]  bufPtr  - Buffer for data (may be nullptr)
    *
    *   @note The end-point is configured to to accept EP_MAXSIZE packet irrespective of bufSize
    */
   void startRxTransaction(EndpointState state, uint8_t bufSize=0, uint8_t *bufPtr=nullptr) {
      // Count of bytes transferred
      fDataTransferred     = 0;
      // Total bytes to Receive
      fDataRemaining       = bufSize;
      // Where to (eventually) place data
      fDataPtr             = bufPtr;
      // State to adopt
      fState               = state;
      // Configure the BDT for transfer
      initialiseBdtRx();
   }

   /**
    * Configure the BDT for OUT [Receive, device <- host, DATA0/1]
    *
    * @note No action is taken if already configured
    * @note Always uses EP_MAXSIZE for packet size accepted
    */
   void initialiseBdtRx() {

      // Get BDT to use
      BdtEntry *bdt = getFreeBdtReceiveEntry();

      usbdm_assert(bdt->own == BdtOwner_MCU, "MCU doesn't own BDT!");

      // Set up to Receive packet
      // Always used maximum size even if expecting less data
      bdt->setByteCount(EP_MAXSIZE);
      if (fDataToggle) {
         bdt->setControl(BDTEntry_OWN_MASK|BDTEntry_DATA1_MASK|BDTEntry_DTS_MASK);
      }
      else {
         bdt->setControl(BDTEntry_OWN_MASK|BDTEntry_DATA0_MASK|BDTEntry_DTS_MASK);
      }
      // console.WRITE("BdtRx(s=").WRITE(EP_MAXSIZE).WRITE(fDataToggle?",D1:":",D0:").WRITE(fRxOdd?"Odd),":"Even),");
   }

   /**
    *  Save the data from an OUT packet and advance pointers etc.
    *
    *  @return Number of bytes saved
    */
   uint8_t saveRxData() {

      // Get BDT to use
      BdtEntry *bdt = getCompleteBdtReceiveEntry();

      uint8_t size = bdt->bc;

      if (size > 0) {
         // Check if more data than requested - discard excess
         if (size > fDataRemaining) {
            size = fDataRemaining;
         }
         // Check if external buffer in use
         if (fDataPtr != nullptr) {
            // Copy the data from the Receive buffer to external buffer
            ( void )memcpy((void*)fDataPtr, (void*)fDataBuffer, size);
            // Advance buffer ptr
            fDataPtr    += size;
         }
         // Count of transferred bytes
         fDataTransferred += size;
         // Count down bytes to go
         fDataRemaining   -= size;
      }
      else {
         console.WRITELN("RxSize = 0\n");
      }
      return size;
   }

   /**
    * Handle OUT [Receive, device <- host, DATA0/1]
    */
   void handleOutToken() {
      //      console.WRITE("Out(),");

      uint8_t transferSize = 0;

      // Toggle DATA0/1 for next packet
      fDataToggle = !fDataToggle;

      switch (fState) {
         case EPDataOut:        // Receiving a sequence of OUT packets
            // Save the data from the Receive buffer
            transferSize = saveRxData();
            // Completed transfer when undersize packet or received expected number of bytes
            if ((transferSize < EP_MAXSIZE) || (fDataRemaining == 0)) {
               // Now idle
               fState = EPIdle;
               fCallback(EPDataOut);
            }
            else {
               // Set up for next OUT packet
               initialiseBdtRx();
            }
            break;

         case EPStatusOut:       // Done OUT packet as a status handshake from host (IN CONTROL transfer)
            // No action
            fState = EPIdle;
            fCallback(EPStatusOut);
            break;

         // We don't expect an OUT token while in the following states
         case EPDataIn:    // Doing a sequence of IN packets (until data count <= EP_MAXSIZE)
         case EPStatusIn:  // Just done an IN packet as a status handshake
         case EPIdle:      // Idle
         case EPComplete:  // Not used
         case EPStall:     // Not used
         case EPThrottle:  // Not used
            console.WRITE("Unexpected OUT, ep=").WRITE(ENDPOINT_NUM).WRITE(", s=").WRITELN(getStateName(fState));
            fState = EPIdle;
            break;
      }
   }

   /**
    * Handle IN token [Transmit, device -> host]
    */
   void handleInToken() {
      //       console.WRITE("In(),");

      // Toggle DATA0/1 for next packet
      fDataToggle = !fDataToggle;

      //   console.WRITELN(fHardwareState[BDM_OUT_ENDPOINT].data0_1?"ep2HandleInToken-T-1":"ep2HandleInToken-T-0");

      switch (fState) {
         case EPDataIn:    // Doing a sequence of IN packets
            // Check if packets remaining
            if ((fDataRemaining > 0) || fNeedZLP) {
               // Set up next IN packet
               initialiseBdtTx();
            }
            else {
               fState = EPIdle;
               // Execute callback function at end of IN packets
               fCallback(EPDataIn);
            }
            break;


         case EPStatusIn: // Just done an IN packet as a status handshake for an OUT Data transfer
            // Now Idle
            fState = EPIdle;
            // Execute callback function at end of OUT packets
            fCallback(EPStatusIn);
            break;

            // We don't expect an IN token while in the following states
         case EPIdle:      // Idle (Transmit complete)
         case EPDataOut:   // Doing a sequence of OUT packets (until data count <= EP_MAXSIZE)
         case EPStatusOut: // Doing an OUT packet as a status handshake
         default:
            console.WRITE("Unexpected IN, ep=").WRITE(ENDPOINT_NUM).WRITE(", s=").WRITELN(getStateName(fState));
            fState = EPIdle;
            break;
      }
   }
};

/**
 * Class for CONTROL endpoint
 *
 * @tparam ENDPOINT_NUM Endpoint number
 * @tparam EP0_SIZE   Maximum size of packet
 */
template<class Info, int EP0_SIZE>
class ControlEndpoint : public Endpoint_T<Info, 0, EP0_SIZE> {

public:
   using Endpoint_T<Info, 0, EP0_SIZE>::fState;
   using Endpoint_T<Info, 0, EP0_SIZE>::initialiseBdtRx;
   using Endpoint_T<Info, 0, EP0_SIZE>::getFreeBdtReceiveEntry;
   using Endpoint_T<Info, 0, EP0_SIZE>::usb;
   using Endpoint_T<Info, 0, EP0_SIZE>::startTxTransaction;
   using Endpoint_T<Info, 0, EP0_SIZE>::startRxTransaction;

   /**
    * Constructor
    */
   constexpr ControlEndpoint() {
   }

   /**
    * Destructor
    */
   virtual ~ControlEndpoint() {
   }

   /**
    * Initialise endpoint
    *  - Internal state
    *  - BDTs
    *  - usb().ENDPOINT[].ENDPT
    */
   void initialise() {
      Endpoint_T<Info, 0, EP0_SIZE>::initialise();

      // Receive/Transmit/SETUP
      usb().ENDPOINT[0].ENDPT = USB_ENDPT_EPRXEN_MASK|USB_ENDPT_EPTXEN_MASK|USB_ENDPT_EPHSHK_MASK;
   }

   /**
    * Configure EP0 for an empty status IN transactions [Tx, device -> host, DATA1]\n
    * Used to acknowledge a DATA out transaction.
    *
    * State       = EPStatusIn
    * Data Toggle = DATA1
    */
   void startTxStatus() {
      Endpoint::setDataToggle(DataToggle_1);
      startTxTransaction(EPStatusIn);
   }

   /**
    * Set up for SETUP packet [Rx, host -> device, DATA0]
    *
    * State       = EPIdle
    * Data Toggle = DATA0
    */
   void startSetupTransaction() {
      Endpoint::setDataToggle(DataToggle_0);
      startRxTransaction(EPIdle);
   }

   /**
    * Conditionally set up for SETUP packet [Rx, host -> device, DATAx]
    * If already configured for OUT transfer does nothing.
    *
    * This routine would be used after processing a SETUP transaction
    * (before processing any data transactions).
    *
    * State       = unchanged
    * Data Toggle = unchanged
    */
   void checkSetupReady() {
      // Get BDT to use
      BdtEntry *bdt = getFreeBdtReceiveEntry();

      if (bdt->own == BdtOwner_MCU) {
         // Make ready for SETUP packet
         initialiseBdtRx();
      }
   }

   /**
    * Modifies endpoint state after SETUP has been received.
    * DATA1 is toggle for 1st data transaction
    *
    * State       = EPIdle
    * Data Toggle = DATA1
    */
   void setupReceived() {
      fState      = EPIdle;
      Endpoint::setDataToggle(DataToggle_1);
   }
};

/**
 * Class for IN endpoint
 *
 * @tparam Info         Class describing associated USB hardware
 * @tparam ENDPOINT_NUM Endpoint number
 * @tparam EP_MAXSIZE   Maximum size of packet
 */
template<class Info, int ENDPOINT_NUM, int EP_MAXSIZE>
class InEndpoint : public Endpoint_T<Info, ENDPOINT_NUM, EP_MAXSIZE> {

protected:
   using Endpoint_T<Info, ENDPOINT_NUM, EP_MAXSIZE>::usb;

private:
   // Make private
   using Endpoint_T<Info, ENDPOINT_NUM, EP_MAXSIZE>::startRxTransaction;

public:
   /**
    * Constructor
    */
   constexpr InEndpoint() {
   }

   /**
    * Initialise endpoint
    *  - Internal state
    *  - BDTs
    *  - usb().ENDPOINT[].ENDPT
    */
   void initialise() {
      Endpoint_T<Info, ENDPOINT_NUM, EP_MAXSIZE>::initialise();

      // Transmit only
      usb().ENDPOINT[ENDPOINT_NUM].ENDPT = USB_ENDPT_EPTXEN_MASK|USB_ENDPT_EPHSHK_MASK;
   }
};

/**
 * Class for OUT endpoint
 *
 * @tparam Info         Class describing associated USB hardware
 * @tparam ENDPOINT_NUM Endpoint number
 * @tparam EP_MAXSIZE   Maximum size of packet
 */
template<class Info, int ENDPOINT_NUM, int EP_MAXSIZE>
class OutEndpoint : public Endpoint_T<Info, ENDPOINT_NUM, EP_MAXSIZE> {

protected:
   using Endpoint_T<Info, ENDPOINT_NUM, EP_MAXSIZE>::usb;

private:
   // Make private
   using Endpoint_T<Info, ENDPOINT_NUM, EP_MAXSIZE>::startTxTransaction;

public:
   /**
    * Constructor
    */
   constexpr OutEndpoint() {
   }

   /**
    * Initialise endpoint
    *  - Internal state
    *  - BDTs
    *  - usb().ENDPOINT[].ENDPT
    */
   void initialise() {
      Endpoint_T<Info, ENDPOINT_NUM, EP_MAXSIZE>::initialise();

      // Receive only
      usb().ENDPOINT[ENDPOINT_NUM].ENDPT = USB_ENDPT_EPRXEN_MASK|USB_ENDPT_EPHSHK_MASK;
   }
};

/**
 * End USB_Group
 * @}
 */

}; // end namespace

#endif /* HEADER_USB_ENDPOINT_H */
