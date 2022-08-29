/**
 * @file storage_objects.h
 * @author Chip McClelland (chip@seeinisghts.com)
 * @brief * This file contains the storage objects for the system and the current values.  
* Defining these as objects allows us to store them in persistent storage
*
* To dos: 
* Figure out some way to abstract the storage method (EEPROM, Flash, FRAM)
* Make a more comprehensive set of system varaibles as changing this object could cause upgrade issues in future releases
 * @version 0.1
 * @date 2022-06-30
 * 
 */
#ifndef STORAGE_OBJECTS_H
#define STORAGE_OBJECTS_H

#include "Particle.h"
#include "MB85RC256V-FRAM-RK.h"                     // Include this library if you are using FRAM

extern MB85RC64 fram;                               // FRAM storage initilized in main source file

// If you modify the sysStatus or current structures, make sure to update the hash function definitions
struct systemStatus_structure {                     // Where we store the configuration / status of the device
  uint16_t deviceID;                                // Unique to the device
  uint16_t nodeNumber;                              // Assigned by the gateway on joining the network
  uint8_t structuresVersion;                        // Version of the data structures (system and data)
  uint8_t firmwareRelease;                          // Version of the device firmware (integer - aligned to particle prodict firmware)
  bool verboseMode;                                 // Turns on extra messaging
  bool solarPowerMode;                              // Powered by a solar panel or utility power
  bool lowPowerMode;                                // Does the device need to run disconnected to save battery
  int resetCount;                                   // reset count of device (0-256)
  unsigned long lastHookResponse;                   // Last time we got a valid Webhook response
  unsigned long lastConnection;                     // Last time we successfully connected to Particle
  uint16_t lastConnectionDuration;                  // How long - in seconds - did it take to last connect to the Particle cloud
  uint16_t nextReportSeconds;                       // How often do we report ( on a node this is seconds - for a gateway it is minutes on the hour)
  uint16_t frequencyMinutes;                        // When we are reporing at minute increments - what are they - for Gateways
  uint8_t lastAlertCode;                            // Last alert to be reported (0 indicates no alert)
  unsigned long lastAlertTime;                      // When was the last alert
  bool verizonSIM;                                  // Are we using a Verizon SIM?
};
extern struct systemStatus_structure sysStatus;

// Current object values are captured at the moment
struct current_structure {                          // Where we store values in the current wake cycle
  uint8_t internalTempC;                              // Enclosure temperature in degrees C
  int stateOfCharge;                                // Battery charge level
  uint8_t batteryState;                             // Stores the current battery state (charging, discharging, etc)
  time_t lastSampleTime;                            // Timestamp of last data collection
  uint16_t rssi;                                    // Latest signal strength value
  uint8_t messageNumber;                            // What message are we on
  uint16_t hourly;                                  // Current Hourly Count
  uint16_t daily;                                   // Current Daily Count
};
extern struct current_structure current;

bool storageObjectStart();                          // Initialize the storage instance
bool storageObjectLoop();                           // Store the current and sysStatus objects
void loadSystemDefaults();                  // Initilize the object values for new deployments

#endif