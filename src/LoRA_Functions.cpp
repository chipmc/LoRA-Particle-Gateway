#include "LoRA_Functions.h"
#include <RHMesh.h>
#include <RH_RF95.h>						        // https://docs.particle.io/reference/device-os/libraries/r/RH_RF95/
#include "device_pinout.h"
#include "MyPersistentData.h"
#include "particle_fn.h"


// Singleton instantiation - from template
LoRA_Functions *LoRA_Functions::_instance;

// [static]
LoRA_Functions &LoRA_Functions::instance() {
    if (!_instance) {
        _instance = new LoRA_Functions();
    }
    return *_instance;
}

LoRA_Functions::LoRA_Functions() {
}

LoRA_Functions::~LoRA_Functions() {
}


// ************************************************************************
// *****                      LoRA Setup                              *****
// ************************************************************************
// In this implementation - we have one gateway and two nodes (will generalize later) - nodes are numbered 2 to ...
// 2- 9 for new nodes making a join request
// 10 - 255 nodes assigned to the mesh
const uint8_t GATEWAY_ADDRESS = 0;			 // Gateway addess is always zero
const double RF95_FREQ = 915.0;				 // Frequency - ISM

// Define the message flags
typedef enum { NULL_STATE, JOIN_REQ, JOIN_ACK, DATA_RPT, DATA_ACK, ALERT_RPT, ALERT_ACK} LoRA_State;
char loraStateNames[7][16] = {"Null", "Join Req", "Join Ack", "Data Report", "Data Ack", "Alert Rpt", "Alert Ack"};
static LoRA_State lora_state = NULL_STATE;

// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHMesh manager(driver, GATEWAY_ADDRESS);

// Mesh has much greater memory requirements, and you may need to limit the
// max message length to prevent wierd crashes
#ifndef RH_MAX_MESSAGE_LEN
#define RH_MAX_MESSAGE_LEN 255
#endif

// Mesh has much greater memory requirements, and you may need to limit the
// max message length to prevent wierd crashes
// #define RH_MESH_MAX_MESSAGE_LEN 50
uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];               // Related to max message size - RadioHead example note: dont put this on the stack:


bool LoRA_Functions::setup(bool gatewayID) {
    // Set up the Radio Module
	if (!manager.init()) {
		Log.info("init failed");					// Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
		return false;
	}
	driver.setFrequency(RF95_FREQ);					// Frequency is typically 868.0 or 915.0 in the Americas, or 433.0 in the EU - Are there more settings possible here?
	driver.setTxPower(23, false);                   // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then you can set transmitter powers from 5 to 23 dBm (13dBm default).  PA_BOOST?

	if (!(sysStatus.get_structuresVersion() == 128)) {    	// This will be our indication that the deviceID and nodeID has not yet been set
		randomSeed(sysStatus.get_lastConnection());			// 32-bit number for seed
		sysStatus.set_deviceID(random(1,65535));			// 16-bit number for deviceID
		if (!gatewayID) sysStatus.set_nodeNumber(random(10,255));		// Random number in - unconfigured - range will trigger a Join request
		else sysStatus.set_nodeNumber(0);
		sysStatus.set_structuresVersion(128);			// Set the structure to the magic number so we can have a stable deviceID
	}

	manager.setThisAddress(sysStatus.get_nodeNumber());	// Assign the NodeNumber to this node
	
	Log.info("LoRA Radio initialized as NodeNumber of %i and DeviceID of %i and a magic number of %i", manager.thisAddress(), sysStatus.get_deviceID(), sysStatus.get_structuresVersion());
	return true;
}

void LoRA_Functions::loop() {
    // Put your code to run during the application thread loop here
}

// ************************************************************************
// *****                      Gateway Functions                       *****
// ************************************************************************

// Common across message types - these messages are general for send and receive

