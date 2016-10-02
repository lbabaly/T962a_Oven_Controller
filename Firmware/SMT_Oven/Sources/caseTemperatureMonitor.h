/*
 * caseTemperatureMonitor.h
 *
 *  Created on: 1Oct.,2016
 *      Author: podonoghue
 */

#ifndef SOURCES_CASETEMPERATUREMONITOR_CPP_
#define SOURCES_CASETEMPERATUREMONITOR_CPP_

#include "pit.h"
#include "max31855.h"

/**
 * Monitor for case temperature
 *
 * @tparam CaseFan      PWM controlling the case fan
 * @tparam pit_channel  PIT channel to use
 * @tparam Sensor       Temperature sensor
 * @tparam startTemp    Temperature at which to start the fan @10%
 * @tparam maxTemp      Temperature at which the fan is to be 100% on
 *
 */
template<typename CaseFan, int pit_channel, int startTemp=40, int maxTemp=80>
class CaseTemperatureMonitor {
   static Max31855 *tempSensor;

   static void checkCaseTemp() {
      float temperature, coldReference;
      int status = tempSensor->getReading(temperature, coldReference);
      if (status<7) {
         int dutyCycle = 10 + (100*(coldReference-startTemp))/(maxTemp-startTemp);
         if (dutyCycle<10) {
            dutyCycle = 0;
         }
         if (dutyCycle>100) {
            dutyCycle = 100;
         }
         CaseFan::setDutyCycle(dutyCycle);
      }
   }

public:
   CaseTemperatureMonitor(Max31855 *sensor) {
      tempSensor = sensor;
      CaseFan::enable();
      CaseFan::setPeriod(20*USBDM::ms);
      CaseFan::setDutyCycle(0);
      USBDM::Pit::enable();
      USBDM::Pit::configureChannel(pit_channel, 1.0f);
      USBDM::Pit::setCallback(pit_channel, checkCaseTemp);
      USBDM::Pit::enableInterrupts(pit_channel);
   }
};

template<typename CaseFan, int pit_channel, int startTemp, int maxTemp>
Max31855 *CaseTemperatureMonitor<CaseFan, pit_channel, startTemp, maxTemp>::tempSensor;

#endif /* SOURCES_CASETEMPERATUREMONITOR_CPP_ */