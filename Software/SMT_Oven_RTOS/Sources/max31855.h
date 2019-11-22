/**
 * @file    max31855.h
 * @brief   MAX31855 Thermocouple interface
 *
 *  Created on: 18 Sep 2016
 *      Author: podonoghue
 */

#ifndef SOURCES_MAX31855_H_
#define SOURCES_MAX31855_H_

#include "flash.h"
#include "spi.h"

/**
 * Class representing an MAX31855 connected over SPI
 */
class Max31855 {

public:
   enum ThermocoupleStatus {
      TH_ENABLED,          //!< Enabled and OK
      TH_OPEN,             //!< No probe or open circuit
      TH_SHORT_VCC,        //!< Probe short to Vcc
      TH_SHORT_GND,        //!< Probe short to Gnd
      TH_MISSING,          //!< No response - Max31855 not present at that address
      TH_DISABLED=0b111,   //!< Available but disabled (Temperature reading will still be valid)
   };

protected:

   /** SPI configuration value */
   USBDM::SpiConfig spiConfig;

   /** SPI used for communication */
   USBDM::Spi &spi;

   /** Which PCS signal to use */
   const USBDM::SpiPeripheralSelect pinNum;

   /** Offset to add to reading from probe */
   USBDM::Nonvolatile<int> &offset;

   /** Used to disable sensor */
   USBDM::Nonvolatile<bool> &enabled;

   /** The result of last Temperature measurements */
   float                lastTemperature;

   /** The result of last Cold Reference Temperature measurements */
   float                lastColdReference;

   /** The status of last Temperature measurements */
   ThermocoupleStatus   lastStatus;

public:
   /**
    * Constructor
    *
    * @param[in] spi     The SPI to use to communicate with MAX31855
    * @param[in] pinNum  PCS to use
    * @param[in] offset  Offset to add to reading from probe
    * @param[in] enabled Reference to non-volatile variable enabling thermocouple
    */
   Max31855(USBDM::Spi &spi, USBDM::SpiPeripheralSelect pinNum, USBDM::Nonvolatile<int> &offset, USBDM::Nonvolatile<bool> &enabled) :
      spi(spi), pinNum(pinNum), offset(offset), enabled(enabled), lastTemperature(0), lastColdReference(0), lastStatus(TH_MISSING) {
      using namespace USBDM;

      spi.startTransaction();

      // Configure SPI
      spi.setPeripheralSelect(pinNum, ActiveLow);
      spi.setSpeed(2.5*MHz);
      spi.setMode(SpiMode_0);
      spi.setFrameSize(8);

      // Record configuration in case SPI is shared
      spiConfig = spi.getConfiguration();

      spi.endTransaction();
      }

   /**
    * Convert status to string
    *
    * @param[in] status 3-bit status value from thermocouple
    *
    * @return Short string representing status
    */
   static const char *getStatusName(ThermocoupleStatus status) {
      switch (status) {
      case TH_ENABLED   : return "OK  ";  // OK!
      case TH_OPEN      : return "Open";  // No probe or open circuit
      case TH_SHORT_VCC : return "Vcc ";  // Probe short to Vcc
      case TH_SHORT_GND : return "Gnd ";  // Probe short to Gnd
      case TH_MISSING   : return "----";  // No response - Max31855 not present at that address
      case TH_DISABLED  : return "Dis ";  // OK but disabled
      default           : return "????";  // Unknown
      }
   }
   /**
    * Enables/disables the sensor
    *
    * @param[in] enable True to enable sensor
    *
    * @note This is a non-volatile setting
    */
   void enable(bool enable=true) {
      enabled = enable;
   }

   /**
    * Toggles the enables state of the sensor
    *
    * @note This is a non-volatile setting
    */
   void toggleEnable() {
      enabled = !enabled;
   }

   /**
    * Check if sensor is enabled
    *
    * @return true => enabled
    *
    * @note This is a non-volatile setting
    */
   bool isEnabled() const {
      return enabled;
   }

