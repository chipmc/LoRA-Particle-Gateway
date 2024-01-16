/*
 * Project LoRA-Particle-Gateway
 * Description: This device will listen for data from client devices and forward the data to Particle via webhook
 * Author: Chip McClelland and Jeff Skarda
 * Date: 7-28-22
 */

// v0.01 - Started with the code from utilities - moving to a standard carrier board
// v0.02 - Adding sleep and scheduling using localTimeRK
// v0.03 - Refactored the code to make it more modular
// v0.04 - Works - refactored the LoRA functions
// v0.05 - Moved to the Storage Helper library
// v0.06 - Moved the LoRA Functions to a class implementation
// v0.07 - Moved to a class implementation for Particle Functions
// v0.08 - Simplified wake timing
// v0.09 - Added LoRA functions to clear the buffer and sleep the LoRA radio
// v0.10 - Stable - adding functionality
// v0.11 - Big changes to messaging and storage.  Onboards upto to 3 nodes.  Works!
// v0.12 - Added mandatory sync time on connect and check for empty queue before disconnect and node number mgt.
// v0.13 - Different webhooks for nodes and gateways. added reporting on cellular connection time and signal.  Added sensor type to join reques - need to figure out how to trigger
// v0.14 - Changing over to JsonParserGeneratorRK for node data, storage, webhook creation and Particle function / variable
// v0.15 - Completing move to class for Particle Functions, zero node alert code after send
// v0.16 - Fix for reporting frequency - using calculated variables
// v0.17 - Periodic health checks for connections to nodes
// v0.18 - Particle connection health check and continued move to classes
// v0.19 - Simpler reporting - adding connection success % to nodeData
// v1	- Release candidate - sending to Pilot Mountain
// v1.01- Working to improve consistency of loading persistent date
// v1.02 - udated the way I am implementing the StorageHelperRK function 
// v1.03 - reliability updates
// v1.04 - Added logic to check deviceID to validate node number
// v1.05 - Improved the erase node functions
// v2.00 - First Release for actual deployment
// V6.00 - Reliability Updates - Stays connected longer if a node fails to check in - deployed to Pilot Mountain on 2/16/23
// v7.00 - Added Signal to Noise Ratio to hourly reporting / webhook, battery level monitoring improvements, added power cycle function - Optimized for stick antenna - new center freq
// v9.00 - Breaking Change - v10 Node Required - Node Now Reports RSSI / SNR to Gateway, Simplified Join Request Logic, Storing / reporting hops
// v9.10 - Added Verizon code
// v11.00 - Verizon code no longer needed, Updated to include Soil Moisture sensor type and webhook
// v12.00 - Added support for the See Insights LoRA Node v1
// v13.00 - Breaking change - updated libraries (see below) and data structures.  Also, implemented a "token" and node API endpoint process that is new.  Requires node ver v4 or higher.  Minimal Gateway model.
// v14.00 - See Read.md for details.  Added support (persistent storage, Join Ack, Particle function) for "spaces" and "placement" enabled WebHooks - moved Gateways to "LoRA Gateway" product group (requires node v7 or above)
// v14.10 - Added support webhooks that are tied to four types: gateay, counter, occupancy and sensor.  This is a breaking change for the webhook names.  Also, added support for the "single" occupancy sensor type.
// v15.00 - Breaking change - amended the data payload values for data and join requests - needs nodes v8 or above
// v16.00 - Breaking change - nodes v9 or above ...
//			... redid all Particle Functions so they are called using node=[node uniqueID] instead of nodeNumber. Implemented mountConfig particle function that sets the payload for the node with the given uniqueID ...
// 			... sensorType, space, placement and multi are now all set on Join requests given that the JSON database contains the uniqueID of the node on any nodeNumber. 
//			To Test: (1) Make sure the device has joined and created a uniqueID in the node database (2) Set the sensorType and/or mountConfig on particle using the uniqueID as the 'node' variable (3) Press "Reset" on the node
//          TODOs:: FleetManager queued updates system compatibility (need to copy particle integrations and put Update-Device hook somewhere)
// v16.10 	Making small changes
// v16.20 	Making small changes to troubleshoot hard fault
// v17		Breaking Change - nodes v11 or above ...
//			... Added ability to configure the SPAD array by changing the "zoneMode" variable. 
//  		... reworked the Alert system to include alertContext with the alert. Stored in the NodeArray as an integer and passed as a single byte
// v17.10	Fixed data type issues when printing occupancyNet
// v17.20	Fixed join process
// v17.30  	Added a function that will keep track of the net occupanncy of rooms. Will be specific for MAFC then will generalize
			// - New class for Room Occupancy, new alert to adjust nodes for negative room occupancy (need to implement alert 3 on the node)
			// - Instead of publishing node webhooks - just publish to the console - perhaps that is a special "verbose mode"
			// - Nodes send updates to the gateway that captures the net and gross for each room in an array
