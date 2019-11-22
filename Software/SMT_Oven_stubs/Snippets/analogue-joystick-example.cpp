/**
 ============================================================================
 * @file analogue-joystick-example.cpp
 * @brief Example showing use of a use of 2 ADC channels with a 2-pot joystick
 *
 *  Created on: 10/6/2016
 *      Author: podonoghue
 ============================================================================
 */
#include "hardware.h"

using namespace USBDM;

/*
 * External Joy-stick
 * 2 x Analogue input
 * 1 x Digital input
 *
 */

// Connection mapping - change as required
using adc = USBDM::Adc0;

using JOYSTICK_X = USBDM::Adc0Channel<0>;
using JOYSTICK_Y = USBDM::Adc0Channel<3>;
using JOYSTICK_K = USBDM::GpioC<3>;

int main(void) {

   // Enable and configure ADC
   adc::configure(AdcResolution_8bit_se);

   // Calibrate before use
   adc::calibrate();

   JOYSTICK_K::setInput();

   for(;;) {
      int  x = JOYSTICK_X::readAnalogue();
      int  y = JOYSTICK_Y::readAnalogue();
      bool k = JOYSTICK_K::read();
      console.write("Joystick (X,Y,K) = ").write(x).write(", ").write(y).write(", ").writeln(k?"HIGH":"LOW");
   }
}