bool LoRA_Functions::listenForLoRAMessageGateway() {
	uint8_t len = sizeof(buf);
	uint8_t from;  
	uint8_t dest;
	uint8_t id;
	uint8_t messageFlag;
	uint8_t hops;
	if (manager.recvfromAck(buf, &len, &from, &dest, &id, &messageFlag, &hops))	{	// We have received a message
		buf[len] = 0;
		current.set_deviceID(buf[0] << 8 | buf[1]);					// Set the current device ID for reporting
		current.set_nodeNumber(buf[2] << 8 | buf[3]);
		lora_state = (LoRA_State)(0x0F & messageFlag);				// Strip out the overhead byte
		Log.info("From node %d / id %d to %d with rssi=%d - a %s message of length %d in %d hops", current.get_nodeNumber(), id, dest, driver.lastRssi(), loraStateNames[lora_state], len, hops);

		if (lora_state == DATA_RPT) { if(!LoRA_Functions::instance().decipherDataReportGateway()) return false;}
		if (lora_state == JOIN_REQ) { if(!LoRA_Functions::instance().decipherJoinRequestGateway()) return false;}
		if (lora_state == ALERT_RPT) { if(!LoRA_Functions::instance().decipherAlertReportGateway()) return false;}

		if (frequencyUpdated) {              							// If we are to change the update frequency, we need to tell the nodes (or at least one node) about it.
			frequencyUpdated = false;
			sysStatus.set_frequencyMinutes(updatedFrequencyMins);		// This was the temporary value from the particle function
			Log.info("We are updating the publish frequency to %i minutes", sysStatus.get_frequencyMinutes());
		}

		if (LoRA_Functions::instance().respondForLoRAMessageGateway()) return true;

	}
	return false; 
}

bool LoRA_Functions::respondForLoRAMessageGateway() {

	Log.info("Responding using the %s message type", loraStateNames[lora_state]);

	if (lora_state == DATA_ACK) { if(LoRA_Functions::instance().acknowledgeDataReportGateway()) return true;}
	if (lora_state == JOIN_ACK) { if(LoRA_Functions::instance().acknowledgeJoinRequestGateway()) return true;}
	if (lora_state == ALERT_ACK) { if(LoRA_Functions::instance().acknowledgeAlertReportGateway()) return true;}

	return false; 
}

// These are the receive and respond messages for data reports

bool LoRA_Functions::decipherDataReportGateway() {

	current.set_hourlyCount(buf[5] << 8 | buf[6]);
	current.set_dailyCount(buf[7] << 8 | buf[8]);
	current.set_stateOfCharge(buf[10]);
	current.set_batteryState(buf[11]);
	current.set_internalTempC(buf[9]);
	current.set_RSSI((buf[14] << 8 | buf[15]) - 65535);
	current.set_messageNumber(buf[16]);
	Log.info("Deciphered data report %d from node %d", current.get_messageNumber(), current.get_nodeNumber());

	lora_state = DATA_ACK;		// Prepare to respond

	return true;
}

bool LoRA_Functions::acknowledgeDataReportGateway() {
	static int attempts = 0;
	static int success = 0;

	// This is a response to a data message it has a length of 9 and a specific payload and message flag
	// Send a reply back to the originator client

	attempts++;
     
	buf[0] = current.get_messageNumber();			 		// Message number
	buf[1] = ((uint8_t) ((Time.now()) >> 24)); 		// Fourth byte - current time
	buf[2] = ((uint8_t) ((Time.now()) >> 16));		// Third byte
	buf[3] = ((uint8_t) ((Time.now()) >> 8));		// Second byte
	buf[4] = ((uint8_t) (Time.now()));		    	// First byte			
	buf[5] = highByte(sysStatus.get_frequencyMinutes());	// Frequency of reports - for Gateways
	buf[6] = lowByte(sysStatus.get_frequencyMinutes());	
	
	Log.info("Sent response to client message %d, time = %s and frequency %d minutes", buf[0], Time.timeStr(Time.now()).c_str(), sysStatus.get_frequencyMinutes());

	digitalWrite(BLUE_LED,HIGH);			        // Sending data

	if (manager.sendtoWait(buf, 9, current.get_nodeNumber(), DATA_ACK) == RH_ROUTER_ERROR_NONE) {
		success++;
		Log.info("Response received successfully - success rate %4.2f", ((success * 1.0)/ attempts)*100.0);
		digitalWrite(BLUE_LED,LOW);
		driver.sleep();                             // Here is where we will power down the LoRA radio module
		return true;
	}

	Log.info("Response not acknowledged - success rate %4.2f", ((success * 1.0)/ attempts)*100.0);
	digitalWrite(BLUE_LED,LOW);
	driver.sleep();                             // Here is where we will power down the LoRA radio module
	return false;
}


