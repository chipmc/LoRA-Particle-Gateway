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
bool initializeLoRA();                      // Initialize the LoRA Radio
bool listenForLoRAMessage();                // When we are in listening mode
bool sendLoRAMessage();                     // When we are acknowledging a message
bool loRAStateMachine();                    // This is the logic of which message and which acknowledgement we need to send
// All these are specific to our three message types - join, data and alert
bool deciperDataReport();   
bool acknowledgeDataReport();


#endif