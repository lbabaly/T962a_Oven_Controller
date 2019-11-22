/**
 * @file pit-example3.cpp (derived from pit-example3-MK.cpp)
 */
#include "hardware.h"
#include "pit.h"

using namespace USBDM;

/**
 * Programmable Interrupt Timer (PIT) Example
 *
 * Demonstrates PIT call-back
 *
 * Toggles LED
 */
/**
 * This example uses PIT interrupts.
 *
 * It is necessary to enable these in Configure.usbdmProject
 * under the "Peripheral Parameters"->PIT tab.
 * Select irqHandlerChannelX option (Class Method - Software ...)
 */

// Connection mapping - change as required
using Led = GpioA<2, USBDM::ActiveLow>;

using Timer        = Pit;
using TimerChannel = PitChannel<0>;
/*
 * This callback is set programmatically
 */
void flash(void) {
   Led::toggle();
}

int main() {
   Led::setOutput(
         PinDriveStrength_High,
         PinDriveMode_PushPull,
         PinSlewRate_Slow);

   Timer::configure(PitDebugMode_Stop);

   // Set handler programmatically
   TimerChannel::setCallback(flash);

   // Flash LED @ 1Hz
   TimerChannel::configureInTicks(::SystemBusClock, PitChannelIrq_Enable);

   TimerChannel::enableNvicInterrupts(true, NvicPriority_Normal);

   // Check for errors so far
   checkError();

   for(;;) {
      __asm__("nop");
   }
}