// v17.40  	Setting Room Occupancy after receiving a Data Report from the Node. Space range now 1-64, but stored as 0-63 (for 6 bits). TODO:: comments added with questions/clarifications
// v17.50  	Breaking Change - Node v11.4 or later - Changed alertContext to uint16_t to prevent occupancyNet correction edge cases. Expanded Particle_Function bounds that used old uint8_t as the bounds.
// v18 		Integrated Ubidots-LoRA-Occupancy-v2 Particle integration, which sends node battery information and space values to the UpdateGatewayNodesAndSpaces UbiFunction.

#define DEFAULT_LORA_WINDOW 5
#define STAY_CONNECTED 60

// Particle Libraries
#include "PublishQueuePosixRK.h"			        // https://github.com/rickkas7/PublishQueuePosixRK
#include "LocalTimeRK.h"					        // https://rickkas7.github.io/LocalTimeRK/
#include "AB1805_RK.h"                          	// https://github.com/mapleiotsolutions/AB1805_RK
#include "Particle.h"                               // Because it is a CPP file not INO
// Application Files
#include "LoRA_Functions.h"							// Where we store all the information on our LoRA implementation - application specific not a general library
#include "device_pinout.h"							// Define pinouts and initialize them
#include "Particle_Functions.h"						// Particle specific functions
#include "take_measurements.h"						// Manages interactions with the sensors (default is temp for charging)
#include "MyPersistentData.h"						// Where my persistent storage files are kept
#include "Room_Occupancy.h"							// Aggregates node data to get net room occupancy for Occupancy Nodes

// Support for Particle Products (changes coming in 4.x - https://docs.particle.io/cards/firmware/macros/product_id/)
PRODUCT_VERSION(14);									// For now, we are putting nodes and gateways in the same product group - need to deconflict #
char currentPointRelease[6] ="17.50";

// Prototype functions
void publishStateTransition(void);                  // Keeps track of state machine changes - for debugging
void userSwitchISR();                               // interrupt service routime for the user switch
void publishWebhook(uint8_t nodeNumber);			// Publish data based on node number
void softDelay(uint32_t t);                 		// Soft delay is safer than delay

// System Health Variables
int outOfMemory = -1;                               // From reference code provided in AN0023 (see above)

// State Machine Variables
enum State { INITIALIZATION_STATE, ERROR_STATE, IDLE_STATE, SLEEPING_STATE, LoRA_STATE, CONNECTING_STATE, DISCONNECTING_STATE, REPORTING_STATE};
char stateNames[9][16] = {"Initialize", "Error", "Idle", "Sleeping", "LoRA", "Connecting", "Disconnecting", "Reporting"};
State state = INITIALIZATION_STATE;
State oldState = INITIALIZATION_STATE;

// Initialize Functions
SystemSleepConfiguration config;                    // Initialize new Sleep 2.0 Api
AB1805 ab1805(Wire);                                // Rickkas' RTC / Watchdog library
LocalTimeConvert conv;								// For determining if the park should be opened or closed - need local time
void outOfMemoryHandler(system_event_t event, int param);

