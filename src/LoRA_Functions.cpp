#include "LoRA_Functions.h"
#include <RHMesh.h>
#include <RH_RF95.h>						        // https://docs.particle.io/reference/device-os/libraries/r/RH_RF95/
#include "device_pinout.h"
#include "MyPersistentData.h"


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
#define RH_MESH_MAX_MESSAGE_LEN 50
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
		if (gatewayID) {
			Log.info("Setting node number as Gateway");
			sysStatus.set_nodeNumber(0);
		} 
		else sysStatus.set_nodeNumber(random(10,255));	// Random number in - unconfigured - range will trigger a Join request
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
	uint8_t messageFlag;
	system_tick_t waitingFor = 0;
	if (manager.recvfromAckTimeout(buf, &len, 1000, &from,__null,__null,&messageFlag))	{	// We have received a message
		buf[len] = 0;
		current.set_deviceID(buf[0] << 8 | buf[1]);					// Set the current device ID for reporting
		current.set_nodeNumber(buf[2] << 8 | buf[3]);
		lora_state = (LoRA_State)(0x0F & messageFlag);				// Strip out the overhead byte
		Log.info("Received from node %d with rssi=%d - a %s message of length %d and waited for %lu mSec", current.get_nodeNumber(), driver.lastRssi(), loraStateNames[lora_state], len, waitingFor);

		if (lora_state == DATA_RPT) { if(LoRA_Functions::instance().decipherDataReportGateway()) return true;}
		if (lora_state == JOIN_REQ) { if(LoRA_Functions::instance().decipherJoinRequestGateway()) return true;}
		if (lora_state == ALERT_RPT) { if(LoRA_Functions::instance().decipherAlertReportGateway()) return true;}
	}
	return false; 
}

