/**
 * @brief This function is called in setup to set the specific settings for the Gateway
 * 
 */

#include "Particle.h"
#include "MyPersistentData.h"

void setGatewayConfiguration() {
  Log.info("Setting values for the Gateway");
  sysStatus.set_sensorType(true); // Default is the car counter (true for PIR)
  // sysStatus.set_deviceID(32148);
  sysStatus.set_frequencyMinutes(10);
}