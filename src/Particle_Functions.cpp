#include "Particle.h"
#include "device_pinout.h"
#include "take_measurements.h"
#include "MyPersistentData.h"
#include "Particle_Functions.h"
#include "LoRA_Functions.h"
#include "JsonParserGeneratorRK.h"

char openTimeStr[8] = " ";
char closeTimeStr[8] = " ";


// Prototypes and System Mode calls
SYSTEM_MODE(SEMI_AUTOMATIC);                        // This will enable user code to start executing automatically.
SYSTEM_THREAD(ENABLED);                             // Means my code will not be held up by Particle processes.
STARTUP(System.enableFeature(FEATURE_RESET_INFO));

SerialLogHandler logHandler(LOG_LEVEL_INFO);     // Easier to see the program flow

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

	int nodeNumber;
	String variable;
	String function;
  char * pEND;
  char messaging[64];
  bool success = true;

	JsonParserStatic<1024, 80> jp;	// Global parser that supports up to 256 bytes of data and 20 tokens

  Log.info(command.c_str());

	jp.clear();
	jp.addString(command);
	if (!jp.parse()) {
		Log.info("Parsing failed - check syntax");
    Particle.publish("cmd", "Parsing failed - check syntax",PRIVATE);
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
		jp.getValueByKey(cmdObjectContainer, "node", nodeNumber);
		jp.getValueByKey(cmdObjectContainer, "var", variable);
		jp.getValueByKey(cmdObjectContainer, "fn", function);

    // In this section we will parse and execute the commands from the console or JSON - assumes connection to Particle
    // ****************  Note: currently there is no valudiation on the nodeNumbers ***************************
    // Reset Function
		if (function == "reset") {
      // Format - function - reset, node - nodeNumber, variables - either "current", "all" or "nodeData"
      // Test - {"cmd":[{"node":1,"var":"all","fn":"reset"}]}
      if (nodeNumber == 0) {
        if (variable == "nodeData") {
          snprintf(messaging,sizeof(messaging),"Resetting the gateway's node Data");
          nodeDatabase.resetNodeIDs();
          Log.info("Resetting the Gateway node so new database is in effect");
          Particle.publish("Alert","Resetting Gateway",PRIVATE);
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
        sysStatus.set_alertCodeGateway(20);              // Alert code 2 will reset the current data on the gateway
        sysStatus.set_resetCount(0);
      } 
      else {
        if (variable == "all") {
          snprintf(messaging,sizeof(messaging),"Resetting node %d's system and current data", nodeNumber);
          LoRA_Functions::instance().changeAlert(nodeNumber,5);    // Alertcode 5 will reset all data on the node
        }
        else {
          snprintf(messaging,sizeof(messaging),"Resetting node %d's current data", nodeNumber);
          LoRA_Functions::instance().changeAlert(nodeNumber,6);                    // Alertcode 6 will only reset all the current data on the node
        }
      }
    }
    // Reporting Frequency Function
    else if (function == "freq") {   
      // Format - function - freq, node - 0, variables - 2-60 (must be divisiable by two)
      // Test - {"cmd":[{"node":0,"var":"5","fn":"freq"}]}
      int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
      if ((tempValue > 0) && (tempValue <= 60) && 60 % tempValue == 0) {
        snprintf(messaging,sizeof(messaging),"Setting reporting frequency to %d minutes", tempValue);
        sysStatus.set_updatedFrequencyMinutes(tempValue);
      }
      else {
        snprintf(messaging,sizeof(messaging),"Not a valid reporting frequency");
        success = false;                                                       // Make sure it falls in a valid range or send a "fail" result
      }
    }
    // Stay Connected
    else if (function == "stay") {
      // Format - function - rpt, node - 0, variables - true or false
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
      LoRA_Functions::instance().printNodeData(true);
    }
    // Setting Open and close hours
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
    else if (function == "close") {
      // Format - function - close, node - 0, variables - 13-24 open hour
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
    // Setting the sensor type
    else if (function == "type") {
      // Format - function - type, node - nodeNumber, variables - 0 (car), 1(person), 2(TBD) 
      // Test - {"cmd":[{"node":1, "var":"1","fn":"type"}]}
      int tempValue = strtol(variable,&pEND,10);                       // Looks for the first integer and interprets it
      if ((tempValue >= 0 ) && (tempValue <= 3)) {
        snprintf(messaging,sizeof(messaging),"Setting sensor type to %d for node %d", tempValue, nodeNumber);
        if (!LoRA_Functions::instance().changeType(nodeNumber,tempValue)) success = false;  // Make a new entry in the nodeID database
        else LoRA_Functions::instance().changeAlert(nodeNumber,7);         // Forces the node to update its sensor Type (on Join, node sets the Gateway)
      }
      else {
        snprintf(messaging,sizeof(messaging),"Sensor Type  - must be 0-2");
        success = false;                                                       // Make sure it falls in a valid range or send a "fail" result
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
    // What if none of these functions are recognized
    else {
      snprintf(messaging,sizeof(messaging),"Not a valid command");
      success = false;
    }

    Log.info(messaging);
    if (Particle.connected()) Particle.publish("cmd",messaging,PRIVATE);
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