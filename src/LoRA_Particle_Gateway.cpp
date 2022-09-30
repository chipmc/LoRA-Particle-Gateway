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

// Particle Libraries
#include "PublishQueuePosixRK.h"			        // https://github.com/rickkas7/PublishQueuePosixRK
#include "LocalTimeRK.h"					        // https://rickkas7.github.io/LocalTimeRK/
#include "AB1805_RK.h"                          	// Watchdog and Real Time Clock - https://github.com/rickkas7/AB1805_RK
#include "Particle.h"                               // Because it is a CPP file not INO
// Application Files
#include "LoRA_Functions.h"							// Where we store all the information on our LoRA implementation - application specific not a general library
#include "device_pinout.h"							// Define pinouts and initialize them
#include "particle_fn.h"							// Particle specific functions
#include "take_measurements.h"						// Manages interactions with the sensors (default is temp for charging)
#include "MyPersistentData.h"						// Where my persistent storage files are kept

// Support for Particle Products (changes coming in 4.x - https://docs.particle.io/cards/firmware/macros/product_id/)
PRODUCT_VERSION(0);
char currentPointRelease[6] ="0.09";

// Prototype functions
void publishStateTransition(void);                  // Keeps track of state machine changes - for debugging
void userSwitchISR();                               // interrupt service routime for the user switch
int secondsUntilNextEvent(); 						// Time till next scheduled event

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
LocalTimeSchedule publishSchedule;					// These allow us to enable a schedule and to use local time
LocalTimeConvert localTimeConvert_NOW;
void outOfMemoryHandler(system_event_t event, int param);

// MB85RC64 fram(Wire, 0);

// Program Variables
volatile bool userSwitchDectected = false;	
bool nextEventTime = false;	
bool testModeFlag = false;

void setup() 
{
	waitFor(Serial.isConnected, 10000);				// Wait for serial connection

    initializePinModes();                           // Sets the pinModes

    initializePowerCfg();                           // Sets the power configuration for solar

	{
		current.setup();
  		sysStatus.setup();
	}

    particleInitialize();                           // Sets up all the Particle functions and variables defined in particle_fn.h

    {                                               // Initialize AB1805 Watchdog and RTC                                 
        ab1805.withFOUT(D8).setup();                // The carrier board has D8 connected to FOUT for wake interrupts
        ab1805.setWDT(AB1805::WATCHDOG_MAX_SECONDS);// Enable watchdog
    }

	System.on(out_of_memory, outOfMemoryHandler);     // Enabling an out of memory handler is a good safety tip. If we run out of memory a System.reset() is done.

	PublishQueuePosix::instance().setup();          // Initialize PublishQueuePosixRK

	LoRA_Functions::instance().setup(true);			// Start the LoRA radio (true for Gateway and false for Node)

	// Setup local time and set the publishing schedule
	LocalTime::instance().withConfig(LocalTimePosixTimezone("EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00"));			// East coast of the US
	localTimeConvert_NOW.withCurrentTime().convert();  				        // Convert to local time for use later

  	Log.info("Gateway startup complete at %s with battery %4.2f", localTimeConvert_NOW.format(TIME_FORMAT_ISO8601_FULL).c_str(), System.batteryCharge());

	if (!digitalRead(BUTTON_PIN)) {
		Log.info("User button pressed, test mode");
		testModeFlag = true;
		digitalWrite(BLUE_LED,HIGH);
		delay(2000);
		digitalWrite(BLUE_LED,LOW);
		state = LoRA_STATE;
	}
	else Log.info("No user button push detechted");
	
	attachInterrupt(BUTTON_PIN,userSwitchISR,CHANGE); // We may need to monitor the user switch to change behaviours / modes

	if (state == INITIALIZATION_STATE) state = IDLE_STATE;  // This is not a bad way to start - could also go to the LoRA_STATE
}