// Program Variables
volatile bool userSwitchDectected = false;	

void setup() 
{
	waitFor(Serial.isConnected, 10000);				// Wait for serial connection

    initializePinModes();                           // Sets the pinModes

    initializePowerCfg();                           // Sets the power configuration for solar

	// Load the persistent storage objects
	sysStatus.setup();
	current.setup();
	nodeDatabase.setup();
	
    Particle_Functions::instance().setup();         // Sets up all the Particle functions and variables defined in particle_fn.h
                         
    ab1805.withFOUT(D8).setup();                	// Initialize AB1805 RTC   
    ab1805.setWDT(AB1805::WATCHDOG_MAX_SECONDS);	// Enable watchdog

	System.on(out_of_memory, outOfMemoryHandler);   // Enabling an out of memory handler is a good safety tip. If we run out of memory a System.reset() is done.

	PublishQueuePosix::instance().setup();          // Initialize PublishQueuePosixRK

	LoRA_Functions::instance().setup(true);			// Start the LoRA radio (true for Gateway and false for Node)

	// Setup local time and set the publishing schedule
	LocalTime::instance().withConfig(LocalTimePosixTimezone("EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00"));			// East coast of the US
	conv.withCurrentTime().convert();  				        // Convert to local time for use later

	if (Time.isValid()) {
		Log.info("LocalTime initialized, time is %s and RTC %s set", conv.format("%I:%M:%S%p").c_str(), (ab1805.isRTCSet()) ? "is" : "is not");
	}
	else {
		Log.info("LocalTime not initialized so will need to Connect to Particle");
		state = CONNECTING_STATE;
	}

	if (!digitalRead(BUTTON_PIN) || sysStatus.get_connectivityMode()== 1) {
		Log.info("User button or pre-existing set to connected mode");
		sysStatus.set_connectivityMode(1);					  // connectivityMode Code 1 keeps both LoRA and Cellular connections on
		state = CONNECTING_STATE;
	}
	
	attachInterrupt(BUTTON_PIN,userSwitchISR,CHANGE); // We may need to monitor the user switch to change behaviours / modes

	if (state == INITIALIZATION_STATE) state = SLEEPING_STATE;  // This is not a bad way to start - could also go to the LoRA_STATE
	
}

