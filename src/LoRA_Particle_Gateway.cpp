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

// Particle Libraries
#include <RHMesh.h>
#include <RH_RF95.h>						        // https://docs.particle.io/reference/device-os/libraries/r/RH_RF95/
#include "PublishQueuePosixRK.h"			        // https://github.com/rickkas7/PublishQueuePosixRK
#include "LocalTimeRK.h"					        // https://rickkas7.github.io/LocalTimeRK/
#include "AB1805_RK.h"                              // Watchdog and Real Time Clock - https://github.com/rickkas7/AB1805_RK
#include "MB85RC256V-FRAM-RK.h"                     // Rickkas Particle based FRAM Library
#include "Particle.h"                               // Because it is a CPP file not INO
// Application Files
#include "LoRA_Functions.h"							// Where we store all the information on our LoRA implementation - application specific not a general library
#include "device_pinout.h"							// Define pinouts and initialize them
#include "particle_fn.h"							// Particle specific functions
#include "storage_objects.h"						// Manage the initialization and storage of persistent objects
#include "take_measurements.h"						// Manages interactions with the sensors (default is temp for charging)

// Support for Particle Products (changes coming in 4.x - https://docs.particle.io/cards/firmware/macros/product_id/)
PRODUCT_ID(PLATFORM_ID);                            // Device needs to be added to product ahead of time.  Remove once we go to deviceOS@4.x
PRODUCT_VERSION(0);
char currentPointRelease[6] ="0.04";

// Prototype functions
void publishStateTransition(void);                  // Keeps track of state machine changes - for debugging
void userSwitchISR();                               // interrupt service routime for the user switch
int secondsUntilNextEvent(); 						// Time till next scheduled event

// State Machine Variables
enum State { INITIALIZATION_STATE, ERROR_STATE, IDLE_STATE, SLEEPING_STATE, LoRA_STATE, CONNECTING_STATE, DISCONNECTING_STATE, REPORTING_STATE};
char stateNames[9][16] = {"Initialize", "Error", "Idle", "Sleeping", "LoRA", "Connecting", "Disconnecting", "Reporting"};
State state = INITIALIZATION_STATE;
State oldState = INITIALIZATION_STATE;

// Initialize Functions
SystemSleepConfiguration config;                    // Initialize new Sleep 2.0 Api
MB85RC64 fram(Wire, 0);                             // Rickkas' FRAM library
AB1805 ab1805(Wire);                                // Rickkas' RTC / Watchdog library
LocalTimeSchedule publishSchedule;					// These allow us to enable a schedule and to use local time
LocalTimeConvert localTimeConvert_NOW;

// Program Variables
volatile bool userSwitchDectected = false;		

void setup() 
{
	delay(5000);	// Wait for serial 

    initializePinModes();                           // Sets the pinModes

    initializePowerCfg();                           // Sets the power configuration for solar

    storageObjectStart();                           // Sets up the storage for system and current status in storage_objects.h

    particleInitialize();                           // Sets up all the Particle functions and variables defined in particle_fn.h

    {                                               // Initialize AB1805 Watchdog and RTC                                 
        ab1805.withFOUT(D8).setup();                // The carrier board has D8 connected to FOUT for wake interrupts
        ab1805.resetConfig();                       // Reset the AB1805 configuration to default values
        ab1805.setWDT(AB1805::WATCHDOG_MAX_SECONDS);// Enable watchdog
    }

	PublishQueuePosix::instance().setup();          // Initialize PublishQueuePosixRK

	initializeLoRA();								// Start the LoRA radio

	// Setup local time and set the publishing schedule
	LocalTime::instance().withConfig(LocalTimePosixTimezone("EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00"));			// East coast of the US
	localTimeConvert_NOW.withCurrentTime().convert();  				        // Convert to local time for use later
  	publishSchedule.withMinuteOfHour(sysStatus.frequencyMinutes, LocalTimeRange(LocalTimeHMS("06:00:00"), LocalTimeHMS("22:59:59")));	 // Publish every 15 minutes from 6am to 10pm

  	Log.info("Startup complete at %s with battery %4.2f", localTimeConvert_NOW.format(TIME_FORMAT_ISO8601_FULL).c_str(), System.batteryCharge());

  	attachInterrupt(BUTTON_PIN,userSwitchISR,CHANGE); // We may need to monitor the user switch to change behaviours / modes

	if (state == INITIALIZATION_STATE) state = LoRA_STATE;  // This is not a bad way to start - could also go to the LoRA_STATE
}

