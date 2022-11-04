//Particle Functions
#include "Particle.h"
#include "particle_fn.h"
#include "device_pinout.h"
#include "take_measurements.h"
#include "MyPersistentData.h"

char openTimeStr[8] = " ";
char closeTimeStr[8] = " ";


// Prototypes and System Mode calls
SYSTEM_MODE(SEMI_AUTOMATIC);                        // This will enable user code to start executing automatically.
SYSTEM_THREAD(ENABLED);                             // Means my code will not be held up by Particle processes.
STARTUP(System.enableFeature(FEATURE_RESET_INFO));

// For monitoring / debugging, you have some options on the next few lines - uncomment one
//SerialLogHandler logHandler(LOG_LEVEL_TRACE);
//SerialLogHandler logHandler(LOG_LEVEL_ALL);         // All the loggings 
SerialLogHandler logHandler(LOG_LEVEL_INFO);     // Easier to see the program flow
// Serial1LogHandler logHandler1(57600);            // This line is for when we are using the OTII ARC for power analysis

bool frequencyUpdated = false;
uint16_t updatedFrequencyMins = 60;

/**
 * @brief Particle cacluated variable
 * 
 * @return String with the number of minutes for reporting frequency
 */
String reportFrequency() {							// Calculated variavble for the report frequency which is an unint16_t and does nto display properly.
    char reportStr[16];
    snprintf(reportStr, sizeof(reportStr), "%u minures", sysStatus.get_frequencyMinutes());
    return reportStr;
}

/**
 * @brief Initializes the Particle functions and variables
 * 
 * @details If new particles of functions are defined, they need to be initialized here
 * 
 */
void particleInitialize() {
  // Battery conect information - https://docs.particle.io/reference/device-os/firmware/boron/#batterystate-
  const char* batteryContext[8] = {"Unknown","Not Charging","Charging","Charged","Discharging","Fault","Diconnected"};

  Log.info("Initializing Particle functions and variables");     // Note: Don't have to be connected but these functions need to in first 30 seconds
  Particle.variable("Release",currentPointRelease);   
  Particle.variable("Signal", signalStr);
  Particle.variable("stateOfChg", current.get_stateOfCharge());
  Particle.variable("BatteryContext",batteryContext[current.get_batteryState()]);
  Particle.variable("Reporting Frequency", reportFrequency);
  Particle.variable("SIM Card", (sysStatus.get_verizonSIM()) ? "Verizon" : "Particle");
  Particle.variable("Open Time", openTimeStr);
  Particle.variable("Close Time", closeTimeStr);

  Particle.function("Set Frequency", setFrequency);
  Particle.function("Set Wake Time", setOpenTime);
  Particle.function("Set Sleep Time", setCloseTime);
  Particle.function("SIM Card", setVerizonSIM);

  takeMeasurements();                               // Initialize sensor values

  makeUpStringMessages();                           // Initialize the string messages needed for the Particle Variables
}

/**
 * @brief Sets the how often the device will report data in minutes.
 *
 * @details Extracts the integer from the string passed in, and sets the closing time of the facility
 * based on this input. Fails if the input is invalid.
 *
 * @param command A string indicating the number of minutes between reporting events.  Note, this function
 * sets an interim value for reporting frequency which takes effect once sent to a new node.
 *
 * @return 1 if able to successfully take action, 0 if invalid command
 */
int setFrequency(String command)
{
  char * pEND;
  char data[256];
  int tempTime = strtol(command,&pEND,10);                       // Looks for the first integer and interprets it
  if ((tempTime < 0) || (tempTime > 120)) return 0;   // Make sure it falls in a valid range or send a "fail" result
  updatedFrequencyMins = tempTime;
  frequencyUpdated = true;                            // Flag to change frequency after next connection to the nodes
  snprintf(data, sizeof(data), "Report frequency will be set to %i minutes at next LoRA connect",updatedFrequencyMins);
  Log.info(data);
  if (Particle.connected()) Particle.publish("Time",data, PRIVATE);
  return 1;
}


/**
 * @brief Sets the closing time of the facility.
 *
 * @details Extracts the integer from the string passed in, and sets the closing time of the facility
 * based on this input. Fails if the input is invalid.
 *
 * @param command A string indicating what the closing hour of the facility is in 24-hour time.
 * Inputs outside of "0" - "24" will cause the function to return 0 to indicate an invalid entry.
 *
 * @return 1 if able to successfully take action, 0 if invalid command
 */

int setOpenTime(String command)
{
  char * pEND;
  char data[64];
  int tempTime = strtol(command,&pEND,10);                             // Looks for the first integer and interprets it
  if ((tempTime < 0) || (tempTime > 23)) return 0;                     // Make sure it falls in a valid range or send a "fail" result
  sysStatus.set_openTime(tempTime);
  snprintf(data, sizeof(data), "Wake time set to %i",sysStatus.get_openTime());
  Log.info(data);
  if (Particle.connected()) {
    Particle.publish("Time",data, PRIVATE);
  }
  return 1;
}

