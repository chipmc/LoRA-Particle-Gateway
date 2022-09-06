/**
 * @brief This function is called in setup to set the specific settings for each node
 * 
 */

#include "storage_objects.h"

void setNodeConfiguration() {
  sysStatus.sensorType = false; // Default is the car counter (true for PIR)
}