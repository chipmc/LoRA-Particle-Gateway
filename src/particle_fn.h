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
extern uint16_t updatedFrequencyMins;
extern char* batteryContext[8];
extern char openTimeStr[8];          
extern char closeTimeStr[8];         

// Functions
void particleInitialize();
int setFrequency(String command);
int setOpenTime(String command);     
int setCloseTime(String command);    
void makeUpStringMessages();
bool disconnectFromParticle();
int setVerizonSIM(String command);

#endif