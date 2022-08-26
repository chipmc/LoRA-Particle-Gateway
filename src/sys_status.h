#ifndef SYS_STATUS_H
#define SYS_STATUS_H

#include "Particle.h"

struct systemStatus_structure {  
  uint8_t structuresVersion;                        // Version of the data structures (system and data)
  uint8_t placeholdervalue;                         // For future use
  bool verboseMode;                                 // Turns on extra messaging
  bool solarPowerMode;                              // Powered by a solar panel or utility power
  bool lowPowerMode;                                // Does the device need to run disconnected to save battery
  uint8_t lowBatteryMode;                           // Is the battery level so low that we can no longer connect
  int stateOfCharge;                                // Battery charge level
  uint8_t batteryState;                             // Stores the current battery state
  int resetCount;                                   // reset count of device (0-256)
  unsigned long lastHookResponse;                   // Last time we got a valid Webhook response
  unsigned long lastConnection;                     // Last time we successfully connected to Particle
  uint16_t lastConnectionDuration;                  // How long - in seconds - did it take to last connect to the Particle cloud
  uint16_t frequencyMinutes;                 // How often do we report
  bool verizonSIM;                                  // Are we using a Verizon SIM?
};

extern struct systemStatus_structure sysStatus;

extern bool systemStatusWriteNeeded;               // Keep track of when we need to write

#endif
