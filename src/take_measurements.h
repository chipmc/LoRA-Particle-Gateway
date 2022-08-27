/**
 * @file take_measurements.h
 * @author Chip McClelland (chip@seeinisghts.com)
 * @brief This function will take measurements at intervals defined int he sleep_helper_config file.  
 * The libraries and functions needed will depend the spectifics of the device and sensors
 * 
 * @version 0.1
 * @date 2022-06-30
 * 
 */

// Particle functions
#ifndef TAKE_MEASUREMENTS_H
#define TAKE_MEASUREMENTS_H

#include "Particle.h"
#include "storage_objects.h"
#include "device_pinout.h"

extern char internalTempStr[16];                       // External as this can be called as a Particle variable
extern char signalStr[64];

bool takeMeasurements();                               // Function that calls the needed functions in turn
float tmp36TemperatureC (int adcValue);                // Temperature from the tmp36 - inside the enclosure
bool batteryState();                                   // Data on state of charge and battery status. Returns true if SOC over 60%
bool isItSafeToCharge();                               // See if it is safe to charge based on the temperature
void getSignalStrength();

#endif