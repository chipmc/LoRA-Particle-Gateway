#include "Particle.h"
#include "storage_objects.h"

namespace FRAM {                                    // Moved to namespace instead of #define to limit scope
  enum Addresses {
    versionAddr           = 0x00,                   // Version of the FRAM memory map
    systemStatusAddr      = 0x01,                   // Where we store the system status data structure
    currentStatusAddr     = 0x50                    // Where we store the current counts data structure
  };
}

const int FRAMversionNumber = 1;

// These two storage objects are initilized here and are external everywhere else
struct systemStatus_structure sysStatus;            // See structure definition in storage_objects.h
struct current_structure current;     

/**
 * @brief This function is executed in setup to initialize FRAM and load the storage objects from memory
 * 
 * @return true - Functions completed - values loaded or initilized if version number changed
 * @return false - Storage not initialized - need to resolve in an ERROR state
 */
bool storageObjectStart() {
    // Next we will load FRAM and check or reset variables to their correct values
  Log.info("Initializing the Object Store");
  fram.begin();                                     // Initialize the FRAM module
  byte tempVersion;
  fram.get(FRAM::versionAddr, tempVersion);         // Load the FRAM memory map version into a variable for comparison
  if (tempVersion != FRAMversionNumber) {           // Check to see if the memory map in the sketch matches the data on the chip
    Log.info("FRAM mismatch, erasing and locafing defaults if it checks out");
    fram.erase();                                   // Reset the FRAM to correct the issue
    fram.put(FRAM::versionAddr, FRAMversionNumber); // Put the right value in
    fram.get(FRAM::versionAddr, tempVersion);       // See if this worked
    if (tempVersion != FRAMversionNumber) {
      // Need to add an error handler here as the device will not work without FRAM will need to reset
      return false;
    }
    loadSystemDefaults();                           // Since we are re-initializing the storage objects, we need to set the right default values
  }
  else {
    Log.info("FRAM initialized, loading objects");
    fram.get(FRAM::systemStatusAddr,sysStatus);     // Loads the System Status array from FRAM
    fram.get(FRAM::currentStatusAddr,current);      // Loads the current values array from FRAM
  }

  return true;
}

/**
 * @brief In this function, we check each second to see if the values in the storage objects have changed
 * 
 * @return true - One or more of the hash values have changed - object written to FRAM
 * @return false - No change, nothing written to FRAM
 */

bool storageObjectLoop() {                          // Monitors the values of the two objects and writes to FRAM if changed after a second
  static time_t lastCheckTime = 0;
  static size_t lastSysStatusHash;
  static size_t lastCurrentHash;
  bool returnValue = false;

  if (Time.now() - lastCheckTime) {          // Check once a second
    lastCheckTime = Time.now();                     // Limit all this math to once a second
    size_t sysStatusHash = std::hash<uint16_t>{}(sysStatus.deviceID) + \
                      std::hash<uint16_t>{}(sysStatus.nodeNumber) + \
                      std::hash<byte>{}(sysStatus.structuresVersion) + \
                      std::hash<byte>{}(sysStatus.firmwareRelease)+ \
                      std::hash<bool>{}(sysStatus.verboseMode) + \
                      std::hash<bool>{}(sysStatus.solarPowerMode) + \
                      std::hash<bool>{}(sysStatus.lowPowerMode) + \
                      std::hash<int>{}(sysStatus.resetCount) + \
                      std::hash<uint32_t>{}(sysStatus.lastHookResponse) + \
                      std::hash<uint32_t>{}(sysStatus.lastConnection) + \
                      std::hash<uint16_t>{}(sysStatus.lastConnectionDuration) + \
                      std::hash<uint16_t>{}(sysStatus.frequencyMinutes) + \
                      std::hash<byte>{}(sysStatus.lastAlertCode)+ \
                      std::hash<uint32_t>{}(sysStatus.lastAlertTime) + \
                      std::hash<bool>{}(sysStatus.verizonSIM);
    if (sysStatusHash != lastSysStatusHash) {       // If hashes don't match write to FRAM
      Log.info("sysStaus object stored and hash updated");
      fram.put(FRAM::systemStatusAddr,sysStatus);
      lastSysStatusHash = sysStatusHash;
      returnValue = true;                           // In case I want to test whether values changed
    } 
    size_t currentHash =  std::hash<byte>{}(current.internalTempC) + \
                      std::hash<int>{}(current.stateOfCharge)+ \
                      std::hash<byte>{}(current.batteryState) + \
                      std::hash<time_t>{}(current.lastSampleTime) + \
                      std::hash<uint16_t>{}(current.rssi) + \
                      std::hash<uint8_t>{}(current.messageNumber) + \
                      std::hash<uint16_t>{}(current.hourly) + \
                      std::hash<uint16_t>{}(current.daily);
    if (currentHash != lastCurrentHash) {           // If hashes don't match write to FRAM
      Log.info("current object stored and hash updated");
      fram.put(FRAM::currentStatusAddr,current);
      lastCurrentHash = currentHash;
      returnValue = true;
    } 

  }
  return returnValue;
}


/**
 * @brief This function is called in setup if the version of the FRAM stoage map has been changed
 * 
 */
void loadSystemDefaults() {                         // This code is only executed with a new device or a new storage object structure
  if (Particle.connected()) {
    Particle.publish("Mode","Loading System Defaults", PRIVATE);
  }
  Log.info("Loading system defaults");              // Letting us know that defaults are being loaded
  sysStatus.structuresVersion = 1;
  sysStatus.firmwareRelease = 1;
  sysStatus.verboseMode = false;
  sysStatus.solarPowerMode = true;
  sysStatus.lowPowerMode = true;
  sysStatus.resetCount = 0;
  sysStatus.lastHookResponse = 0;
  sysStatus.frequencyMinutes = 60;
  sysStatus.lastAlertCode = 0;
  sysStatus.lastAlertTime = 0;
  sysStatus.verizonSIM = false;
}