/**
 * @brief Sets the closing time of the facility.
 *
 * @details Extracts the integer from the string passed in, and sets the closing time of the facility
 * based on this input. Fails if the input is invalid.
 *
 * @param command A string indicating what the closing hour of the facility is in 24-hour time.
 * Inputs outside of "0" - "24" will cause the function to return 0 to indicate an invalid entry.
 *
 * @return 1 if able to successfully take action, 0 if invalid command
 */

int setCloseTime(String command)
{
  char * pEND;
  char data[64];
  int tempTime = strtol(command,&pEND,10);                       // Looks for the first integer and interprets it
  if ((tempTime < 0) || (tempTime > 24)) return 0;   // Make sure it falls in a valid range or send a "fail" result
  sysStatus.set_closeTime(tempTime);
  snprintf(data, sizeof(data), "Sleep time set to %i",sysStatus.get_closeTime());
  Log.info(data);
  if (Particle.connected()) {
    Particle.publish("Time",data, PRIVATE);
  }
  return 1;
}


/**
 * @brief Set the Verizon SIM
 * 
 * @param command  - 1 for Verizon and 0 for Particle (default)
 * @return int 
 */
int setVerizonSIM(String command)                                   // If we are using a Verizon SIM, we will need to execute "keepAlive" calls in the main loop when not in low power mode
{
  if (command == "1")
  {
    sysStatus.set_verizonSIM(true);
    Particle.keepAlive(60);                                         // send a ping every minute
    if (Particle.connected()) Particle.publish("Mode","Set to Verizon SIM", PRIVATE);
    return 1;
  }
  else if (command == "0")
  {
    sysStatus.set_verizonSIM(false);
    Particle.keepAlive(23 * 60);                                     // send a ping every 23 minutes
    if (Particle.connected()) Particle.publish("Mode","Set to Particle SIM", PRIVATE);
    return 1;
  }
  else return 0;
}

 /**
  * @brief Simple Function to construct the strings that make the console easier to read
  * 
  * @details Looks at all the system setting values and creates the appropriate strings.  Note that this 
  * is a little inefficient but it cleans up a fair bit of code.
  * 
  */

void makeUpStringMessages() {

  if (sysStatus.get_openTime() == 0 && sysStatus.get_closeTime() == 24) {                         // Special case for 24 hour operations
    snprintf(openTimeStr, sizeof(openTimeStr), "NA");
    snprintf(closeTimeStr, sizeof(closeTimeStr), "NA");
  }
  else {
    snprintf(openTimeStr, sizeof(openTimeStr), "%i:00", sysStatus.get_openTime());           // Open and Close Times
    snprintf(closeTimeStr, sizeof(closeTimeStr), "%i:00", sysStatus.get_closeTime());
  }

  return;
}



/**
 * @brief Disconnect from Particle function has one purpose - to disconnect the Boron from Particle, the cellular network and to power down the cellular modem to save power
 * 
 * @details This function is supposed to be able to handle most issues and tee up a power cycle of the Boron and the Cellular Modem if things do not go as planned
 * 
 * @return true - Both disconnect from Particle and the powering down of the cellular modem are successful
 * @return false - One or the other failed and the device will go to the ERROR_STATE to be power cycled.
 */

bool disconnectFromParticle()                                          // Ensures we disconnect cleanly from Particle
                                                                       // Updated based on this thread: https://community.particle.io/t/waitfor-particle-connected-timeout-does-not-time-out/59181
{
  time_t startTime = Time.now();
  Log.info("In the disconnect from Particle function");
  detachInterrupt(BUTTON_PIN);                                         // Stop watching the userSwitch as we will no longer be connected
  // First, we need to disconnect from Particle
  Particle.disconnect();                                               // Disconnect from Particle
  waitForNot(Particle.connected, 15000);                               // Up to a 15 second delay() 
  Particle.process();
  if (Particle.connected()) {                      // As this disconnect from Particle thing can be a·syn·chro·nous, we need to take an extra step to wait, 
    Log.info("Failed to disconnect from Particle");
    return(false);
  }
  else Log.info("Disconnected from Particle in %i seconds", (int)(Time.now() - startTime));
  // Then we need to disconnect from Cellular and power down the cellular modem
  startTime = Time.now();
  Cellular.disconnect();                                               // Disconnect from the cellular network
  Cellular.off();                                                      // Turn off the cellular modem
  waitFor(Cellular.isOff, 30000);                                      // As per TAN004: https://support.particle.io/hc/en-us/articles/1260802113569-TAN004-Power-off-Recommendations-for-SARA-R410M-Equipped-Devices
  Particle.process();
  if (Cellular.isOn()) {                                               // At this point, if cellular is not off, we have a problem
    Log.info("Failed to turn off the Cellular modem");
    return(false);                                                     // Let the calling function know that we were not able to turn off the cellular modem
  }
  else {
    Log.info("Turned off the cellular modem in %i seconds", (int)(Time.now() - startTime));
    return true;
  }
}