void loop() {

	switch (state) {
		case IDLE_STATE: {
			if (state != oldState) publishStateTransition();                   // We will apply the back-offs before sending to ERROR state - so if we are here we will take action
			if (sysStatus.get_alertCodeGateway() != 0) state = ERROR_STATE;
			else state = LoRA_STATE;											// Go to the LoRA state to start the next cycle									
		} break;

		case SLEEPING_STATE: {
			unsigned long wakeInSeconds, wakeBoundary;
			time_t time;

			publishStateTransition();                   					// We will apply the back-offs before sending to ERROR state - so if we are here we will take action
			wakeBoundary = (sysStatus.get_frequencyMinutes() * 60UL);
			wakeInSeconds = constrain(wakeBoundary - Time.now() % wakeBoundary, 0UL, wakeBoundary);  // If Time is valid, we can compute time to the start of the next report window	
			time = Time.now() + wakeInSeconds;
			Log.info("Sleep for %lu seconds until next event at %s", wakeInSeconds, Time.format(time, "%T").c_str());
			config.mode(SystemSleepMode::ULTRA_LOW_POWER)
				.gpio(BUTTON_PIN,CHANGE)
				.duration(wakeInSeconds * 1000L);
			ab1805.stopWDT();  												   // No watchdogs interrupting our slumber
			SystemSleepResult result = System.sleep(config);                   // Put the device to sleep device continues operations from here
			ab1805.resumeWDT();                                                // Wakey Wakey - WDT can resume
			if (result.wakeupPin() == BUTTON_PIN) {
				waitFor(Serial.isConnected, 10000);							   // Wait for serial connection
				softDelay(1000);
				Log.info("Woke with user button");
			}
			else {															   // Awoke for time
				Log.info("Awoke at %s with %li free memory", Time.format(Time.now(), "%T").c_str(), System.freeMemory());
			}
			state = IDLE_STATE;

		} break;

		case LoRA_STATE: {														// Enter this state every reporting period and stay here for 5 minutes
			static system_tick_t startLoRAWindow = 0;
			static byte connectionWindow = 0;

			if (state != oldState) {
				if (oldState != REPORTING_STATE) startLoRAWindow = millis();    // Mark when we enter this state - for timeouts - but multiple messages won't keep us here forever
				publishStateTransition();                   					// We will apply the back-offs before sending to ERROR state - so if we are here we will take action
				conv.withCurrentTime().convert();								// Get the time and convert to Local
				if (conv.getLocalTimeHMS().hour >= sysStatus.get_openTime() && conv.getLocalTimeHMS().hour <= sysStatus.get_closeTime()) current.set_openHours(true);
				else current.set_openHours(false);

				if (sysStatus.get_connectivityMode() == 0) connectionWindow = DEFAULT_LORA_WINDOW;
				else connectionWindow = STAY_CONNECTED;

				Log.info("Gateway is listening for %d minutes for LoRA messages (%d / %d / %d)", (sysStatus.get_connectivityMode() == 0) ? DEFAULT_LORA_WINDOW : 60, conv.getLocalTimeHMS().hour, sysStatus.get_openTime(), sysStatus.get_closeTime());
			} 

			if (LoRA_Functions::instance().listenForLoRAMessageGateway()) {
				Log.info("Received LoRA message from node %d", current.get_nodeNumber());
				if (current.get_alertCodeNode() != 1) state = REPORTING_STATE; 				    // Received and acknowledged data from a node - need to report the alert
			}

			if ((millis() - startLoRAWindow) > (connectionWindow *60000UL)) { 					// Keeps us in listening mode for the specified windpw - then back to idle unless in test mode - keeps listening
				Log.info("Listening window over");
				LoRA_Functions::instance().sleepLoRaRadio();									// Done with the LoRA phase - put the radio to sleep
				LoRA_Functions::instance().printNodeData(false);
				nodeDatabase.flush(true);
				if (Time.hour() != Time.hour(sysStatus.get_lastConnection())) state = CONNECTING_STATE;  	// Only Connect once an hour after the LoRA window is over and if the park is open			
				else if (sysStatus.get_alertCodeGateway() != 0) state = ERROR_STATE;
				else state = SLEEPING_STATE;
			}
		} break;

		case REPORTING_STATE: {
			publishStateTransition();
			publishWebhook(current.get_nodeNumber());							// Gateway or node webhook
			current.set_alertCodeNode(0);										// Zero alert code after send
			state = LoRA_STATE;
		} break;

		case CONNECTING_STATE: {
			static system_tick_t connectingTimeout = 0;

			if (state != oldState) {
				publishStateTransition();  
				if (Time.day(sysStatus.get_lastConnection()) != conv.getLocalTimeYMD().getDay()) {
					sysStatus.set_messageCount(0);									// Reset the message count at midnight
					sysStatus.set_resetCount(0);									// Reset the reset counter
					Log.info("New Day - Resetting everything");
				}
				publishWebhook(sysStatus.get_nodeNumber());								// Before we connect - let's send the gateway's webhook
				if (!Particle.connected()) Particle.connect();							// Time to connect to Particle
				connectingTimeout = millis();
			}

			if (Particle.connected()) {													// Either we will connect or we will timeout - will try for 10 minutes 
				sysStatus.set_lastConnection(Time.now());
				sysStatus.set_lastConnectionDuration((millis() - connectingTimeout) / 1000);	// Record connection time in seconds
				if (Particle.connected()) {
					Particle.syncTime();												// To prevent large connections, we will sync every hour when we connect to the cellular network.
					waitUntil(Particle.syncTimeDone);									// Make sure sync is complete
					CellularSignal sig = Cellular.RSSI();
				}
				if (sysStatus.get_connectivityMode() == 1) state = LoRA_STATE;			// Go back to the LoRA State if we are in connected mode
				else state = DISCONNECTING_STATE;	 									// Typically, we will disconnect and sleep to save power - publishes occur during the 90 seconds before disconnect
			}
			else if (millis() - connectingTimeout > 600000L) {
				Log.info("Failed to connect in 10 minutes - giving up");
				sysStatus.set_connectivityMode(0);										// Setting back to zero - must not have coverage here or here at this time
				state = DISCONNECTING_STATE;											// Makes sure we turn off the radio
			}

		} break;

		case DISCONNECTING_STATE: {														// Waits 90 seconds then disconnects
			static system_tick_t stayConnectedWindow = 0;

			if (state != oldState) {
				publishStateTransition(); 
				stayConnectedWindow = millis(); 
			}

			if ((millis() - stayConnectedWindow > 90000UL) && PublishQueuePosix::instance().getCanSleep()) {	// Stay on-line for 90 seconds and until we are done clearing the queue
				if (sysStatus.get_connectivityMode() == 0) Particle_Functions::instance().disconnectFromParticle();
				state = SLEEPING_STATE;
			}
		} break;

		case ERROR_STATE: {
			static system_tick_t resetTimeout = millis();

			if (state != oldState) {
				publishStateTransition();
				if (Particle.connected()) Particle.publish("Alert","Deep power down in 30 seconds", PRIVATE);
				sysStatus.set_alertCodeGateway(0);			// Reset this
			}

			if (millis() - resetTimeout > 30000L) {
				Log.info("Deep power down device");
				softDelay(2000);
				ab1805.deepPowerDown(); 
			}
		} break;
	}

	ab1805.loop();                                  // Keeps the RTC synchronized with the Boron's clock

	PublishQueuePosix::instance().loop();           // Check to see if we need to tend to the message 

	sysStatus.loop();
	current.loop();
	nodeDatabase.loop();

	LoRA_Functions::instance().loop();				// Check to see if Node connections are healthy

	if (outOfMemory >= 0) {                         // In this function we are going to reset the system if there is an out of memory error
		Log.info("Resetting due to low memory");
		softDelay(2000);
		System.reset();
  	}

	if (sysStatus.get_alertCodeGateway() > 0) state = ERROR_STATE;
}

