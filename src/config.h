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
// 0 = Argon, 1 = Boron
#define CELLULAR_RADIO 1

// Next, the timezone setting for the gateway is set here to support developmnet in different locations.
// This will be used to set the time on the gateway device but - remember - nodes do not care about local time
// This is the timezone string from: https://github.com/rickkas7/LocalTimeRK/
// #define TIME_CONFIG "SGT-8"                                     // Uncomment for Singapore Time Zone
#define TIME_CONFIG "EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00"  // Uncomment for Eastern Time Zone



#endif // CONFIG_H