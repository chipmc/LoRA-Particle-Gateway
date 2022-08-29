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
bool initializeLoRA(bool gatewayID);                      // Initialize the LoRA Radio

// Generic Gateway Functions
bool listenForLoRAMessageGateway();                       // When we are in listening mode
bool respondForLoRAMessageGateway(int nextSeconds);       // When we are responding

// Specific Gateway Message Functions
bool decipherDataReportGateway();                            // Gateway - decodes data from a data report
bool decipherJoinRequestGateway();                          // Gateway - decodes data from a join request
bool acknowledgeDataReportGateway(int nextSeconds);    // Gateway- acknowledged receipt of a data report
bool acknowledgeJoinRequestGateway(int nextSeconds);   // Gateway - acknowledged receipt of a join request

// Node Functions
bool composeDataReportNode();                  // Node - Composes data report
bool receiveAcknowledmentDataReportNode();     // Node - receives acknolwedgement
bool composeJoinRequesttNode();                // Node - Composes Join request
bool receiveAcknowledmentJoinRequestNode();    // Node - received join request asknowledgement

#endif