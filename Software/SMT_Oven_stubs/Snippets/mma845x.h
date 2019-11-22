/**
 * @file     mma845x.h
 * @brief    Interface for MMA845x accelerometer
 *
 * @version  V4.11.1.70
 * @date     18 June 2015
 */

#ifndef INCLUDE_USBDM_MMA845X_H_
#define INCLUDE_USBDM_MMA845X_H_
 /*
 * *****************************
 * *** DO NOT EDIT THIS FILE ***
 * *****************************
 *
 * This file is generated automatically.
 * Any manual changes will be lost.
 */
#include <stdint.h>
#include "i2c.h"

namespace USBDM {

/**
 * @addtogroup MMA845x_Group MMA845x 3-axis accelerometer
 * @brief C++ Class providing interface to MMA845x
 * @{
 */

/**
 * @brief Class representing an interface for MMA845x 3-axis accelerometer over I2C
 *
 * <b>Example</b>
 * @code
 *  // Instantiate interfaces
 *
 *  // I2C interface
 *  I2c0     i2c0;
 *  // Accelerometer via I2C
 *  MMA845x  accelerometer(i2c0, MMA845x::ACCEL_2Gmode);
 *
 *  uint8_t id = accelerometer.readID();
 *  printf("Device ID = 0x%02X\n", id);
 *
 *  printf("Before simple calibration (make sure the device is level!)\n");
 *  accelerometer.calibrateAccelerometer();
 *
 *  printf("After calibration\n");
 *  for(;;) {
 *     int accelStatus;
 *     int16_t accelX,accelY,accelZ;
 *
 *     accelerometer.readAccelerometerXYZ(accelStatus, accelX, accelY, accelZ);
 *     printf("s=0x%02X, aX=%10d, aY=%10d, aZ=%10d\n", accelStatus, accelX, accelY, accelZ);
 *     waitMS(400);
 *  }
 *
 * @endcode
 */
class MMA845x {

public:
   enum AccelDataRate {
      AccelDataRate_800Hz      = (0<<3),  //!< Sample Rate 800 Hz
      AccelDataRate_400Hz      = (1<<3),  //!< Sample Rate 400 Hz
      AccelDataRate_200Hz      = (2<<3),  //!< Sample Rate 200 Hz
      AccelDataRate_100Hz      = (3<<3),  //!< Sample Rate 100 Hz
      AccelDataRate_50Hz       = (4<<3),  //!< Sample Rate 50 Hz
      AccelDataRate_12_5Hz     = (5<<3),  //!< Sample Rate 12.5 Hz
      AccelDataRate_6_25Hz     = (6<<3),  //!< Sample Rate 6.25 Hz
      AccelDataRate_1_56Hz     = (7<<3),  //!< Sample Rate 1.56 Hz
   };

   enum AccelSleepDataRate {
      AccelSleepDataRate_50Hz    = (0<<6), //!< Sample Rate when sleeping 50 Hz
      AccelSleepDataRate_12_5Hz  = (1<<6), //!< Sample Rate when sleeping 12.5 Hz
      AccelSleepDataRate_6_25Hz  = (2<<6), //!< Sample Rate when sleeping 6.25 Hz
      AccelSleepDataRate_1_56Hz  = (3<<6), //!< Sample Rate when sleeping 1.56 Hz
   };

   /**
    * @param[in] accelDataRate        // Rate when in normal mode
    * @param[in] accelSleepDataRate   // Rate when is sleep
    * @param[in] active               // Active
    * @param[in] reducedNoise         // Reduced noise mode
    * @param[in] fastRead             // Fast read mode
    */
   static constexpr uint8_t cr1Value(
         AccelDataRate        accelDataRate        = AccelDataRate_50Hz,
         AccelSleepDataRate   accelSleepDataRate   = AccelSleepDataRate_50Hz,
         bool                 active               = true,
         bool                 reducedNoise         = false,
         bool                 fastRead             = false
         ) {
      return accelSleepDataRate|accelDataRate|(reducedNoise?(1<<2):0)|(fastRead?(1<<1):0)|(active?(1<<0):0);
   }

   enum AccelerometerMode {
      ACCEL_2Gmode      = (0<<0),        //!< 2g Full-scale, no high-pass filter
      ACCEL_4Gmode      = (1<<0),        //!< 4g Full-scale, no high-pass filter
      ACCEL_8Gmode      = (2<<0),        //!< 8g Full-scale, no high-pass filter
      ACCEL_2G_HPF_mode = (1<<4)|(0<<0), //!< 2g Full-scale, high-pass filter
      ACCEL_4G_HPF_mode = (1<<4)|(1<<0), //!< 4g Full-scale, high-pass filter
      ACCEL_8G_HPF_mode = (1<<4)|(2<<0), //!< 8g Full-scale, high-pass filter
   } ;

private:
   USBDM::I2c &i2c;
   static const uint8_t DEVICE_ADDRESS = 0x1D<<1;  // SA0 pin : 0=>1C, 1=>1D
   static const uint8_t WHO_AM_I_VALUE = 0x1A;

   /**
    * Read Accelerometer register
    *
    * @param[in] regNum  - Register number
    */
   uint8_t readReg(uint8_t regNum);
   /**
    * Write Accelerometer register
    *
    * @param[in] regNum  - Register number
    * @param[in] value   - Value to write
    */
   void    writeReg(uint8_t regNum, uint8_t value);
   /**
    * Reset Accelerometer
    */
   void    reset(void);

public:

   /**
    * Constructor
    *
    * @param[in] i2c   - The I2C interface to use
    * @param[in] mode  - Mode of operation (gain and filtering)
    * @param[in] cr1   - Data rate etc (see cr1Value())
    */
   MMA845x(USBDM::I2c &i2c, AccelerometerMode mode, uint8_t cr1=cr1Value());
   /**
    * Put accelerometer into Standby mode
    */
   void standby();
   /**
    * Put accelerometer into Active mode
    */
   void active();
   /**
    * Obtains measurements from the accelerometer
    *
    * @param[out] status  - Indicates status of x, y & z measurements
    * @param[out] x       - X axis value
    * @param[out] y       - Y axis value
    * @param[out] z       - Z axis value
    */
   void readAccelerometerXYZ(int &status, int16_t &x, int16_t &y, int16_t &z);
   /**
    * Configure accelerometer
    *
    * @param[in] mode - One of ACCEL_2Gmode etc.
    * @param[in] cr1  - Data rate etc (see cr1Value())
    */
   void configure(AccelerometerMode mode, uint8_t cr1=cr1Value());
   /**
    * Read ID from accelerometer
    *
    * @return ID value as 8-bit number (0x1A for MMA8451Q)
    */
   uint32_t readID();
   /**
    * Calibrate accelerometer
    *
    * This assumes the accelerometer is level and stationary.
    * If the accelerometer is too far from level then no correction is applied and error returned
    *
    * @return true  Success
    * @return false Calibration failed
    */
   ErrorCode calibrateAccelerometer();
};

/**
 * @}
 */

} // End namespace USBDM

#endif /* INCLUDE_USBDM_MMA845X_H_ */