bool LoRA_Functions::respondForLoRAMessageGateway(int nextSeconds) {

	Log.info("Responding using the %s message type", loraStateNames[lora_state]);

	if (lora_state == DATA_ACK) { if(LoRA_Functions::instance().acknowledgeDataReportGateway(nextSeconds)) return true;}
	if (lora_state == JOIN_ACK) { if(LoRA_Functions::instance().acknowledgeJoinRequestGateway(nextSeconds)) return true;}
	if (lora_state == ALERT_ACK) { if(LoRA_Functions::instance().acknowledgeAlertReportGateway(nextSeconds)) return true;}

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

bool LoRA_Functions::acknowledgeDataReportGateway(int nextSeconds) {
	uint16_t nextSecondsShort = (uint16_t)nextSeconds;

	// This is a response to a data message it has a length of 9 and a specific payload and message flag
	// Send a reply back to the originator client
     
	buf[0] = current.get_messageNumber();			 		// Message number
	buf[1] = ((uint8_t) ((Time.now()) >> 24)); 		// Fourth byte - current time
	buf[2] = ((uint8_t) ((Time.now()) >> 16));		// Third byte
	buf[3] = ((uint8_t) ((Time.now()) >> 8));		// Second byte
	buf[4] = ((uint8_t) (Time.now()));		    	// First byte			
	buf[5] = highByte(sysStatus.get_frequencyMinutes());	// Frequency of reports - for Gateways
	buf[6] = lowByte(sysStatus.get_frequencyMinutes());	
	buf[7] = highByte(nextSecondsShort);	// Seconds until next report - for Nodes
	buf[8] = lowByte(nextSecondsShort);
	
	Log.info("Sent response to client message %d, time = %s, next report = %u seconds", buf[0], Time.timeStr(Time.now()).c_str(), nextSecondsShort);

	digitalWrite(BLUE_LED,HIGH);			        // Sending data

	if (manager.sendtoWait(buf, 9, current.get_nodeNumber(), DATA_ACK) == RH_ROUTER_ERROR_NONE) {
		Log.info("Response received successfully");
		digitalWrite(BLUE_LED,LOW);
		// driver.sleep();                             // Here is where we will power down the LoRA radio module
		return true;
	}

	Log.info("Response not acknowledged");
	digitalWrite(BLUE_LED,LOW);
	// driver.sleep();                             // Here is where we will power down the LoRA radio module
	return false;
}


// These are the receive and respond messages for join requests
bool LoRA_Functions::decipherJoinRequestGateway() {
	// Ths only question here is whether the node with the join request needs a new nodeNumber or is just looking for a clock set

	lora_state = JOIN_ACK;			// Prepare to respond

	return true;
}

bool LoRA_Functions::acknowledgeJoinRequestGateway(int nextSeconds) {
	uint16_t newNodeNumber = 0;
	uint16_t nextSecondsShort = (uint16_t)nextSeconds;

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
	buf[5] = highByte(newNodeNumber);			// New Node Number for device
	buf[6] = lowByte(newNodeNumber);	
	buf[7] = highByte(nextSecondsShort);		// Seconds until next report - for Nodes
	buf[8] = lowByte(nextSecondsShort);
	
	Log.info("Sent response to Node %d, time = %s, next report = %u seconds", newNodeNumber, Time.timeStr(Time.now()).c_str(), nextSecondsShort);

	digitalWrite(BLUE_LED,HIGH);			        // Sending data

	if (manager.sendtoWait(buf, 9, current.get_nodeNumber(), JOIN_ACK) == RH_ROUTER_ERROR_NONE) {
		Log.info("Response received successfully");
		digitalWrite(BLUE_LED,LOW);
		// driver.sleep();                             // Here is where we will power down the LoRA radio module
		return true;
	}

	Log.info("Response not acknowledged");
	digitalWrite(BLUE_LED,LOW);
	// driver.sleep();                             // Here is where we will power down the LoRA radio module
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

bool LoRA_Functions::acknowledgeAlertReportGateway(int nextSeconds) {
	uint16_t nextSecondsShort = (uint16_t)nextSeconds;
	Log.info("Preparing acknowledgement with %i seconds",nextSecondsShort);


	// This is a response to a data message it has a length of 9 and a specific payload and message flag
	// Send a reply back to the originator client
     
	buf[0] = 0;									// Reserved
	buf[1] = ((uint8_t) ((Time.now()) >> 24));  // Fourth byte - current time
	buf[2] = ((uint8_t) ((Time.now()) >> 16));	// Third byte
	buf[3] = ((uint8_t) ((Time.now()) >> 8));	// Second byte
	buf[4] = ((uint8_t) (Time.now()));		    // First byte			
	buf[5] = highByte(nextSecondsShort);		// Seconds until next report - for Nodes
	buf[6] = lowByte(nextSecondsShort);
	
	Log.info("Sent response to Node %d, time = %s, next report = %u seconds", current.get_nodeNumber(), Time.timeStr(Time.now()).c_str(), nextSecondsShort);

	digitalWrite(BLUE_LED,HIGH);			        // Sending data

	if (manager.sendtoWait(buf, 7, current.get_nodeNumber(), ALERT_ACK) == RH_ROUTER_ERROR_NONE) {
		Log.info("Response received successfully");
		digitalWrite(BLUE_LED,LOW);
		// driver.sleep();                             // Here is where we will power down the LoRA radio module
		return true;
	}

	Log.info("Response not acknowledged");
	digitalWrite(BLUE_LED,LOW);
	// driver.sleep();                             // Here is where we will power down the LoRA radio module
	return false;
}



// ************************************************************************
// *****                         Node Functions                       *****
// ************************************************************************
bool LoRA_Functions::listenForLoRAMessageNode() {
	uint8_t len = sizeof(buf);
	uint8_t from;  
	uint8_t messageFlag;	
	system_tick_t waitingFor = 0;
	if (manager.recvfromAckTimeout(buf, &len, 1000, &from,__null,__null,&messageFlag)) {
		buf[len] = 0;
		lora_state = (LoRA_State)messageFlag;
		Log.info("Received from node %d with rssi=%d - a %s message wated for %lu mSec", from, driver.lastRssi(), loraStateNames[lora_state], waitingFor);

		if (lora_state == DATA_ACK) { if(LoRA_Functions::instance().receiveAcknowledmentDataReportNode()) return true;}
		if (lora_state == JOIN_ACK) { if(LoRA_Functions::instance().receiveAcknowledmentJoinRequestNode()) return true;}
		if (lora_state == ALERT_ACK) { if(LoRA_Functions::instance().receiveAcknowledmentAlertReportNode()) return true;}
	}
	return false;
}


bool LoRA_Functions::composeDataReportNode() {
	static uint8_t msgCnt = 0;

	Log.info("Sending data report to Gateway");
	digitalWrite(BLUE_LED,HIGH);

	buf[0] = highByte(sysStatus.get_deviceID());					// Set for device
	buf[1] = lowByte(sysStatus.get_deviceID());
	buf[2] = highByte(sysStatus.get_nodeNumber());				// NodeID for verification
	buf[3] = lowByte(sysStatus.get_nodeNumber());				
	buf[4] = 1;						// Set for code release - fix later
	buf[5] = highByte(current.get_hourlyCount());
	buf[6] = lowByte(current.get_hourlyCount()); 
	buf[7] = highByte(current.get_dailyCount());
	buf[8] = lowByte(current.get_dailyCount()); 
	buf[9] = current.get_internalTempC();
	buf[10] = current.get_stateOfCharge();
	buf[11] = current.get_batteryState();	
	buf[12] = sysStatus.get_resetCount();
	buf[13] = 1;				// reserved for later
	buf[14] = highByte(driver.lastRssi());
	buf[15] = lowByte(driver.lastRssi()); 
	buf[16] = msgCnt++;

	// Send a message to manager_server
  	// A route to the destination will be automatically discovered.
	Log.info("sending message number %d", buf[16]);
	if (manager.sendtoWait(buf, 17, GATEWAY_ADDRESS, DATA_RPT) == RH_ROUTER_ERROR_NONE) {
		// It has been reliably delivered to the next node.
		// Now wait for a reply from the ultimate server 
		Log.info("Node %d - Data report send to gateway %d successfully", sysStatus.get_nodeNumber(), GATEWAY_ADDRESS);
		digitalWrite(BLUE_LED, LOW);
		return true;
	}
	else {
		Log.info("Node %d - Data report send to gateway %d failed", sysStatus.get_nodeNumber(), GATEWAY_ADDRESS);
		digitalWrite(BLUE_LED, LOW);
		return false;
	}
}

bool LoRA_Functions::receiveAcknowledmentDataReportNode() {

	Log.info("Node %d - Receiving acknowledgment - Data Report", sysStatus.get_nodeNumber());
		
	sysStatus.set_nextReportSeconds((buf[7] << 8) | buf[8]);
	uint32_t newTime = ((buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4]);
	Time.setTime(newTime);  // Set time based on response from gateway
	Log.info("Time set to %s and next report is in %u seconds at %s", Time.timeStr(newTime).c_str(),sysStatus.get_nextReportSeconds(), Time.timeStr(newTime + sysStatus.get_nextReportSeconds()).c_str());
	return true;
}

bool LoRA_Functions::composeJoinRequesttNode() {
	Log.info("Sending data report to Gateway");
	digitalWrite(BLUE_LED,HIGH);

	buf[0] = highByte(sysStatus.get_deviceID());                      // deviceID is unique to the device
	buf[1] = lowByte(sysStatus.get_deviceID());
	buf[2] = highByte(sysStatus.get_nodeNumber());                  			// Node Number
	buf[3] = lowByte(sysStatus.get_nodeNumber());
	buf[4] = sysStatus.get_structuresVersion();						// Needs to equal 128
	buf[5] = highByte(driver.lastRssi());				        // Signal strength
	buf[6] = lowByte(driver.lastRssi()); 

	
	// Send a message to manager_server
  	// A route to the destination will be automatically discovered.
	Log.info("Sending join request because %s",(sysStatus.get_nodeNumber() < 10) ? "a NodeNumber is needed" : "the clock is not set");
	if (manager.sendtoWait(buf, 7, GATEWAY_ADDRESS, JOIN_REQ) == RH_ROUTER_ERROR_NONE) {
		// It has been reliably delivered to the next node.
		// Now wait for a reply from the ultimate server 
		Log.info("Data report send to gateway successfully");
		digitalWrite(BLUE_LED, LOW);
		return true;
	}
	else {
		Log.info("Data report send to Gateway failed");
		digitalWrite(BLUE_LED, LOW);
		return false;
	}
}

bool LoRA_Functions::receiveAcknowledmentJoinRequestNode() {
	Log.info("Receiving acknowledgment - Join Request");

	if (sysStatus.get_nodeNumber() < 10 && buf[0] == 128) sysStatus.set_nodeNumber((buf[5] << 8 | buf[6]));
	sysStatus.set_nextReportSeconds((buf[7] << 8) | buf[8]);
	uint32_t newTime = ((buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4]);
	Time.setTime(newTime);  // Set time based on response from gateway
	Log.info("Time set to %s, node is %d and next report is in %u seconds", Time.timeStr(newTime).c_str(),sysStatus.get_nodeNumber(), sysStatus.get_nextReportSeconds());
	return true;
}

bool LoRA_Functions::composeAlertReportNode() {
	Log.info("Node - Sending Alert Report to Gateway");
	digitalWrite(BLUE_LED,HIGH);

	buf[0] = highByte(sysStatus.get_deviceID());       // deviceID is unique to the device
	buf[1] = lowByte(sysStatus.get_deviceID());
	buf[2] = highByte(sysStatus.get_nodeNumber());     // Node Number
	buf[3] = lowByte(sysStatus.get_nodeNumber());
	buf[4] = highByte(current.get_alertCodeNode());   // Node's Alert Code
	buf[5] = ((uint8_t) ((Time.now()) >> 24));  // Fourth byte - current time
	buf[6] = ((uint8_t) ((Time.now()) >> 16));	// Third byte
	buf[7] = ((uint8_t) ((Time.now()) >> 8));	// Second byte
	buf[8] = ((uint8_t) (Time.now()));		    // First byte			
	buf[9] = highByte(driver.lastRssi());		// Signal strength
	buf[10] = lowByte(driver.lastRssi()); 

	// Send a message to manager_server
  	// A route to the destination will be automatically discovered.
	Log.info("Sending Alert Report number %d to gateway at %d", current.get_alertCodeNode(), GATEWAY_ADDRESS);
	if (manager.sendtoWait(buf, 11, GATEWAY_ADDRESS, ALERT_RPT) == RH_ROUTER_ERROR_NONE) {
		// It has been reliably delivered to the next node.
		// Now wait for a reply from the ultimate server 
		Log.info("Node - Alert report send to gateway successfully");
		digitalWrite(BLUE_LED, LOW);
		return true;
	}
	else {
		Log.info("Node - Alert Report send to Gateway failed");
		digitalWrite(BLUE_LED, LOW);
		return false;
	}
}

bool LoRA_Functions::receiveAcknowledmentAlertReportNode() {

	Log.info("Receiving acknowledgment - Alert Report");

	uint32_t newTime = ((buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4]);
	sysStatus.set_nextReportSeconds((buf[5] << 8) | buf[6]);
	Time.setTime(newTime);  // Set time based on response from gateway
	Log.info("Time set to %s, node is %d and next report is in %u seconds", Time.timeStr(newTime).c_str(),sysStatus.get_nodeNumber(), sysStatus.get_nextReportSeconds());
	return true;
}


