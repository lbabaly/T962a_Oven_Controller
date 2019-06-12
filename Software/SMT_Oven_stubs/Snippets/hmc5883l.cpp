/**
 ============================================================================
 * @file     hmc5883l.cpp
 * @brief    Interface for HMC5883L 3-axis magnetometer
 * @version  V4.11.1.70
 * @date     18 June 2015
 * @author   podonoghue
 ============================================================================
 */

#include "hmc5883l.h"
 /*
 * *****************************
 * *** DO NOT EDIT THIS FILE ***
 * *****************************
 *
 * This file is generated automatically.
 * Any manual changes will be lost.
 */
using namespace USBDM;

enum Mag_Addr {
   /*                            type  default  */
   CRA_REG_M         = 0x00,  /* rw    00010000 */
   CRB_REG_M         = 0x01,  /* rw    00100000 */
   MR_REG_M          = 0x02,  /* rw    00000011 */
   OUT_X_H_M         = 0x03,  /* r     -        */
   OUT_X_L_M         = 0x04,  /* r     -        */
   OUT_Y_H_M         = 0x05,  /* r     -        */
   OUT_Y_L_M         = 0x06,  /* r     -        */
   OUT_Z_H_M         = 0x07,  /* r     -        */
   OUT_Z_L_M         = 0x08,  /* r     -        */
   SR_REG_Mg         = 0x09,  /* r     00000000 */
   IRA_REG_M         = 0x0A,  /* r     01001000 */
   IRB_REG_M         = 0x0B,  /* r     00110100 */
   IRC_REG_M         = 0x0C,  /* r     00110011 */
};

#define HMC5883L_SR_LOCK   (1<<1)   // Register values are locked when:
   //                               //  1. some, but not all of, the six data output registers have been read,
   //                               //  2. mode register has been read.
   //                               // Remains locked until:
   //                               //  1. all six result bytes have been read,
   //                               //  2. the mode register is changed,
   //                               //  3. the measurement configuration (CRA) is changed,
   //                               //  4. power is reset.
#define HMC5883L_SR_RDY    (1<<0)   // Ready Bit.
   //                               // Set when data is written to all six data registers.
   //                               // Cleared when device initiates a write to the data output registers and
   //                               // after one or more of the data output registers are written to.

/**
 * Constructor
 *
 * @param i2c - I2C interface to use
 *
 */
   HMC5883L::HMC5883L(USBDM::I2c &i2c) : i2c(i2c) {

   // Set default settings
   static const uint8_t settings[] = {
      CRA_REG_M,
      craValue(MagAverages_8, MagBias_Normal, MagDataRate_1_5_Hz),
      crbValue(MagRange_4_7),
      MagMode_Sleep,
   };
   i2c.transmit(magAddress, sizeof(settings), settings);

#ifdef DEBUG_BUILD
   // Read back - debug only
   uint8_t confirm[3];
   i2c.txRx(magAddress, 1, settings, sizeof(confirm), confirm);
#endif
}

/**
 * Read ID from compass
 *
 * @return ID value as 24-bit number (0x483433 for HMC5883L)
 */
uint32_t HMC5883L::readID(void) {
   uint8_t values[] = {IRA_REG_M, 0x00, 0x00};
   i2c.txRx(magAddress, 1, sizeof(values), values);
   return (values[0]<<16)|(values[1]<<8)|values[2];
}

/**
    * Set compass gain and hence range on all channels
 *
 * @param range                                      \n
 * G    Recommended   Gain        Resolution         \n
 * 321  Sensor Range  (LSB/Gauss) (mGauss/LSB)       \n
 * 000   +/- 0.88 Ga    1370        0.73             \n
 * 001   +/- 1.3  Ga    1090        0.92 (default)   \n
 * 010   +/- 1.9  Ga     820        1.22             \n
 * 011   +/- 2.5  Ga     660        1.52             \n
 * 100   +/- 4.0  Ga     440        2.27             \n
 * 101   +/- 4.7  Ga     390        2.56             \n
 * 110   +/- 5.6  Ga     330        3.03             \n
 * 111   +/- 8.1  Ga     230        4.35
 */
   void HMC5883L::setRange(MagRange range) {
   static const uint8_t controlRegB_Setting[] = {CRB_REG_M, crbValue(range)};
   i2c.transmit(magAddress, sizeof(controlRegB_Setting), controlRegB_Setting);
}

   /**
    * Set Control register values
    *
    * @param cra - Use craValue() to construct
    * @param crb - Use crbValue() to construct
    */
   void HMC5883L::setConfiguration(uint8_t cra, uint8_t crb) {
   // Set CRA & CRB
   static const uint8_t controlReg_Settings[] = {CRA_REG_M, cra, crb};
   i2c.transmit(magAddress, sizeof(controlReg_Settings), controlReg_Settings);
}

/**
 * Do a single triggered measurement of magnetic field
 *
 * @param x - X intensity
 * @param y - Y intensity
 * @param z - Z intensity
 */
   void HMC5883L::doMeasurement(int16_t *x, int16_t *y, int16_t *z) {
   static const uint8_t modeReg_Setting[] = {MR_REG_M, MagMode_Single};
   i2c.transmit(magAddress, sizeof(modeReg_Setting), modeReg_Setting);

   static const uint8_t statusRegAddress[] = {SR_REG_Mg};
   uint8_t status[1];
   do {
      i2c.txRx(magAddress, sizeof(statusRegAddress), statusRegAddress, sizeof(status), status);
      } while ((status[0]&HMC5883L_SR_RDY) == 0);

   static const uint8_t resultRegAddress[] = {OUT_X_H_M};
   uint8_t values[6];
   i2c.txRx(magAddress, sizeof(resultRegAddress), resultRegAddress, sizeof(values), values);

   *x = (values[0]<<8)+values[1];
   *z = (values[2]<<8)+values[3];
   *y = (values[4]<<8)+values[5];
}

//   void HMC5883L::calibrate() {
//      const uint8_t cra[]     = {CRA_REG_M,  0x71};
//      i2c.transmit(magAddress,  sizeof(cra), cra);
//      const uint8_t crb[]     = {CRB_REG_M,  0xA0};
//      i2c.transmit(magAddress,  sizeof(cra), crb);
//      const uint8_t mode[]    = {MR_REG_M,   0x00};
//      i2c.transmit(magAddress, sizeof(cra), mode);
//
//      uint8_t status[1];
//      static const uint8_t statusRegAddress[] = {SR_REG_Mg};
//      do {
//         i2c.txRx(magAddress, sizeof(statusRegAddress), statusRegAddress, sizeof(status), status);
//      } while ((status[0]&HMC5883L_SR_RDY) == 0);
//
//      static const uint8_t resultRegAddress[] = {OUT_X_H_M};
//      uint8_t values[6];
//      i2c.txRx(magAddress, sizeof(resultRegAddress), resultRegAddress, sizeof(values), values);
//
//      int x = (int16_t)((values[0]<<8)+values[1]);
//      int z = (int16_t)((values[2]<<8)+values[3]);
//      int y = (int16_t)((values[4]<<8)+values[5]);
//   }