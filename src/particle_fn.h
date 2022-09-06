/**
 * @file particle_fn.h
 * @author Chip McClelland (chip@seeinisghts.com)
 * @brief This file initilizes the Particle functions and variables needed for control from the console / API calls
 * @version 0.1
 * @date 2022-06-30
 * 
 */

// Particle functions
#ifndef PARTICLE_FN_H
#define PARTICLE_FN_H

#include "Particle.h"

// Variables
extern char currentPointRelease[6];
extern bool frequencyUpdated;
extern char* batteryContext[8];
extern char sensorTypeConfigStr[16];
// extern char wakeTimeStr[8];          // May add this functionality later
// extern char sleepTimeStr[8];         // May add this functionality later

void particleInitialize();
int setFrequency(String command);
// int setWakeTime(String command);     // May add this functionality later
// int setSleepTime(String command);    // May add this functionality later
int setLowPowerMode(String command);
void makeUpStringMessages();
bool disconnectFromParticle();
bool setSensorType();                  // Particle function
int setVerizonSIM(String command);

#endif