   /**
    * Read the thermocouple.
    * This initiates an measure of the thermocouple and updates internal state as well as returning the new values.
    *
    * @param[out] temperature   Temperature reading of external probe (.25 degree resolution)
    * @param[out] coldReference Temperature reading of internal cold-junction reference (.0625 degree resolution)
    *
    * @return status flag
    *
    * @note Temperature and cold-junction may be valid even if the thermocouple is disabled (TH_DISABLED).
    */
   ThermocoupleStatus getNewReading(float &temperature, float &coldReference) {
      uint8_t data[] = {
            0xFF, 0xFF, 0xFF, 0xFF,
      };
      {
//      PulseTp tp(8);

      spi.startTransaction(spiConfig);
      {
//         PulseTp tp(5);
         spi.txRx(sizeof(data), (uint8_t*)nullptr, data);
      }
      spi.endTransaction();
      }
      // Temperature = sign-extended 14-bit value
      lastTemperature = (((int16_t)((data[0]<<8)|data[1]))>>2)/4.0;

      // Add manual offset
      lastTemperature += offset;

      // Cold junction = sign-extended 12-bit value
      lastColdReference = (((int16_t)((data[2]<<8)|data[3]))>>4)/16.0;

      /*  Raw status
       *    0x000 => OK
       *    0bxx1 => Open circuit
       *    0bx1x => Short to Gnd
       *    0b1xx => Short to Vcc
       *    0b111 => No response - Max31855 not present at that address
       */
      int rawStatus = data[3]&0x07;
      lastStatus = TH_ENABLED;
      if (rawStatus != 0) {
         // Invalid lastTemperature measurement
         lastTemperature = NAN;
      }
      if (rawStatus == 0x111) {
         // No device so no Cold reference
         lastColdReference = NAN;
         lastStatus = TH_MISSING;
      }
      else if (rawStatus & 0b001) {
         //Open
         lastStatus = TH_OPEN;
      }
      else if (rawStatus & 0b010) {
         // Ground short
         lastStatus = TH_SHORT_GND;
      }
      else if (rawStatus & 0b100) {
         // Vcc short
         lastStatus = TH_SHORT_VCC;
      }
      else if (!enabled) {
         // Available but not enabled
         lastStatus = TH_DISABLED;
      }
      // Return results
      temperature   = lastTemperature;
      coldReference = lastColdReference;

      // Return status flag
      return lastStatus;
   }

   /**
    * Get thermocouple reading.
    * This does not initiate a new measurement - it just return the last measurement taken.
    *
    * @param[out] temperature   Temperature reading of external probe (.25 degree resolution)
    * @param[out] coldReference Temperature reading of internal cold-junction reference (.0625 degree resolution)
    *
    * @return Status of sensor
    *
    * @note Temperature will be zero if the thermocouple is disabled or unusable.
    * @note Cold-junction will be valid even if the thermocouple is disabled (TH_DISABLED).
    */
   ThermocoupleStatus getLastEnabledReading(float &temperature, float &coldReference) {
      USBDM::CriticalSection cs;
      temperature = lastTemperature;
      if (lastStatus == TH_DISABLED) {
         temperature = 0;
      }
      coldReference = lastColdReference;
      return lastStatus;
   }

   /**
    * Get thermocouple reading.
    * This does not initiate a new measurement - it just return the last measurement taken.
    *
    * @param[out] temperature   Temperature reading of external probe (.25 degree resolution)
    * @param[out] coldReference Temperature reading of internal cold-junction reference (.0625 degree resolution)
    *
    * @return Status of sensor
    *
    * @note Temperature will be zero if the thermocouple is disabled or unusable.
    * @note Cold-junction will be valid even if the thermocouple is disabled (TH_DISABLED).
    */
   ThermocoupleStatus getLastReading(float &temperature, float &coldReference) {
      USBDM::CriticalSection cs;
      temperature   = lastTemperature;
      coldReference = lastColdReference;
      return lastStatus;
   }

   /**
    * Set offset added to temperature reading
    *
    * @param[in] off offset to set
    *
    * @note This is a non-volatile setting
    */
   void setOffset(int off) {
      offset = off;
   }

   /**
    * Get offset that is added to temperature reading
    *
    * @return Offset as an integer
    *
    * @note This is a non-volatile setting
    */
   int getOffset() {
      return offset;
   }
};

#endif /* SOURCES_MAX31855_H_ */
