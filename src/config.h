/**
 * @file config.h
 * @brief Configuration file for the project
 * @details This file Hold configuration data to support development of the project in different locations and with
 * different hardware configurations.
 * @author Chip McClelland
 * @date 2024-09-24
 */


#ifndef CONFIG_H
#define CONFIG_H

// First, we may use a Boron with a Cellular radio, or an Argon with a WiFi radio
// This will determine which radio we use for signal strength and charging safety.
// Important differences between Boron and Argon: The Argon does not have a PMIC, so we can't disable charging based on temperature.
// The Argon does not have a cellular radio, so we can't get signal strength from the cellular radio.
// The Argon does not have a fuel guage so we can't get battery state or charge from the fuel guage.
// 0 = WiFi, 1 = Cellular, 2 = Serial Only 
// In serial only mode, all publishes will come to the serial console - Particle Functions via serial to be tried later
// In serial only mode, we will take the least common denominator approach and not sure PMIC or get signal strength
#define TRANSPORT_MODE 1

// We can get a lot of extra information in the serial terminal making it harder to follow what is going on.  
// This allows us to set the level of verbosity - All the Particle levels are here and I added one more - VERBOSE.
// INFO should be good for most uses, VERBOSE or ALL will cover the rest
// Levels - 0 - None, 1 - Error, 2 - Warn, 3 - Info, 4 - All = These are heirarchy so level 3 will get 0, 1 and 2 messages as well, 
#define SERIAL_LOG_LEVEL 3

// How long is the LORA Window for low power operations
#define DEFAULT_LORA_WINDOW 5
// How many minutes will the Gateway stay connected
#define STAY_CONNECTED 60

// How frequently will the gateway tell the nodes to report (note this value is re-applied every night at midnight)
#define DEFAULT_REPORTING_FREQUENCY_SECONDS 3600

// Next, the timezone setting for the gateway is set here to support developmnet in different locations.
// This will be used to set the time on the gateway device but - remember - nodes do not care about local time
// This is the timezone string from: https://github.com/rickkas7/LocalTimeRK/
// #define TIME_CONFIG "SGT-8"                                     // Uncomment for Singapore Time Zone
#define TIME_CONFIG "EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00"  // Uncomment for Eastern Time Zone


#endif // CONFIG_H