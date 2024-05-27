#include "Particle.h"
#include "device_pinout.h"
#include "take_measurements.h"
#include "MyPersistentData.h"
#include "Particle_Functions.h"
#include "PublishQueuePosixRK.h"
#include "JsonDataManager.h"
#include "Room_Occupancy.h"
#include "JsonParserGeneratorRK.h"

char openTimeStr[8] = " ";
char closeTimeStr[8] = " ";

// Prototypes and System Mode calls
SYSTEM_MODE(SEMI_AUTOMATIC);                        // This will enable user code to start executing automatically.
SYSTEM_THREAD(ENABLED);                             // Means my code will not be held up by Particle processes.
STARTUP(System.enableFeature(FEATURE_RESET_INFO));

SerialLogHandler logHandler(LOG_LEVEL_INFO);        // Easier to see the program flow

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
  Log.info("Initializing Particle functions and variables");     // Note: Don't have to be connected but these functions need to in first 30 seconds
  Particle.function("Commands", &Particle_Functions::jsonFunctionParser, this);
}


void Particle_Functions::loop() {
  // Put your code to run during the application thread loop here
}

int Particle_Functions::jsonFunctionParser(String command) {
  // const char * const commandString = "{\"cmd\":[{\"node\":1,\"var\":\"hourly\",\"fn\":\"reset\"},{\"node\":0,\"var\":1,\"fn\":\"lowpowermode\"},{\"node\":2,\"var\":\"daily\",\"fn\":\"report\"}]}";
  // String to put into Uber command window {"cmd":[{"node":1,"var":"hourly","fn":"reset"},{"node":0,"var":1,"fn":"lowpowermode"},{"node":2,"var":"daily","fn":"report"}]}

	uint32_t nodeUniqueID;
  int nodeNumber;
	String variable;
	String function;
  char * pEND;
  char messaging[64];
  bool success = true;  
  bool invalidCommand = false;

	JsonParserStatic<1024, 80> jp;	// Global parser that supports up to 256 bytes of data and 20 tokens

  Log.info(command.c_str());

	jp.clear();
	jp.addString(command);
	if (!jp.parse()) {
		Log.info("Parsing failed - check syntax");
    PublishQueuePosix::instance().publish("cmd", "Parsing failed - check syntax",PRIVATE);
		char data[128];  
    snprintf(data, sizeof(data), "{\"commands\":%i,\"context\":\"%s,\"timestamp\":%lu000 }", -1, command.c_str(), Time.now());        // Send -1 (Syntax Error) to the 'commands' Synthetic Variable
    PublishQueuePosix::instance().publish("Ubidots_Command_Hook", data, PRIVATE);
    snprintf(data, sizeof(data), "{\"commands\":%i,\"context\":\"%s\",\"timestamp\":%lu000 }", -10, command.c_str(), Time.now());    // Send -10, resolve any events
    PublishQueuePosix::instance().publish("Ubidots_Command_Hook", data, PRIVATE);
		return 0;
	}

	const JsonParserGeneratorRK::jsmntok_t *cmdArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "cmd", cmdArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *cmdObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i=0; i<10; i++) {												// Iterate through the array looking for a match
		cmdObjectContainer = jp.getTokenByIndex(cmdArrayContainer, i);
		if(cmdObjectContainer == NULL) {
      if (i == 0) return 0;                                       // No valid entries
			else break;								                    // Ran out of entries 
		} 
		jp.getValueByKey(cmdObjectContainer, "node", nodeUniqueID);
    nodeNumber = JsonDataManager::instance().getNodeNumberForUniqueID(nodeUniqueID); // nodeNumber is uniqueID
		jp.getValueByKey(cmdObjectContainer, "var", variable);
		jp.getValueByKey(cmdObjectContainer, "fn", function);

    const JsonParserGeneratorRK::jsmntok_t *varArrayContainer;      // Token for the objects in the var array (if it is an array)
    jp.getValueTokenByKey(cmdObjectContainer, "var", varArrayContainer);

    // In this section we will parse and execute the commands from the console or JSON - assumes connection to Particle
    // ****************  Note: currently there is no valudiation on the nodeNumbers ***************************
    
    // Reset Function
		if (function == "reset") {
      // Format - function - reset, node - nodeNumber, variables - either "current", "all" or "nodeData"
      // Test - {"cmd":[{"node":1,"var":"all","fn":"reset"}]}
      if (nodeUniqueID == 0) {        // if the unique ID passed to the node is "0", we are talking about the gateway
        if (variable == "nodeData") {
          snprintf(messaging,sizeof(messaging),"Resetting the gateway's node Data");
          nodeDatabase.resetNodeIDs();
          Log.info("Resetting the Gateway node so new database is in effect");
          PublishQueuePosix::instance().publish("Alert","Resetting Gateway",PRIVATE);
          delay(2000);
          System.reset();
        }
        else if (variable == "all") {
            snprintf(messaging,sizeof(messaging),"Resetting the gateway's system and current data");
            sysStatus.initialize();                     // All will reset system values as well
            nodeDatabase.initialize();
        }
        else snprintf(messaging,sizeof(messaging),"Resetting the gateway's current data");
        sysStatus.set_messageCount(0);                  // Reset the message count
        sysStatus.set_alertCodeGateway(20);              // Alert code 20 will reset the current data on the gateway
        sysStatus.set_resetCount(0);
      } 
      else if(nodeNumber != 0) {                        // if we could not set the nodeNumber from that unique ID, throw an error
        if (variable == "all") {
          snprintf(messaging,sizeof(messaging),"Resetting node %d's system and current data", nodeNumber);
          JsonDataManager::instance().resetAllDataForNode(nodeNumber);
        }
        else {
          snprintf(messaging,sizeof(messaging),"Resetting node %d's current data", nodeNumber);
          JsonDataManager::instance().resetCurrentDataForNode(nodeNumber);
        }
      } else {
        snprintf(messaging,sizeof(messaging),"Not a valid node uniqueID");
      }
    }

    // Reporting Frequency Function
    else if (function == "freq") {   
      // Format - function - freq, node - 0, variables - 2-60 (must be divisiable by two)
      // Test - {"cmd":[{"node":0,"var":"5","fn":"freq"}]}
      int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
      if ((tempValue > 0) && (tempValue <= 60) && 60 % tempValue == 0) {
        snprintf(messaging,sizeof(messaging),"Setting reporting frequency to %d minutes", tempValue);
        sysStatus.set_updatedfrequencySeconds(tempValue * 60UL);
      }
      else {
        snprintf(messaging,sizeof(messaging),"Not a valid reporting frequency");
        success = false;                                                       // Make sure it falls in a valid range or send a "fail" result
      }
    }

    // Stay Connected
    else if (function == "stay") {
      // Format - function - stay, node - 0, variables - true or false
      // Test - {"cmd":[{"node":0,"var":"true" or "false","fn":"stay"}]}
      if (variable == "true") {
        snprintf(messaging,sizeof(messaging),"Going to keep Gateway on Particle and LoRA networks");
        sysStatus.set_connectivityMode(1);
      }
      else {
        snprintf(messaging,sizeof(messaging),"Going back to normal connectivity");
        sysStatus.set_connectivityMode(0);                            // Make sure we are set to not connect on resetart.
        sysStatus.flush(true);    
        Particle_Functions::disconnectFromParticle();                 // Can't reset if modem is powered up
        System.reset();                                               // Needed to disconnect from LoRA
      }
    }

    // Node ID Report
    else if (function == "rpt") {
      // Format - function - rpt, node - 0, variables - NA
      // Test - {"cmd":[{"node":0,"var":" ","fn":"rpt"}]}
      snprintf(messaging,sizeof(messaging),"Printing nodeID Data");
      JsonDataManager::instance().printNodeData(true);
    }

    // Sets Opening hour
    else if (function == "open") {
      // Format - function - open, node - 0, variables - 0-12 open hour
      // Test - {"cmd":[{"node":0, "var":"6","fn":"open"}]}
      int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
      if ((tempValue >= 0) && (tempValue <= 12)) {
        snprintf(messaging,sizeof(messaging),"Setting opening hour to %d:00", tempValue);
        sysStatus.set_openTime(tempValue);
      }
      else {
        snprintf(messaging,sizeof(messaging),"Open hour - must be 0-12");
        success = false;                                                       // Make sure it falls in a valid range or send a "fail" result
      }
    }

    // Sets Closing hour
    else if (function == "close") {
      // Format - function - close, node - 0, variables - 13-24 closing hour
      // Test - {"cmd":[{"node":0, "var":"21","fn":"close"}]}
      int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
      if ((tempValue >= 13 ) && (tempValue <= 24)) {
        snprintf(messaging,sizeof(messaging),"Setting closing hour to %d:00", tempValue);
        sysStatus.set_closeTime(tempValue);
      }
      else {
        snprintf(messaging,sizeof(messaging),"Close hour - must be 13-24");
        success = false;                                                       // Make sure it falls in a valid range or send a "fail" result
      }
    }

    // Sets break time (hour)
    else if (function == "break") {
      // Format - function - open, node - 0, variables - 0-23 break start hour (set to 24 to dedenote having no break)
      // Test - {"cmd":[{"node":0, "var":"2","fn":"break"}]}
      int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
      if ((tempValue >= 0) && (tempValue <= 24)) {
        snprintf(messaging,sizeof(messaging),"Setting break start hour to %d:00", tempValue);
        sysStatus.set_breakTime(tempValue);
      }
      else {
        snprintf(messaging,sizeof(messaging),"Break start hour - must be 0-24");
        success = false;                                               // Make sure it falls in a valid range or send a "fail" result
      }
    }

    // Sets break length in minutes
    else if (function == "breakLengthMinutes") {
      // Format - function - close, node - 0, variables - 0-60 break length (in minutes)
      // Test - {"cmd":[{"node":0, "var":"21","fn":"breakLengthMinutes"}]}
      int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
      if ((tempValue >= 0 ) && (tempValue <= 240)) {
        snprintf(messaging,sizeof(messaging),"Setting break length to %d minutes", tempValue);
        sysStatus.set_breakLengthMinutes(tempValue);
      }
      else {
        snprintf(messaging,sizeof(messaging),"Break length (minutes) - must be 0-240");
        success = false;                                               // Make sure it falls in a valid range or send a "fail" result
      }
    }

    // Sets weekend break time (hour)
    else if (function == "weekendBreak") {
      // Format - function - open, node - 0, variables - 0-23 break start hour (set to 24 to dedenote having no break)
      // Test - {"cmd":[{"node":0, "var":"2","fn":"break"}]}
      int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
      if ((tempValue >= 0) && (tempValue <= 24)) {
        snprintf(messaging,sizeof(messaging),"Setting weekend break start hour to %d:00", tempValue);
        sysStatus.set_weekendBreakTime(tempValue);
      }
      else {
        snprintf(messaging,sizeof(messaging),"Weekend break start hour - must be 0-24");
        success = false;                                               // Make sure it falls in a valid range or send a "fail" result
      }
    }

    // Sets weekend break length in minutes
    else if (function == "weekendBreakLengthMinutes") {
      // Format - function - close, node - 0, variables - 0-60 break length (in minutes)
      // Test - {"cmd":[{"node":0, "var":"21","fn":"breakLengthMinutes"}]}
      int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
      if ((tempValue >= 0 ) && (tempValue <= 240)) {
        snprintf(messaging,sizeof(messaging),"Setting weekend break length to %d minutes", tempValue);
        sysStatus.set_weekendBreakLengthMinutes(tempValue);
      }
      else {
        snprintf(messaging,sizeof(messaging),"Weekend break length (minutes) - must be 0-240");
        success = false;                                               // Make sure it falls in a valid range or send a "fail" result
      }
    }

    // Power Cycle the Device
    else if (function == "pwr") {
      // Format - function - pwr, node - 0, variables - 1
      // Test - {"cmd":[{"node":0, "var":"1","fn":"pwr"}]}
      int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
      if (tempValue == 1) {
        snprintf(messaging,sizeof(messaging),"Setting Alert Code to Trigger Reset");
        sysStatus.set_alertCodeGateway(1);
        sysStatus.set_alertTimestampGateway(Time.now());
      }
      else {
        snprintf(messaging,sizeof(messaging),"Power Cycle value not = 1)");
        success = false;                                                       // Make sure it falls in a valid range or send a "fail" result
      }
    }

    // Sets the sensor type for a node
    else if (function == "type") {
      // Format - function - type, node - node uniqueID, variables 
        // - 0 (Vehicle Pressure Sensor)
        // - 1 (Pedestrian Infrared Sensor)
        // - 2 (Vehicle Magnetometer Sensor)
        // - 3 (Rain Sensor)
        // - 4 (Vibration / Motion Sensor - Basic)
        // - 5 (Vibration Sensor - Advanced)
        // - 10 (Indoor Room Occupancy Sensor)
        // - 11 (Outdoor Occupancy Sensor)
        // - 20 (Soil Moisture Sensor)
        // - 21 (Distance Sensor)
      // Test - {"cmd":[{"node":"3312487035", "var":"1","fn":"type"}]}
      if(nodeNumber != 0) {
        int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
        if ((tempValue >= 0 ) && (tempValue <= 29)) {                    // See - https://seeinsights.freshdesk.com/support/solutions/articles/154000101712-sensor-types-and-identifiers
          snprintf(messaging,sizeof(messaging),"Setting sensor type to %d for node %d", tempValue, nodeNumber);
          if (!JsonDataManager::instance().setType(nodeNumber,tempValue)) success = false;  // Make a new entry in the nodeID database
          else JsonDataManager::instance().setAlertCode(nodeNumber,1);        // Forces the node to update its sensor Type by setting it and triggering rejoin
        }
        else {
          snprintf(messaging,sizeof(messaging),"Sensor Type - must be 0-29");
          success = false;                                               // Make sure it falls in a valid range or send a "fail" result
        }
      } else {
        snprintf(messaging,sizeof(messaging),"No node exists in the database with that uniqueID");
        success = false; 
      }
    }

    // Designates the space number (the room/area it is counting for) of an Occupancy Sensor (device type >= 10)
    else if (function == "mountConfig") {
      // Format - function - mountConfig, node - node uniqueID, variable - [INT (space), BOOL (placement), BOOL (multi)] 
      // Test - {"cmd":[{"var":["31","true","true"],"fn":"mountConfig","node":3312487035}]}
      if (varArrayContainer->type == JsonParserGeneratorRK::JSMN_ARRAY) {   // Check if "var" is an array and if nodeNumber is erroneous
              int spaceInt;
              uint8_t placement; String placementStr;
              uint8_t multi; String multiStr;
              // int nothing;                                       // reserved for future use
              jp.getValueByIndex(varArrayContainer, 0, spaceInt);
              jp.getValueByIndex(varArrayContainer, 1, placementStr);
              jp.getValueByIndex(varArrayContainer, 2, multiStr);
              //jp.getValueByIndex(varArrayContainer, 3, nothing);     // reserved for future use
              uint8_t space = static_cast<uint8_t>(spaceInt);
              if(nodeNumber != 0) {
                if(JsonDataManager::instance().getJoinPayload(nodeNumber)) {
                  if(space >= 1 && space <= 64) {                            // People don't like a space to be zero so they start at one
                    current.set_payload1(space - 1);                              // Store it as a 6 bit anyway though (This allows us to index space as 0-63 in the app, while displaying space + 1 to Ubidots/Particle)
                    if(placementStr != "null"){                              // null is sent here when blank on ubidots widget - ignoring reduces punishment of user error
                      placement = placementStr == "true" ? 1 : 0;
                      current.set_payload2(placement);
                    }
                    if(multiStr != "null"){                                   // null is sent here when blank on ubidots widget - ignoring reduces punishment of user error
                      multi = multiStr == "true" ? 1 : 0;
                      current.set_payload3(multi);
                    }
                    snprintf(messaging,sizeof(messaging), "Set payload for node %d. space: %d, placement: %s, multi: %s", nodeNumber, space, placementStr.c_str(), multiStr.c_str());
                    JsonDataManager::instance().setJoinPayload(nodeNumber);
                    JsonDataManager::instance().setAlertCode(nodeNumber, 1);    // trigger a rejoin alert code               
                  } else {
                    snprintf(messaging,sizeof(messaging), "Error in mountConfig. \"Space\" must be between 1 and 64.");
                    success = false;
                  }
                } else {
                  snprintf(messaging,sizeof(messaging), "Error in mountConfig. Could not set payload.");
                  success = false;
                }
              } else {
                snprintf(messaging,sizeof(messaging), "No node exists in the database with that uniqueID");
                success = false;
              }
      } else {
          snprintf(messaging,sizeof(messaging), "Error executing mountConfig - Var was not an array");
          success = false;
      }
    }

    // Sets the zoneMode for an occupancy node's SPAD configuration
    else if (function == "zoneMode") {
      // Test - {"cmd":[{"node":3312487035, "var":"1","fn":"zoneMode"}]}
      if(nodeNumber != 0) {
        int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
        if ((tempValue >= 0 ) && (tempValue <= 4)) {                   
          snprintf(messaging,sizeof(messaging),"Setting zone mode to %d for node %d", tempValue, nodeNumber);
          JsonDataManager::instance().setAlertCode(nodeNumber,7);
          JsonDataManager::instance().setAlertContext(nodeNumber,tempValue);  // Forces the node to update its zone mode by setting an alert code and sending the value as context 
        }
        else {
          snprintf(messaging,sizeof(messaging),"Zone Mode must be 0-4");
          success = false;                                                   // Make sure it falls in a valid range or send a "fail" result
        }
      } else {
        snprintf(messaging,sizeof(messaging),"No node exists in the database with that uniqueID");
        success = false; 
      }
    }

    // Sets the distanceMode for an occupancy node
    else if (function == "distanceMode") {
      // Test - {"cmd":[{"node":3312487035, "var":"1","fn":"distanceMode"}]}
      if(nodeNumber != 0) {
        int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
        if ((tempValue >= 0 ) && (tempValue <= 2)) {                   
          snprintf(messaging,sizeof(messaging),"Setting distanceMode to %d for node %d", tempValue, nodeNumber);
          JsonDataManager::instance().setAlertCode(nodeNumber,8);
          JsonDataManager::instance().setAlertContext(nodeNumber,tempValue);  // Forces the node to update its floor interference buffer by setting an alert code and sending the value as context 
        }
        else {
          snprintf(messaging,sizeof(messaging),"distanceMode must be 0 (short), 1(medium) or 2(long)");
          success = false;                                                   // Make sure it falls in a valid range or send a "fail" result
        }
      } else {
        snprintf(messaging,sizeof(messaging),"No node exists in the database with that uniqueID");
        success = false; 
      }
    }

    // Sets the interferenceBuffer for a node
    else if (function == "interferenceBuffer") {
      // Test - {"cmd":[{"node":3312487035, "var":"150","fn":"interferenceBuffer"}]}
      if(nodeNumber != 0) {
        int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
        if ((tempValue >= 0 ) && (tempValue <= 2000)) {                   
          snprintf(messaging,sizeof(messaging),"Setting interferenceBuffer to %d for node %d", tempValue, nodeNumber);
          JsonDataManager::instance().setAlertCode(nodeNumber,9);
          JsonDataManager::instance().setAlertContext(nodeNumber,tempValue);  // Forces the node to update its floor interference buffer by setting an alert code and sending the value as context 
        }
        else {
          snprintf(messaging,sizeof(messaging),"Floor Interference Buffer must be 0-2000mm");
          success = false;                                                   // Make sure it falls in a valid range or send a "fail" result
        }
      } else {
        snprintf(messaging,sizeof(messaging),"No node exists in the database with that uniqueID");
        success = false; 
      }
    }

    // Sets the number of calibration loops for an occupancy node
    else if (function == "occupancyCalibrationLoops") {
      // Test - {"cmd":[{"node":3312487035, "var":"50","fn":"occupancyCalibrationLoops"}]}
      if(nodeNumber != 0) {
        int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
        if ((tempValue >= 0 ) && (tempValue <= 1000)) {                   
          snprintf(messaging,sizeof(messaging),"Setting occupancyCalibrationLoops to %d for node %d", tempValue, nodeNumber);
          JsonDataManager::instance().setAlertCode(nodeNumber,10);
          JsonDataManager::instance().setAlertContext(nodeNumber,tempValue);  // Forces the node to update its floor interference buffer by setting an alert code and sending the value as context 
        }
        else {
          snprintf(messaging,sizeof(messaging),"the number of calibration loops must be 0-1000");
          success = false;                                                   // Make sure it falls in a valid range or send a "fail" result
        }
      } else {
        snprintf(messaging,sizeof(messaging),"No node exists in the database with that uniqueID");
        success = false; 
      }
    }

    // Sets the number of calibration loops for an occupancy node
    else if (function == "recalibrate") {
      // Test - {"cmd":[{"node":3312487035, "var":"true","fn":"recalibrate"}]}
      if(nodeNumber != 0) {
        if (variable == "true") {                   
          snprintf(messaging,sizeof(messaging),"Initiating recalibration for node %d", nodeNumber);
          JsonDataManager::instance().setAlertCode(nodeNumber,11);
        } else {
          snprintf(messaging,sizeof(messaging),"Incorrect value for var. Must be \"true\"");
          success = false; 
        }
      } else {
        snprintf(messaging,sizeof(messaging),"No node exists in the database with that uniqueID");
        success = false; 
      }
    }

    // Resets the Room Occupancy numbers
    else if (function == "resetRoomCounts") {
      // Test - {"cmd":[{"node":3312487035, "var":"true","fn":"recalibrate"}]}
      if(nodeNumber == 0) {
        if (variable == "all") {                   
          snprintf(messaging,sizeof(messaging),"Resetting Room gross AND net counts");
          Room_Occupancy::instance().resetAllCounts();
        }
        if (variable == "net") {                   
          snprintf(messaging,sizeof(messaging),"Resetting Room net counts");
          Room_Occupancy::instance().resetNetCounts();
        } else {
          snprintf(messaging,sizeof(messaging),"Must enter \"all\" or \"net\" for var");
          success = false; 
        }
      } else {
        snprintf(messaging,sizeof(messaging),"Can only reset counts for Gateway (node 0)");
        success = false; 
      }
    }

    // Resets the Room Occupancy numbers
    else if (function == "resetSpace") {
      // Test - {"cmd":[{"node":0, "var":"1","fn":"resetSpace"}]}
      if(nodeNumber == 0) {
        int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
        if ((tempValue >= 1 ) && (tempValue <= 64)) {                   
          snprintf(messaging,sizeof(messaging),"Resetting Space %d", tempValue);
          JsonDataManager::instance().resetSpace(tempValue - 1); // Send the index of the space rather than the space number. Ex. space 1 is index 0
        } else {
          snprintf(messaging,sizeof(messaging),"Space number must be between 1 and 64");
          success = false; 
        }
      } else {
        snprintf(messaging,sizeof(messaging),"Can only reset spaces through Gateway (node 0)");
        success = false; 
      }
    }

    // Sets the net count for a node manually
    else if (function == "setOccupancyNetForNode") {
      // Test - {"cmd":[{"node":3312487035, "var":"5","fn":"setNodeNetCount"}]}
      if(nodeNumber != 0) {
        int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
        snprintf(messaging,sizeof(messaging),"Setting net occupancy to %d for node %d", tempValue, nodeNumber);
        Room_Occupancy::instance().setOccupancyNetForNode(nodeNumber, tempValue);
      } else {
        snprintf(messaging,sizeof(messaging),"No node exists in the database with that uniqueID");
        success = false; 
      }
    }

    // Sets the TOF sensor detections per second for a node 
    else if (function == "setTofDetectionsPerSecond") {
      // Test - {"cmd":[{"node":3312487035, "var":"5","fn":"setTofDetectionsPerSecond"}]}
      if(nodeNumber != 0) {
        int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
        if ((tempValue >= 1 ) && (tempValue <= 30)) {                   
          snprintf(messaging,sizeof(messaging), "Setting tofDetectionsPerSecond to %d/second for node %d", tempValue, nodeNumber);
          JsonDataManager::instance().setAlertCode(nodeNumber,13);
          JsonDataManager::instance().setAlertContext(nodeNumber,tempValue);  // Forces the node to update its tofPollingRateMS by setting an alert code and sending the value as context 
        }
        else {
          snprintf(messaging,sizeof(messaging),"tofDetectionsPerSecond must be 1-30");
          success = false;                                                   // Make sure it falls in a valid range or send a "fail" result
        }
      } else {
        snprintf(messaging,sizeof(messaging),"No node exists in the database with that uniqueID");
        success = false; 
      }
    }

    // Sets the TOF sensor detections per second for a node 
    else if (function == "setTransmitLatencySeconds") {
      // Test - {"cmd":[{"node":3312487035, "var":"60","fn":"setTransmitLatencySeconds"}]}
      if(nodeNumber != 0) {
        int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
        if ((tempValue >= 1 ) && (tempValue <= 60)) {                   
          snprintf(messaging,sizeof(messaging), "Setting tofDetectionsPerSecond to %dms for node %d", tempValue, nodeNumber);
          JsonDataManager::instance().setAlertCode(nodeNumber,14);
          JsonDataManager::instance().setAlertContext(nodeNumber,tempValue);  // Forces the node to update its tofPollingRateMS by setting an alert code and sending the value as context 
        }
        else {
          snprintf(messaging,sizeof(messaging),"transmitLatencySeconds must be 1-60");
          success = false;                                                   // Make sure it falls in a valid range or send a "fail" result
        }
      } else {
        snprintf(messaging,sizeof(messaging),"No node exists in the database with that uniqueID");
        success = false; 
      }
    }

    // What if none of these functions are recognized
    else {
      snprintf(messaging,sizeof(messaging),"Not a valid command");
      success = false;
      invalidCommand = true;
    }
    
    if (!(strncmp(messaging," ",1) == 0)) {
      Log.info(messaging);
      if (Particle.connected()) PublishQueuePosix::instance().publish("cmd",messaging,PRIVATE);
    }
  }

  if (Particle.connected()){
    // char configData[256]; // Store the configuration data in this character array - not global
    // snprintf(configData, sizeof(configData), "{\"timestamp\":%lu000, \"power\":\"%s\", \"lowPowerMode\":\"%s\", \"timeZone\":\"" + sysStatus.get_timeZoneStr() + "\", \"open\":%i, \"close\":%i, \"sensorType\":%i, \"verbose\":\"%s\", \"connecttime\":%i, \"battery\":%4.2f}", Time.now(), sysStatus.get_solarPowerMode() ? "Solar" : "Utility", sysStatus.get_lowPowerMode() ? "Low Power" : "Not Low Power", sysStatus.get_openTime(), sysStatus.get_closeTime(), sysStatus.get_sensorType(), sysStatus.get_verboseMode() ? "Verbose" : "Not Verbose", sysStatus.get_lastConnectionDuration(), current.get_stateOfCharge());
    // PublishQueuePosix::instance().publish("Send-Configuration", configData, PRIVATE | WITH_ACK);                                    // Send new configuration to FleetManager backend. (v1.4)
    if(success == true){           // send the Success slack notification if the command was not recognized
      char data[128];
      snprintf(data, sizeof(data), "{\"commands\":%i,\"context\":\"%s\",\"timestamp\":%lu000 }", 1, function.c_str(), Time.now());    // Send 1 (Execution Success) to the 'commands' Synthetic Variable
      PublishQueuePosix::instance().publish("Ubidots_Command_Hook", data, PRIVATE);
      snprintf(data, sizeof(data), "{\"commands\":%i,\"context\":\"%s\",\"timestamp\":%lu000 }", -10, function.c_str(), Time.now());  // Send -10, resolve any events
      PublishQueuePosix::instance().publish("Ubidots_Command_Hook", data, PRIVATE);
    } else {
      char data[128];
      if(invalidCommand == true){  // send the Invalid Command slack notification if the command was not recognized
        snprintf(data, sizeof(data), "{\"commands\":%i,\"context\":\"%s\",\"timestamp\":%lu000 }", 2, function.c_str(), Time.now());    // Send 2 (Invalid Command) to the 'commands' Synthetic Variable
        PublishQueuePosix::instance().publish("Ubidots_Command_Hook", data, PRIVATE);
        snprintf(data, sizeof(data), "{\"commands\":%i,\"context\":\"%s\",\"timestamp\":%lu000 }", -10, function.c_str(), Time.now());  // Send -10, resolve any events
        PublishQueuePosix::instance().publish("Ubidots_Command_Hook", data, PRIVATE);
      } else {                     // send the Execution Failure slack notification if the command was not recognized
        snprintf(data, sizeof(data), "{\"commands\":%i,\"context\":\"%s\",\"timestamp\":%lu000 }", 0, function.c_str(), Time.now());    // Send 0 (Execution Failure) to the 'commands' Synthetic Variable
        PublishQueuePosix::instance().publish("Ubidots_Command_Hook", data, PRIVATE);
        snprintf(data, sizeof(data), "{\"commands\":%i,\"context\":\"%s\",\"timestamp\":%lu000 }", -10, function.c_str(), Time.now());  // Send -10, resolve any events
        PublishQueuePosix::instance().publish("Ubidots_Command_Hook", data, PRIVATE);
      }
    }
	}
	return success;
}                    

bool Particle_Functions::disconnectFromParticle()                      // Ensures we disconnect cleanly from Particle
                                                                       // Updated based on this thread: https://community.particle.io/t/waitfor-particle-connected-timeout-does-not-time-out/59181
{
  time_t startTime = Time.now();
  Log.info("In the disconnect from Particle function");
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