/**
 * @brief Publishes a state transition to the Log Handler and to the Particle monitoring system.
 *
 * @details A good debugging tool.
 */
void publishStateTransition(void)
{
	char stateTransitionString[256];
	if (state == IDLE_STATE && !Time.isValid()) snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s with invalid time", stateNames[oldState],stateNames[state]);
	else snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s", stateNames[oldState],stateNames[state]);

	oldState = state;

	Log.info(stateTransitionString);
}

// Here are the various hardware and timer interrupt service routines
void outOfMemoryHandler(system_event_t event, int param) {
    outOfMemory = param;
}

void userSwitchISR() {
	userSwitchDectected = true;
}

/**
 * @brief Publish Webhook will put the webhook data into the publish queue
 * 
 * @details Nodes and Gateways will use the same format for this webook - data sources will change
 * 
 * Webhooks will come in a few flavors based on the type of sensors we are using.  To keep this from getting to tedious, we will use a single webhook for each class of node and the gateway.
 * The classes of webhooks are: 
 * 1. Gateway - This is the gateway webhook - it will be published every time the gateway wakes up and connects to Particle.  It uses the "Ubidots-LoRA-Gateway-v1" webhook
 * 2. Counter - This is the webhook for the counter nodes.  It uses the "Ubidots-LoRA-Counter-v1" webhook
 * 3. Occupancy - This is the webhook for the occupancy nodes.  It uses the "Ubidots-LoRA-Occupancy-v1" webhook
 * 4. Sensor - This is the webhook for the sensor nodes.  It uses the "Ubidots-LoRA-Sensor-v1" webhook
 * 
 * See this article for details including the device type definitions:
 * @link https://seeinsights.freshdesk.com/support/solutions/articles/154000101712-sensor-types-and-identifiers
 * 
 */