// These are the receive and respond messages for join requests
bool LoRA_Functions::decipherJoinRequestGateway() {
	// Ths only question here is whether the node with the join request needs a new nodeNumber or is just looking for a clock set

	lora_state = JOIN_ACK;			// Prepare to respond

	return true;
}

bool LoRA_Functions::acknowledgeJoinRequestGateway() {
	uint16_t newNodeNumber = 0;

	if (current.get_nodeNumber() < 10) {							// Device needs a new node number
		randomSeed(sysStatus.get_lastHookResponse());
		newNodeNumber = random(10,255);
	}

	// This is a response to a data message it has a length of 9 and a specific payload and message flag
	// Send a reply back to the originator client
     
	buf[0] = 128;								// Magic number - so you can trust me
	buf[1] = ((uint8_t) ((Time.now()) >> 24));  // Fourth byte - current time
	buf[2] = ((uint8_t) ((Time.now()) >> 16));	// Third byte
	buf[3] = ((uint8_t) ((Time.now()) >> 8));	// Second byte
	buf[4] = ((uint8_t) (Time.now()));		    // First byte		
	buf[5] = highByte(sysStatus.get_frequencyMinutes());	// Frequency of reports - for Gateways
	buf[6] = lowByte(sysStatus.get_frequencyMinutes());	
	buf[7] = highByte(newNodeNumber);			// New Node Number for device
	buf[8] = lowByte(newNodeNumber);	
	
	Log.info("Sent response to Node %d, time = %s and frequency %d minutes", newNodeNumber, Time.timeStr().c_str(), sysStatus.get_frequencyMinutes());

	digitalWrite(BLUE_LED,HIGH);			        // Sending data

	if (manager.sendtoWait(buf, 9, current.get_nodeNumber(), JOIN_ACK) == RH_ROUTER_ERROR_NONE) {
		Log.info("Response received successfully");
		digitalWrite(BLUE_LED,LOW);
		driver.sleep();                             // Here is where we will power down the LoRA radio module
		return true;
	}

	Log.info("Response not acknowledged");
	digitalWrite(BLUE_LED,LOW);
	driver.sleep();                             // Here is where we will power down the LoRA radio module
	return false;
}


bool LoRA_Functions::decipherAlertReportGateway() {
	current.set_alertCodeNode(buf[0]);
	current.set_alertTimestampNode(buf[1] << 24 | buf[2] << 16 | buf[3] <<8 | buf[4]);
	current.set_RSSI((buf[5] << 8 | buf[6]) - 65535);
	Log.info("Deciphered alert report from node %d", current.get_nodeNumber());

	lora_state = ALERT_ACK;		// Prepare to respond

	return true;
}

bool LoRA_Functions::acknowledgeAlertReportGateway() {
	// uint16_t nextSecondsShort = (uint16_t)nextSeconds;

	// This is a response to a data message it has a length of 9 and a specific payload and message flag
	// Send a reply back to the originator client
     
	buf[0] = 0;									// Reserved
	buf[1] = ((uint8_t) ((Time.now()) >> 24));  // Fourth byte - current time
	buf[2] = ((uint8_t) ((Time.now()) >> 16));	// Third byte
	buf[3] = ((uint8_t) ((Time.now()) >> 8));	// Second byte
	buf[4] = ((uint8_t) (Time.now()));		    // First byte	
	buf[5] = highByte(sysStatus.get_frequencyMinutes());	// Frequency of reports - for Gateways
	buf[6] = lowByte(sysStatus.get_frequencyMinutes());			
	
	digitalWrite(BLUE_LED,HIGH);			        // Sending data

	if (manager.sendtoWait(buf, 7, current.get_nodeNumber(), ALERT_ACK) == RH_ROUTER_ERROR_NONE) {
		Log.info("Sent acknowledgment to Node %d, time = %s and frequency %d minutes", current.get_nodeNumber(), Time.timeStr().c_str(), sysStatus.get_frequencyMinutes());
		digitalWrite(BLUE_LED,LOW);
		driver.sleep();                             // Here is where we will power down the LoRA radio module
		return true;
	}

	Log.info("Response not acknowledged");
	digitalWrite(BLUE_LED,LOW);
	driver.sleep();                             // Here is where we will power down the LoRA radio module
	return false;
}
