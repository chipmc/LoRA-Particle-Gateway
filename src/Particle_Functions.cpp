#include "Particle.h"
#include "device_pinout.h"
#include "take_measurements.h"
#include "MyPersistentData.h"
#include "Particle_Functions.h"

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

Particle_Functions *Particle_Functions::_instance;

// [static]
Particle_Functions &Particle_Functions::instance() {
    if (!_instance) {
        _instance = new Particle_Functions();
    }
    return *_instance;
}

Particle_Functions::Particle_Functions() {
}

Particle_Functions::~Particle_Functions() {
}

void Particle_Functions::setup() {
    // Battery conect information - https://docs.particle.io/reference/device-os/firmware/boron/#batterystate-
    // const char* batteryContext[8] = {"Unknown","Not Charging","Charging","Charged","Discharging","Fault","Diconnected"};

    Log.info("Initializing Particle functions and variables");     // Note: Don't have to be connected but these functions need to in first 30 seconds
    Particle.variable("Reporting Frequency", sysStatus.get_frequencyMinutes());

    Particle.function("Set Frequency", &Particle_Functions::setFrequency, this);

    takeMeasurements();                               // Initialize sensor values
}

void Particle_Functions::loop() {
    // Put your code to run during the application thread loop here
}


int Particle_Functions::setFrequency(String command)
{
  char * pEND;
  char data[256];
  int tempTime = strtol(command,&pEND,10);                       // Looks for the first integer and interprets it
  if ((tempTime < 0) || (tempTime > 120)) return 0;   // Make sure it falls in a valid range or send a "fail" result
  sysStatus.set_updatedFrequencyMinutes(tempTime);
  snprintf(data, sizeof(data), "Report frequency will be set to %i minutes at next LoRA connect",sysStatus.get_updatedFrequencyMinutes());
  Log.info(data);
  if (Particle.connected()) Particle.publish("Time",data, PRIVATE);
  return 1;
}

bool Particle_Functions::disconnectFromParticle()                                          // Ensures we disconnect cleanly from Particle
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