void publishWebhook(uint8_t nodeNumber) {							
	char data[256];                             						// Store the date in this character array - not global
	// Battery conect information - https://docs.particle.io/reference/device-os/firmware/boron/#batterystate-
    const char* batteryContext[8] = {"Unknown","Not Charging","Charging","Charged","Discharging","Fault","Diconnected"};
	Log.info("Publishing webhook for node %d", nodeNumber);

	if (!Time.isValid()) {
		return;															// A webhook without a valid timestamp is worthless
		Log.info("Time is not valid - not publishing webhook");
	}
	unsigned long endTimePeriod = Time.now() - (Time.second() + 1);		// Moves the timestamp within the reporting boundary - so 18:00:14 becomes 17:59:59 - helps in Ubidots reporting

	if (nodeNumber == 0) {												// Webhook for the Gateway					
		Log.info("Publishing for Gateway");
		takeMeasurements();												// Loads the current values for the Gateway
		// Gateway reporting
		// The first webhook could be sent once a day or so it would give the health of the gateway
		snprintf(data, sizeof(data), "{\"deviceid\":\"%s\", \"battery\":%d,\"key1\":\"%s\",\"temp\":%d, \"resets\":%d, \"alerts\": %d, \"msg\":%d, \"timestamp\":%lu000}",\
		Particle.deviceID().c_str(), current.get_stateOfCharge(), batteryContext[current.get_batteryState()],\
		current.get_internalTempC(), sysStatus.get_resetCount(), sysStatus.get_alertCodeGateway(), sysStatus.get_messageCount(), endTimePeriod);
		PublishQueuePosix::instance().publish("Ubidots-LoRA-Gateway-v1", data, PRIVATE | WITH_ACK);
	}
	else {
	Log.info("Publishing for nodeNumber is %i of sensorType of %s", nodeNumber, (nodeNumber == 0) ? "Gateway" : (current.get_sensorType() <= 9) ? "Visitation Counter" : (current.get_sensorType() <= 19) ? "Occupancy Counter" : (current.get_sensorType() <= 29) ? "Sensor" : "Unknown");
		switch (current.get_sensorType()) {
			case 1 ... 9: {													// Counter
				snprintf(data, sizeof(data), "{\"uniqueid\":\"%lu\", \"hourly\":%u, \"daily\":%u, \"sensortype\":%d, \"battery\":%d,\"key1\":\"%s\",\"temp\":%d, \"resets\":%d,\"alerts\": %d, \"node\": %d, \"rssi\":%d,  \"snr\":%d, \"hops\":%d,\"timestamp\":%lu000}",\
				current.get_uniqueID(), (current.get_payload1() << 8 | current.get_payload2()), (current.get_payload3() << 8 | current.get_payload4()), current.get_sensorType(), current.get_stateOfCharge(), batteryContext[current.get_batteryState()],\
				current.get_internalTempC(), current.get_resetCount(), current.get_alertCodeNode(), current.get_nodeNumber(), current.get_RSSI(), current.get_SNR(), current.get_hops(), endTimePeriod);
				Log.info("Data is %s", data);
				PublishQueuePosix::instance().publish("Ubidots-LoRA-Counter-v1", data, PRIVATE | WITH_ACK);
			} break;

			case 10 ... 19: {												// Occupancy
				// (v17.40, now sending (space + 1) to Ubidots, as we index it as an unsigned 6-bit integer in the application now and would cause problems if space were to be 64)
				snprintf(data, sizeof(data), "{\"uniqueid\":\"%lu\", \"gross\":%u, \"net\":%i, \"space\":%d, \"placement\":%d, \"multi\":%d, \"zoneMode\":%d, \"sensortype\":%d, \"battery\":%d,\"key1\":\"%s\",\"temp\":%d, \"resets\":%d,\"alerts\":%d, \"node\":%d, \"rssi\":%d, \"snr\":%d,\"hops\":%d,\"timestamp\":%lu000}",\
				current.get_uniqueID(), (current.get_payload1() << 8 | current.get_payload2()), (int16_t)(current.get_payload3() << 8 | current.get_payload4()), current.get_payload5() + 1, current.get_payload6(), current.get_payload7(), current.get_payload8(), current.get_sensorType(), current.get_stateOfCharge(), batteryContext[current.get_batteryState()],\
				current.get_internalTempC(), current.get_resetCount(), current.get_alertCodeNode(), current.get_nodeNumber(), current.get_RSSI(), current.get_SNR(), current.get_hops(), endTimePeriod);
				Log.info("Data is %s", data);
				// Don't send the webhook - just publish if connected
				if (Particle.connected()) {
					PublishQueuePosix::instance().publish("Node Data", data, PRIVATE);
				}
				/*** TODO:: (Alex) Review this if you have time ***/
				Room_Occupancy::instance().setRoomCounts();
				// Compose and send the node and space information to the UpdateGatewayNodesAndSpaces ubiFunction
				snprintf(data, sizeof(data), "{\"nodeUniqueID\":\"%lu\",\"battery\":%d,\"space\":%d,\"spaceNet\":%d,\"spaceGross\":%d}",\
				current.get_uniqueID(), current.get_stateOfCharge(), current.get_payload5() + 1, Room_Occupancy::instance().getRoomNet(current.get_payload5()), Room_Occupancy::instance().getRoomGross(current.get_payload5()));
				PublishQueuePosix::instance().publish("Ubidots-LoRA-Occupancy-v2", data, PRIVATE | WITH_ACK);
			} break;

			case 20 ... 29: {												// Sensor
				snprintf(data, sizeof(data), "{\"uniqueid\":\"%lu\", \"soilvwc\":%u, \"soiltemp\":%u, \"space\":%d, \"placement\":%d, \"sensortype\":%d, \"battery\":%d,\"key1\":\"%s\",\"temp\":%d, \"resets\":%d,\"alerts\": %d, \"node\": %d, \"rssi\":%d,  \"snr\":%d, \"hops\":%d,\"timestamp\":%lu000}",\
				current.get_uniqueID(), (current.get_payload1() << 8 | current.get_payload2()), (current.get_payload3() << 8 | current.get_payload4()), current.get_payload5() + 1, current.get_payload6(),current.get_sensorType(), current.get_stateOfCharge(), batteryContext[current.get_batteryState()],\
				current.get_internalTempC(), current.get_resetCount(), current.get_alertCodeNode(), current.get_nodeNumber(), current.get_RSSI(), current.get_SNR(), current.get_hops(), endTimePeriod);
				Log.info("Data is %s", data);
				PublishQueuePosix::instance().publish("Ubidots-LoRA-Sensor-v1", data, PRIVATE | WITH_ACK);
			} break;

			default: {														// Unknown
				Log.info("Unknown sensor type in gateway publish %d", current.get_sensorType());
				if (Particle.connected()) Particle.publish("Alert","Unknown sensor type in gateway publish", PRIVATE);
			} break;
		}
	}

	return;
}

/**
 * @brief soft delay let's us process Particle functions and service the sensor interrupts while pausing
 * 
 * @details takes a single unsigned long input in millis
 * 
 */
inline void softDelay(uint32_t t) {
  for (uint32_t ms = millis(); millis() - ms < t; Particle.process());  //  safer than a delay()
}