void loop() {
	switch (state) {
		case IDLE_STATE: {
			if (state != oldState) publishStateTransition();                   // We will apply the back-offs before sending to ERROR state - so if we are here we will take action
			if (publishSchedule.isScheduledTime()) state = LoRA_STATE;		   // See Time section in setup for schedule
			if (userSwitchDectected) state = CONNECTING_STATE;
		} break;

		case SLEEPING_STATE: {
			if (state != oldState) publishStateTransition();                   // We will apply the back-offs before sending to ERROR state - so if we are here we will take action
			ab1805.stopWDT();  												   // No watchdogs interrupting our slumber
			int wakeInSeconds = secondsUntilNextEvent()-10;  		   		   // Subtracting ten seconds to reduce prospect of round tripping to IDLE
			Log.info("Sleep for %i seconds", wakeInSeconds);
			config.mode(SystemSleepMode::ULTRA_LOW_POWER)
				.gpio(BUTTON_PIN,CHANGE)
				.duration(wakeInSeconds * 1000L);
			SystemSleepResult result = System.sleep(config);                   // Put the device to sleep device continues operations from here
			ab1805.resumeWDT();                                                // Wakey Wakey - WDT can resume
			if (result.wakeupPin() == BUTTON_PIN) {                            // If the user woke the device we need to get up - device was sleeping so we need to reset opening hours
				setLowPowerMode("0");                                          // We are waking the device for a reason
				Log.info("Woke with user button - normal operations");
			}
			state = IDLE_STATE;
		} break;

		case LoRA_STATE: {
			static system_tick_t startLoRAWindow = 0;

			if (state != oldState) {
				publishStateTransition();                   // We will apply the back-offs before sending to ERROR state - so if we are here we will take action
				startLoRAWindow = millis();               // Mark when we enter this state - for timeouts
				Log.info("In the LoRA state with a frequency of %u minutes", sysStatus.frequencyMinutes);
			} 

			if (listenForLoRAMessage()) {
				
				if (frequencyUpdated) {              // If we are to change the update frequency, we need to tell the nodes (or at least one node) about it.
					frequencyUpdated = false;
					publishSchedule.withMinuteOfHour(sysStatus.frequencyMinutes, LocalTimeRange(LocalTimeHMS("06:00:00"), LocalTimeHMS("21:59:59")));	 // Publish every 15 minutes from 6am to 10pm
				}
				sendLoRAMessage();					// Here we send our response based on the type of message received.
				state = REPORTING_STATE;
			}

			if ((millis() - startLoRAWindow) > 300000L) state = CONNECTING_STATE;	// This is a fail safe to make sure an off-line client won't prevent gatewat from checking in - and setting its clock

		} break;

		case REPORTING_STATE: {
			if (state != oldState) publishStateTransition();
		  	char data[256];                             // Store the date in this character array - not global

  			snprintf(data, sizeof(data), "{\"nodeid\":%u, \"hourly\":%u, \"daily\":%u,\"battery\":%d,\"key1\":\"%s\",\"temp\":%d, \"resets\":%d, \"alerts\":%d,\"rssi\":%d, \"msg\":%d,\"timestamp\":%lu000}",sysStatus.nodeNumber, current.hourly, current.daily, current.stateOfCharge, batteryContext[current.batteryState], current.internalTempC, sysStatus.resetCount, sysStatus.lastAlertCode, current.rssi, current.messageNumber, Time.now());
  			PublishQueuePosix::instance().publish("Ubidots-LoRA-Hook-v1", data, PRIVATE | WITH_ACK);

			if (!Particle.connected()) state = CONNECTING_STATE;                     // Now we will turn on the cellular radio and connect to Particle
			else if (sysStatus.lowPowerMode) state = DISCONNECTING_STATE;			// If we are connected but need to go to low power mode, we need to disconnect
			else state = IDLE_STATE;

		} break;

		case CONNECTING_STATE: {
			static system_tick_t connectingTimeout = 0;

			if (state != oldState) {
				publishStateTransition();  
				Particle.connect();
				connectingTimeout = millis();
			}

			if (Particle.connected() || millis() - connectingTimeout > 300000L) {		// Either we will connect or we will timeout 
				sysStatus.lastConnection = Time.now();
				if(Particle.connected() && !sysStatus.lowPowerMode) state = IDLE_STATE;	// We are connected and not low power - stay connected
				else state = DISCONNECTING_STATE;										// Typically, we will disconnect and sleep to save power
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
				if (sysStatus.lowPowerMode) state = SLEEPING_STATE;
				else state = IDLE_STATE; 										// Not sure if we need this
			}
		} break;
	}

	ab1805.loop();                                  // Keeps the RTC synchronized with the Boron's clock

	PublishQueuePosix::instance().loop();           // Check to see if we need to tend to the message queue

    storageObjectLoop();                            // Compares current system and current objects and stores if the hash changes (once / second) in storage_objects.h
}

/**
 * @brief Publishes a state transition to the Log Handler and to the Particle monitoring system.
 *
 * @details A good debugging tool.
 */
void publishStateTransition(void)
{
	char stateTransitionString[40];
	snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s", stateNames[oldState],stateNames[state]);
	oldState = state;
	if (Particle.connected()) {
		static time_t lastPublish = 0;
		if (millis() - lastPublish > 1000) {
			lastPublish = millis();
			Particle.publish("State Transition",stateTransitionString, PRIVATE);
		}
	}
	Log.info(stateTransitionString);
}

void userSwitchISR() {
  userSwitchDectected = true;                                            // The the flag for the user switch interrupt
}

/**
 * @brief Function to compute time to next scheduled event
 * 
 * @details - Computes seconds and returns 0 if no event is scheduled or time is invalid
 * 
 * 
 */
int secondsUntilNextEvent() {											// Time till next scheduled event
   if (Time.isValid()) {

        localTimeConvert_NOW.withCurrentTime().convert();
        Log.info("local time: %s", localTimeConvert_NOW.format(TIME_FORMAT_DEFAULT).c_str());

        LocalTimeConvert localTimeConvert_NEXT;
        localTimeConvert_NEXT.withCurrentTime().convert();

		if (publishSchedule.getNextScheduledTime(localTimeConvert_NEXT)) {
			long unsigned secondsToReturn = constrain(localTimeConvert_NEXT.time - localTimeConvert_NOW.time, 0L, 86400L);	// Constrain to positive seconds less than or equal to a day.
        	Log.info("time of next event is: %s which is %lu seconds away", localTimeConvert_NEXT.format(TIME_FORMAT_DEFAULT).c_str(), secondsToReturn);
			return secondsToReturn;
		}
		else return 0;
    }
	else return 0;
}