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

// Gateway Functions
bool listenForLoRAMessageGateway();                // When we are in listening mode
bool deciperDataReportGateway();                   // Gateway - decodes data from report
bool acknowledgeDataReportGateway(int nextSeconds);// Gateway- acknowledged recipt of a data report

// Node Functions
bool composeDataReportNode();                  // Node - Composes data report
bool receiveAcknowledmentDataReportNode();     // Node - receives acknolwedgement


#endif