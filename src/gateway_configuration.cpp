/**
 * @brief This function is called in setup to set the specific settings for the Gateway
 * 
 */

#include "Particle.h"
#include "MyPersistentData.h"

void setGatewayConfiguration() {
  Log.info("Setting values for the Gateway");
  sysStatus.set_frequencyMinutes(10);
  sysStatus.set_magicNumber(27617);
}