void loop() {

	switch (state) {
		case IDLE_STATE: {
			if (state != oldState) publishStateTransition();                   // We will apply the back-offs before sending to ERROR state - so if we are here we will take action
			if (nextEventTime) {
				nextEventTime = false;
				state = LoRA_STATE;		   										// See Time section in setup for schedule
			}
			else state = SLEEPING_STATE;
		} break;

		case SLEEPING_STATE: {
			if (state != oldState) publishStateTransition();                   // We will apply the back-offs before sending to ERROR state - so if we are here we will take action
			ab1805.stopWDT();  												   // No watchdogs interrupting our slumber
			int wakeInSeconds = secondsUntilNextEvent();  		   		   	   // Time till next event
			Log.info("Sleep for %i seconds till next event at %s with %li free memory", wakeInSeconds, Time.timeStr(Time.now()+wakeInSeconds).c_str(),System.freeMemory());
			delay(2000);									// Make sure message gets out
			config.mode(SystemSleepMode::ULTRA_LOW_POWER)
				.gpio(BUTTON_PIN,CHANGE)
				.duration(wakeInSeconds * 1000L);
			SystemSleepResult result = System.sleep(config);                   // Put the device to sleep device continues operations from here
			ab1805.resumeWDT();                                                // Wakey Wakey - WDT can resume
			waitFor(Serial.isConnected, 10000);				// Wait for serial connection
			state = IDLE_STATE;
			nextEventTime = true;
			Log.info("Awoke at %s with %li free memory", Time.timeStr(Time.now()+wakeInSeconds).c_str(), System.freeMemory());
		} break;

		case LoRA_STATE: {														// Enter this state every reporting period and stay here for 5 minutes
			static system_tick_t startLoRAWindow = 0;

			if (state != oldState) {
				if (oldState != REPORTING_STATE) startLoRAWindow = millis();    // Mark when we enter this state - for timeouts - but multiple messages won't keep us here forever
				publishStateTransition();                   					// We will apply the back-offs before sending to ERROR state - so if we are here we will take action
				LoRA_Functions::instance().clearBuffer();						// Clear the buffer before we start the LoRA state
				Log.info("Gateway is listening for LoRA messages");
			} 

			if (LoRA_Functions::instance().listenForLoRAMessageGateway()) state = REPORTING_STATE; // Received and acknowledged data from a node - report

			if (!testModeFlag && ((millis() - startLoRAWindow) > 150000L)) { 								// Keeps us in listening mode for the specified windpw - then back to idle unless in test mode - keeps listening
				LoRA_Functions::instance().sleepLoRaRadio();												// Done with the LoRA phase - put the radio to sleep
				if (Time.hour() != Time.hour(sysStatus.get_lastConnection())) state = CONNECTING_STATE;  	// Only Connect once an hour after the LoRA window is over
				else state = IDLE_STATE;
			}

		} break;

		case REPORTING_STATE: {
			if (state != oldState) publishStateTransition();
		  	char data[256];                             						// Store the date in this character array - not global

  			snprintf(data, sizeof(data), "{\"nodeid\":%u, \"hourly\":%u, \"daily\":%u,\"battery\":%4.2f,\"key1\":\"%s\",\"temp\":%d, \"resets\":%d,\"rssi\":%d, \"msg\":%d,\"timestamp\":%lu000}", \
				current.get_deviceID(), current.get_hourlyCount(), current.get_dailyCount(), current.get_stateOfCharge(), batteryContext[current.get_batteryState()], current.get_internalTempC(), sysStatus.get_resetCount(), current.get_RSSI(), current.get_messageNumber(), Time.now());
  			PublishQueuePosix::instance().publish("Ubidots-LoRA-Hook-v1", data, PRIVATE | WITH_ACK);

			state = LoRA_STATE;
		} break;

		case CONNECTING_STATE: {
			static system_tick_t connectingTimeout = 0;

			if (state != oldState) {
				publishStateTransition();  
				if (!Particle.connected()) Particle.connect();
				connectingTimeout = millis();
			}

			if (Particle.connected() || millis() - connectingTimeout > 300000L) {		// Either we will connect or we will timeout 
				sysStatus.set_lastConnection(Time.now());
				state = DISCONNECTING_STATE;										// Typically, we will disconnect and sleep to save power
			}

		} break;

		case DISCONNECTING_STATE: {
			static system_tick_t stayConnectedWindow = 0;

			if (state != oldState) {
				publishStateTransition(); 
				stayConnectedWindow = millis(); 
			}

			if (millis() - stayConnectedWindow > 90000) {							// Stay on-line for 90 seconds
				disconnectFromParticle();
				state = IDLE_STATE;
			}
		} break;

		case ERROR_STATE: {
			static system_tick_t resetTimeout = millis();

			if (state != oldState) publishStateTransition();

			if (millis() - resetTimeout > 30000L) {
				Log.info("Deep power down device");
				delay(2000);
				ab1805.deepPowerDown(); 
			}
		} break;
	}

	ab1805.loop();                                  // Keeps the RTC synchronized with the Boron's clock

	PublishQueuePosix::instance().loop();           // Check to see if we need to tend to the message queue

	current.loop();
	
	sysStatus.loop();

	if (outOfMemory >= 0) {                         // In this function we are going to reset the system if there is an out of memory error
		System.reset();
  	}
}

/**
 * @brief Publishes a state transition to the Log Handler and to the Particle monitoring system.
 *
 * @details A good debugging tool.
 */
void publishStateTransition(void)
{
	char stateTransitionString[256];
	if (state == IDLE_STATE) {
		if (!Time.isValid()) snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s with invalid time", stateNames[oldState],stateNames[state]);
		else snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s for %u seconds", stateNames[oldState],stateNames[state],(secondsUntilNextEvent()));	
	}
	else snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s", stateNames[oldState],stateNames[state]);

	oldState = state;

	Log.info(stateTransitionString);
}

// Here are the various hardware and timer interrupt service routines
void outOfMemoryHandler(system_event_t event, int param) {
    outOfMemory = param;
}

void userSwitchISR() {
  userSwitchDectected = true;                                            // The the flag for the user switch interrupt
}

/**
 * @brief Function to compute time to next scheduled event
 * 
 * @details - Computes seconds and returns 10 if the device is in test mode or time is invalid
 * 
 * 
 */
int secondsUntilNextEvent() {											// Time till next scheduled event
	unsigned long secondsToReturn = 10;									// Minimum is 10 seconds to avoid transmit loop

	unsigned long wakeBoundary = sysStatus.get_frequencyMinutes() * 60UL;
   	if (Time.isValid() && !testModeFlag) {
		secondsToReturn = constrain( wakeBoundary - Time.now() % wakeBoundary, 10UL, wakeBoundary);  // In test mode, we will set a minimum of 10 seconds
        Log.info("Report frequency %d mins, next event in %lu seconds", sysStatus.get_frequencyMinutes(), secondsToReturn);
    }
	return secondsToReturn;
}

