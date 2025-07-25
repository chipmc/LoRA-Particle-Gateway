/*
 * Project LoRA-Particle-Gateway
 * Description: This device will listen for data from client devices and forward the data to Particle via webhook
 * Author: Chip McClelland, Jeff Skarda, Alex Bowen
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
// v19 		Added 'break time' to SysStatus, which now tells the nodes to reset to 0 at a certain time. Increased NodeID string size to 3072 characters. Fixed major bug in Room_Occupancy.cpp. 
// v19.1 	removed type-based JSON in favor of 'jsonData1' and 'jsonData2' which have been added as additional persistent storage based on sensor type.
// v19.2 	Added alerts to troubleshoot infinite joins caused by JSON database errors. Join requests now set the nodes' configuration settings (from the join payload) when the node database is empty and the node joins the network.
// v19.3 	Fixed declaration of Base64RK in LoRA_Functions.cpp that was preventing cloud compile and cloud flash
// v19.4 	Changed all instances of Particle.publish() to PublishQueuePosix.instance().publish(). Properly cleared nodeDatabase values when calling setType
// v20		New JSON fixes from JSON-Parser-Test integrated into the system. Conducted a lot of testing. Good results, and found/fixed a bug with current.openHours that caused loops of alert code 6.
// v20.1	Added the getJsonString helper function to eliminate problems with the JSON->String->memory piece.
// v21		Added a function layer by which any alert codes that set a value on a device ALSO set that value in the JSON database and update Ubidots so reflecting node state does not rely on data reports being sent. 
// v21.1    Integrated new backend layer from v21 in places where alert code 12, 5, and 6 are set on the node.
// v21.2    Going back to LORA_STATE now resets any spaces that have been inactive for an hour. A separate weekend break time is now in effect, with Particle functions to manage it.
// v21.3    Added a particle function (setOccupancyNetForNode) and Room_Occupancy layer that allows for a node to have its net count set manually through Particle/Ubidots.
// v21.4    Added a particle function (resetSpace) that allows for a space (and all its nodes) to have their values reset through Particle/Ubidots.
// v21.5    Node v12 or later - Added a particle function (setTofDetectionsPerSecond) with alert code 13 - sets tofDetectionsPerSecond on a node
// v22.0	Changing the way that the gateway tells the nodes how often to report.  This will be a Particle function that will be called by the Fleet Manager.  This will be a breaking change for the nodes.
// v22.1    Cleaned up the LoRA_Functions class, separated all JSON database functionality into JsonDataManager.cpp
// v23.0    Modified to support Argon WiFi Gateway instead of Cellular. - Added a "config.h" to hold the configuration settings for the device.
// v23.1 	Moved timezone selection to config.h - what else should be here?
// v23.2 	Added code that will ensure that the gateway connects at least once an hour - even if no nodes are connected.
// v23.3 	Fixed interpretation of the battery context value.
// v23.4 	Added a configureation to allow for a disconnected gateway (Serial Only)
// v23.5	Added a rate limit for check inactive spaces and reset counts - once an hour
// v23.6 	Added a .gitignore file to stop replicating the compiled code to the repo

// Particle Libraries
#include "PublishQueuePosixRK.h"			        // https://github.com/rickkas7/PublishQueuePosixRK
#include "LocalTimeRK.h"					        // https://rickkas7.github.io/LocalTimeRK/
#include "AB1805_RK.h"                          	// https://github.com/mapleiotsolutions/AB1805_RK
#include "Particle.h"                               // Because it is a CPP file not INO
// Application Files
#include "LoRA_Functions.h"							// Where we store all the information on our LoRA implementation - application specific not a general library
#include "JsonDataManager.h"						// Where we interact with an on-board JSON database	
#include "device_pinout.h"							// Define pinouts and initialize them
#include "Particle_Functions.h"						// Particle specific functions
#include "take_measurements.h"						// Manages interactions with the sensors (default is temp for charging)
#include "MyPersistentData.h"						// Where my persistent storage files are kept
#include "Room_Occupancy.h"							// Aggregates node data to get net room occupancy for Occupancy Nodes
#include "config.h"									// Configuration file for the device

// Support for Particle Products (changes coming in 4.x - https://docs.particle.io/cards/firmware/macros/product_id/)
PRODUCT_VERSION(23);								// For now, we are putting nodes and gateways in the same product group - need to deconflict #
char currentPointRelease[6] ="23.6";

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

	Log.code(1).info("Info message");

	// This is a kluge to speed development - going to set the flag for WiFi Manually here - need to set up a function to do this
	sysStatus.set_connectivityMode(4);				// connectivityMode Code 4 keeps both LoRA and WiFi Connections on
	
    Particle_Functions::instance().setup();         // Sets up all the Particle functions and variables defined in particle_fn.h
                         
    ab1805.withFOUT(D8).setup();                	// Initialize AB1805 RTC   
    ab1805.setWDT(AB1805::WATCHDOG_MAX_SECONDS);	// Enable watchdog

	System.on(out_of_memory, outOfMemoryHandler);   // Enabling an out of memory handler is a good safety tip. If we run out of memory a System.reset() is done.

	PublishQueuePosix::instance().setup();          // Initialize PublishQueuePosixRK

	LoRA_Functions::instance().setup(true);			// Start the LoRA radio (true for Gateway and false for Node)

	// Setup local time and set the publishing schedule
	LocalTime::instance().withConfig(LocalTimePosixTimezone(TIME_CONFIG));			// East coast of the US
	conv.withCurrentTime().convert();  				// Convert to local time for use later

	if (Time.isValid()) {
		Log.info("LocalTime initialized, time is %s and RTC %s set", conv.format("%I:%M:%S%p").c_str(), (ab1805.isRTCSet()) ? "is" : "is not");
	}
	else {
		Log.info("LocalTime not initialized so will need to Connect to Particle");
		state = CONNECTING_STATE;
	}

	// if (!digitalRead(BUTTON_PIN) || sysStatus.get_connectivityMode()== 1) {
	// 	Log.info("User button or pre-existing set to connected mode");
		sysStatus.set_connectivityMode(1);			  // connectivityMode Code 1 keeps both LoRA and Cellular connections on
		state = CONNECTING_STATE;
	// }
	
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
			wakeBoundary = (sysStatus.get_frequencySeconds());
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
			static byte lastCheckedInactiveSpaces = 0;	
			static byte lastResetOccupancyWhenClosed = 0;		

			if (state != oldState) {
				if (oldState != REPORTING_STATE){ 
					startLoRAWindow = millis(); // Mark when we enter this state - for timeouts - but multiple messages won't keep us here forever
				} else {
					// Check for inactive spaces and reset them. Ignores nodes of a "Counter" sensorType as they do not have spaces. Only once an hour
					if (lastCheckedInactiveSpaces != Time.hour()) {
						lastCheckedInactiveSpaces = Time.hour();
						// Check for inactive spaces and reset them. Ignores nodes of a "Counter" sensorType as they do not have spaces. Only once an hour
						if (sysStatus.get_connectivityMode() == 0) {
							Log.info("Checking for inactive spaces...");
							JsonDataManager::instance().resetInactiveSpaces(3600);  // Define "inactive" spaces as those where ALL of the nodes in that space have not sent a report in 3600 seconds.
						}
					}
				}

				publishStateTransition();                   					// We will apply the back-offs before sending to ERROR state - so if we are here we will take action
				conv.withCurrentTime().convert();								// Get the time and convert to Local
				if (conv.getLocalTimeHMS().hour >= sysStatus.get_openTime() && conv.getLocalTimeHMS().hour <= sysStatus.get_closeTime()) {
					current.set_openHours(true);
				} else {	// Closed for the day - reset
					if (lastResetOccupancyWhenClosed != Time.hour()) {
						lastResetOccupancyWhenClosed = Time.hour();
						Log.info("Resetting all counts - not in open hours. Open hour: %d, Close Hour: %d, Current Hour: %d", sysStatus.get_openTime(), sysStatus.get_closeTime(), conv.getLocalTimeHMS().hour);
						Room_Occupancy::instance().resetAllCounts();	// reset the room net AND gross counts at end of day for all occupancy nodes and update Ubidots
					}	
					current.set_openHours(false);
				};																										
				
				String dayString = conv.timeStr().substring(0, 3);	// Take the first three characters of the timeStr ("Fri", "Sat", "Sun")
				uint8_t breakLengthHours;
				bool isWeekend = (dayString == "Sat" || dayString == "Sun");	// If it is a weekend, we will use the weekendBreakTime and weekendBreakLengthMinutes instead
				if (isWeekend) {
					breakLengthHours = (sysStatus.get_weekendBreakLengthMinutes() / 60);
				} else {
					breakLengthHours = (sysStatus.get_breakLengthMinutes() / 60);
				}
				uint8_t breakTime = isWeekend ? sysStatus.get_weekendBreakTime() : sysStatus.get_breakTime();
				uint16_t breakLengthMinutes = isWeekend ? sysStatus.get_weekendBreakLengthMinutes() : sysStatus.get_breakLengthMinutes();
				
				if (breakTime != 24) { // Ignore break functionality entirely if it is set to be 24. This means no break is needed for this gateway
					if (conv.getLocalTimeHMS().hour >= breakTime &&
						conv.getLocalTimeHMS().hour <= (breakTime + breakLengthHours) &&
						conv.getLocalTimeHMS().minute < (breakLengthMinutes - (breakLengthHours * 60))) {
						if (current.get_onBreak() == 0) {
							current.set_onBreak(1); // start the break
							Room_Occupancy::instance().resetNetCounts(); // reset the room net counts for all occupancy nodes and update Ubidots
						}
					} else {
						current.set_onBreak(0); // Otherwise, we are still not on break
					}

					Log.info("%s Break Starts at %d with length of %d minutes. Current hour = %d, minute = %d On Break? %s", isWeekend ? "Weekend" : "Weekday", breakTime, breakLengthMinutes, conv.getLocalTimeHMS().hour, conv.getLocalTimeHMS().minute, current.get_onBreak() ? "Yes" : "No");
				}

				if (sysStatus.get_connectivityMode() == 0) connectionWindow = DEFAULT_LORA_WINDOW;
				else connectionWindow = STAY_CONNECTED;

				Log.info("Gateway is listening for %d minutes for LoRA messages (%d / %d / %d)", (sysStatus.get_connectivityMode() == 0) ? DEFAULT_LORA_WINDOW : 60, conv.getLocalTimeHMS().hour, sysStatus.get_openTime(), sysStatus.get_closeTime());
			} 

			if (LoRA_Functions::instance().listenForLoRAMessageGateway()) {
				Log.info("Received LoRA message from node %d", current.get_nodeNumber());
				if (current.get_alertCodeNode() != 1) state = REPORTING_STATE; 				    // Received and acknowledged data from a node - need to report the alert
			}

			if (sysStatus.get_connectivityMode() == 1)	{										// If we are in connected mode - we will stay in the LoRA state
				if (Time.hour() != Time.hour(sysStatus.get_lastConnection())) state = CONNECTING_STATE;  	// Connect once an hour even if no messages are received	
				break;
			}
			else if ((millis() - startLoRAWindow) > (connectionWindow *60000UL)) { 				// Keeps us in listening mode for the specified windpw - then back to idle unless in test mode - keeps listening
				Log.info("Listening window over");
				LoRA_Functions::instance().sleepLoRaRadio();									// Done with the LoRA phase - put the radio to sleep
				JsonDataManager::instance().printNodeData(false);
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

				if (TRANSPORT_MODE == 2) {												// Serial connection only - no WiFi or Cellular
					state = LoRA_STATE;													// No need to connect - we are running locally
					sysStatus.set_lastConnection(Time.now());
					sysStatus.set_lastConnectionDuration(0);							// Record connection time in seconds
					break;																// Done - we are out of here - Note, this should keep us listening in LoRA mode - not sleeping
				}

				if (!Particle.connected()) Particle.connect();							// Time to connect to Particle
				connectingTimeout = millis();
			}

			if (Particle.connected()) {													// Either we will connect or we will timeout - will try for 10 minutes 
				sysStatus.set_lastConnection(Time.now());
				sysStatus.set_lastConnectionDuration((millis() - connectingTimeout) / 1000);	// Record connection time in seconds
				if (Particle.connected()) {
					Particle.syncTime();												// To prevent large connections, we will sync every hour when we connect to the cellular network.
					waitUntil(Particle.syncTimeDone);									// Make sure sync is complete
					#if TRANSPORT_MODE == 1
						CellularSignal sig = Cellular.RSSI();
						Log.info("Cellular Signal Strength: %d dBm", (int8_t)sig.getStrength());
					#endif
					#if TRANSPORT_MODE == 0	
						WiFiSignal sig = WiFi.RSSI();
						Log.info("WiFi Signal Strength: %d dBm", (int8_t)sig.getStrength());
					#endif
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
				if (Particle.connected()) PublishQueuePosix::instance().publish("Alert","Deep power down in 30 seconds", PRIVATE);
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
	char webhook[256];													// Store Webhook name

	// Battery conect information - https://docs.particle.io/reference/device-os/firmware/boron/#batterystate-
	const char* batteryContext[7] = {"Unknown","Not Charging","Charging","Charged","Discharging","Fault","Diconnected"};		// Fixed

	if (!Time.isValid()) {
		return;															// A webhook without a valid timestamp is worthless
		Log.info("Time is not valid - not publishing webhook");
	}
	unsigned long endTimePeriod = Time.now() - (Time.second() + 1);		// Moves the timestamp within the reporting boundary - so 18:00:14 becomes 17:59:59 - helps in Ubidots reporting

	if (nodeNumber == 0) {												// Webhook for the Gateway					
		takeMeasurements();												// Loads the current values for the Gateway
		snprintf(webhook, sizeof(webhook),"Ubidots-LoRA-Gateway-v1");
		snprintf(data, sizeof(data), "{\"deviceid\":\"%s\", \"battery\":%d,\"key1\":\"%s\",\"temp\":%d, \"resets\":%d, \"alerts\": %d, \"msg\":%d, \"timestamp\":%lu000}",\
		Particle.deviceID().c_str(), current.get_stateOfCharge(), batteryContext[current.get_batteryState()],\
		current.get_internalTempC(), sysStatus.get_resetCount(), sysStatus.get_alertCodeGateway(), sysStatus.get_messageCount(), endTimePeriod);
	}
	else {
	Log.info("Publishing for nodeNumber is %i of sensorType of %s", nodeNumber, (nodeNumber == 0) ? "Gateway" : (current.get_sensorType() <= 9) ? "Visitation Counter" : (current.get_sensorType() <= 19) ? "Occupancy Counter" : (current.get_sensorType() <= 29) ? "Sensor" : "Unknown");
		switch (current.get_sensorType()) {
			case 1 ... 9: {													// Counter
				snprintf(webhook, sizeof(webhook),"Ubidots-LoRA-Counter-v1");
				snprintf(data, sizeof(data), "{\"uniqueid\":\"%lu\", \"hourly\":%u, \"daily\":%u, \"sensortype\":%d, \"battery\":%d,\"key1\":\"%s\",\"temp\":%d, \"resets\":%d,\"alerts\": %d, \"node\": %d, \"rssi\":%d,  \"snr\":%d, \"hops\":%d,\"timestamp\":%lu000}",\
				current.get_uniqueID(), (current.get_payload1() << 8 | current.get_payload2()), (current.get_payload3() << 8 | current.get_payload4()), current.get_sensorType(), current.get_stateOfCharge(), batteryContext[current.get_batteryState()],\
				current.get_internalTempC(), current.get_resetCount(), current.get_alertCodeNode(), current.get_nodeNumber(), current.get_RSSI(), current.get_SNR(), current.get_hops(), endTimePeriod);
			} break;

			case 10 ... 19: {												// Occupancy
				// (v17.40, now sending (space + 1) to Ubidots, as we index it as an unsigned 6-bit integer in the application now and would cause problems if space were to be 64)
				#if SERIAL_LOG_LEVEL == 3	
				// This is creating extra serial communications - likely turn off once things are working
				snprintf(webhook, sizeof(webhook),"Node Data");				
				snprintf(data, sizeof(data), "{\"uniqueid\":\"%lu\", \"gross\":%u, \"net\":%i, \"space\":%d, \"placement\":%d, \"multi\":%d, \"zoneMode\":%d, \"sensortype\":%d, \"battery\":%d,\"key1\":\"%s\",\"temp\":%d, \"resets\":%d,\"alerts\":%d, \"node\":%d, \"rssi\":%d, \"snr\":%d,\"hops\":%d,\"timestamp\":%lu000}",\
				current.get_uniqueID(), (current.get_payload1() << 8 | current.get_payload2()), (int16_t)(current.get_payload3() << 8 | current.get_payload4()), current.get_payload5() + 1, current.get_payload6(), current.get_payload7(), current.get_payload8(), current.get_sensorType(), current.get_stateOfCharge(), batteryContext[current.get_batteryState()],\
				current.get_internalTempC(), current.get_resetCount(), current.get_alertCodeNode(), current.get_nodeNumber(), current.get_RSSI(), current.get_SNR(), current.get_hops(), endTimePeriod);
				if (Particle.connected()) PublishQueuePosix::instance().publish(webhook, data, PRIVATE | WITH_ACK);  // Only going to publish if connected
				Log.info("%s : %s", webhook, data);				
				#endif 

				// Compose and send the node and space information to the UpdateGatewayNodesAndSpaces ubiFunction
				snprintf(webhook, sizeof(webhook),"Ubidots-LoRA-Occupancy-v2");		
				snprintf(data, sizeof(data), "{\"nodeUniqueID\":\"%lu\",\"battery\":%d,\"space\":%d,\"spaceNet\":%d,\"spaceGross\":%d}",\
				current.get_uniqueID(), current.get_stateOfCharge(), current.get_payload5() + 1, Room_Occupancy::instance().getRoomNet(current.get_payload5()), Room_Occupancy::instance().getRoomGross(current.get_payload5()));
			} break;

			case 20 ... 29: {												// Sensor
				snprintf(webhook, sizeof(webhook),"Ubidots-LoRA-Sensor-v1");		
				snprintf(data, sizeof(data), "{\"uniqueid\":\"%lu\", \"soilvwc\":%u, \"soiltemp\":%u, \"space\":%d, \"placement\":%d, \"sensortype\":%d, \"battery\":%d,\"key1\":\"%s\",\"temp\":%d, \"resets\":%d,\"alerts\": %d, \"node\": %d, \"rssi\":%d,  \"snr\":%d, \"hops\":%d,\"timestamp\":%lu000}",\
				current.get_uniqueID(), (current.get_payload1() << 8 | current.get_payload2()), (current.get_payload3() << 8 | current.get_payload4()), current.get_payload5() + 1, current.get_payload6(),current.get_sensorType(), current.get_stateOfCharge(), batteryContext[current.get_batteryState()],\
				current.get_internalTempC(), current.get_resetCount(), current.get_alertCodeNode(), current.get_nodeNumber(), current.get_RSSI(), current.get_SNR(), current.get_hops(), endTimePeriod);
			} break;

			default: {														// Unknown
				snprintf(webhook, sizeof(webhook),"Alert");	
				snprintf(data, sizeof(data),"Unknown sensor type in gateway publish" );
			} break;
		}
	}
	if (Particle.connected()) PublishQueuePosix::instance().publish(webhook, data, PRIVATE | WITH_ACK);  // Only going to publish if connected
	Log.info("%s : %s", webhook, data);

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
