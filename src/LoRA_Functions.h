/**
 * @file LoRA-Functions.h
 * @author Chip McClelland (chip@seeinisghts.com)
 * @brief This file allows me to move all the LoRA functionality out of the main program - application implementation / not general
 * @version 0.1
 * @date 2022-08-27
 * 
 */

// Particle functions
#ifndef LORA_FUNCTIONS_H
#define LORA_FUNCTIONS_H

#include "Particle.h"

// External Variables


// Functions
bool initializeLoRA();
bool listenForLoRAMessage();
bool sendLoRAMessage();
void flashTheLEDs();
bool deciperDataReport();
bool acknowledgeDataReport();
bool loRAStateMachine();

#endif