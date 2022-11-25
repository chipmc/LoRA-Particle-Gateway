#include "LoRA_Functions.h"
#include <RHMesh.h>
#include <RH_RF95.h>						        // https://docs.particle.io/reference/device-os/libraries/r/RH_RF95/
#include "device_pinout.h"
#include "MyPersistentData.h"
#include "JsonParserGeneratorRK.h"
#include "Particle_Functions.h"

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
// ********            JSON Object - Global                     ***********
// ************************************************************************
// JSON for node data
JsonParserStatic<1024, 50> jp;						// Make this global - reduce possibility of fragmentation


// ************************************************************************
// *****                      LoRA Setup                              *****
// ************************************************************************
// In this implementation - we have one gateway with a fixed node  number of 0 and up to 10 nodes with node numbers 1-10 ...
// In an unconfigured node, there will be a node number of greater than 10 triggering a join request
// Configured nodes will be stored in a JSON object by the gateway with three fields: node number, Particle deviceID and Time stamp of last contact
//
const double RF95_FREQ = 915.0;				 // Frequency - ISM
const uint8_t GATEWAY_ADDRESS = 0;

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
	
	if (gatewayID == true) {
		sysStatus.set_nodeNumber(GATEWAY_ADDRESS);							// Gateway - Manager is initialized by default with GATEWAY_ADDRESS - make sure it is stored in FRAM
		Log.info("LoRA Radio initialized as a gateway with a deviceID of %s", System.deviceID().c_str());
	}
	else if (sysStatus.get_nodeNumber() > 0 && sysStatus.get_nodeNumber() <= 10) {
		manager.setThisAddress(sysStatus.get_nodeNumber());// Node - use the Node address in valid range from memory
		Log.info("LoRA Radio initialized as node %i and a deviceID of %s", manager.thisAddress(), System.deviceID().c_str());
	}
	else {																						// Else, we will set as an unconfigured node
		sysStatus.set_nodeNumber(11);
		manager.setThisAddress(11);
		Log.info("LoRA Radio initialized as an unconfigured node %i and a deviceID of %s", manager.thisAddress(), System.deviceID().c_str());
	}

	// Here is where we load the JSON object from memory and parse
	jp.addString(nodeID.get_nodeIDJson());				// Read in the JSON string from memory

	if (jp.parse()) Log.info("Parsed Successfully");
	else Log.info("Parsing error");

	printNodeData();

	return true;
}

void LoRA_Functions::loop() {
    // Put your code to run during the application thread loop here
}


// ************************************************************************
// *****					Common LoRA Functions					*******
// ************************************************************************


void LoRA_Functions::clearBuffer() {
	uint8_t bufT[RH_RF95_MAX_MESSAGE_LEN];
	uint8_t lenT;

	while(driver.recv(bufT, &lenT)) {};
}

void LoRA_Functions::sleepLoRaRadio() {
	driver.sleep();                             // Here is where we will power down the LoRA radio module
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

		if (!((buf[0] << 8 | buf[1]) == sysStatus.get_magicNumber())) {
			Log.info("Message magic number of %d did not match the Magic Number in memory %d - Ignoring", (buf[0] << 8 | buf[1]), sysStatus.get_magicNumber());
			return false;
		}
		else {
			current.set_nodeNumber(from);
			lora_state = (LoRA_State)(0x0F & messageFlag);				// Strip out the overhead byte
			Log.info("From node %d with rssi=%d - a %s message of length %d in %d hops", current.get_nodeNumber(), driver.lastRssi(), loraStateNames[lora_state], len, hops);
		}

		if (lora_state == DATA_RPT) { if(!LoRA_Functions::instance().decipherDataReportGateway()) return false;}
		else if (lora_state == JOIN_REQ) { if(!LoRA_Functions::instance().decipherJoinRequestGateway()) return false;}
		else if (lora_state == ALERT_RPT) { if(!LoRA_Functions::instance().decipherAlertReportGateway()) return false;}

		if (sysStatus.get_updatedFrequencyMinutes() > 0) {              							// If we are to change the update frequency, we need to tell the nodes (or at least one node) about it.
			sysStatus.set_frequencyMinutes(sysStatus.get_updatedFrequencyMinutes());		// This was the temporary value from the particle function
			sysStatus.set_updatedFrequencyMinutes(0);
			Log.info("We are updating the publish frequency to %i minutes", sysStatus.get_frequencyMinutes());

		}
		else Log.info("Reporting frequency unchanged");

		if (LoRA_Functions::instance().respondToLoRAMessageGateway()) return true;
		else return false;

	}
	return false;
}

bool LoRA_Functions::respondToLoRAMessageGateway() {

	Log.info("Responding using the %s message type", loraStateNames[lora_state]);

	if (lora_state == DATA_ACK) { if(LoRA_Functions::instance().acknowledgeDataReportGateway()) return true;}
	if (lora_state == JOIN_ACK) { if(LoRA_Functions::instance().acknowledgeJoinRequestGateway()) return true;}
	if (lora_state == ALERT_ACK) { if(LoRA_Functions::instance().acknowledgeAlertReportGateway()) return true;}

	return false; 
}

// These are the receive and respond messages for data reports

bool LoRA_Functions::decipherDataReportGateway() {			// Receives the data report and loads results into current object for reporting

	current.set_hourlyCount(buf[3] << 8 | buf[4]);
	current.set_dailyCount(buf[5] << 8 | buf[6]);
	current.set_sensorType(buf[7]);
	current.set_internalTempC(buf[8]);
	current.set_stateOfCharge(buf[9]);
	current.set_batteryState(buf[10]);
	current.set_resetCount(buf[11]);
	current.set_messageNumber(buf[12]);
	current.set_RSSI(driver.lastRssi());

	Log.info("Deciphered data report %d from node %d", current.get_messageNumber(), current.get_nodeNumber());

	lora_state = DATA_ACK;		// Prepare to respond

	return true;
}

bool LoRA_Functions::acknowledgeDataReportGateway() { 		// This is a response to a data message it has a length of 11 and a specific payload and message flag
	static int attempts = 0;
	static int success = 0;

	attempts++;
     
	buf[2] = ((uint8_t) ((Time.now()) >> 24)); 		// Fourth byte - current time
	buf[3] = ((uint8_t) ((Time.now()) >> 16));		// Third byte
	buf[4] = ((uint8_t) ((Time.now()) >> 8));		// Second byte
	buf[5] = ((uint8_t) (Time.now()));		    	// First byte			
	buf[6] = highByte(sysStatus.get_frequencyMinutes());	// Frequency of reports - for Gateways
	buf[7] = lowByte(sysStatus.get_frequencyMinutes());	
	buf[8] = current.get_openHours();
	// Next we will check to make sure that node number is properly configured in the nodeID object
	if (nodeConfigured(current.get_nodeNumber())) {			// Check to see if we need to set an alert code
		Log.info("Data Ack- Node number %d valid", current.get_nodeNumber());
		buf[9] = 0;											// Default is that there is no alert code
	}
	else {
		Log.info("Data Ack - Node number %d is invalid - resetting", current.get_nodeNumber());
		current.set_alertCodeNode(true);						// Store as this will impact reporting
		buf[9] = 1;												// Set the alert code
		current.set_alertTimestampNode(Time.now());

	}
	buf[10] = current.get_messageNumber();			 // Message number
	
	Log.info("Acknowlegment to %snode %d message %d, time = %s and frequency %d minutes", (buf[9] > 0) ? "misconfigured ":"", current.get_nodeNumber(), buf[10], Time.timeStr(Time.now()).c_str(), sysStatus.get_frequencyMinutes());

	printNodeData();

	digitalWrite(BLUE_LED,HIGH);			        // Sending data

	if (manager.sendtoWait(buf, 11, current.get_nodeNumber(), DATA_ACK) == RH_ROUTER_ERROR_NONE) {
		success++;
		Log.info("Response received successfully - success rate %4.2f", ((success * 1.0)/ attempts)*100.0);
		digitalWrite(BLUE_LED,LOW);
		return true;
	}
	else {
		Log.info("Response not acknowledged - success rate %4.2f", ((success * 1.0)/ attempts)*100.0);
		digitalWrite(BLUE_LED,LOW);
		return false;
	}
}


// These are the receive and respond messages for join requests
bool LoRA_Functions::decipherJoinRequestGateway() {

	char nodeDeviceID[25];
	// Ths only question here is whether the node with the join request needs a new nodeNumber or is just looking for a clock set

	lora_state = JOIN_ACK;			// Prepare to respond
	// Need to store the deviceID here
	for (uint8_t i=0; i<sizeof(nodeDeviceID); i++) {
		nodeDeviceID[i] = buf[i+2];
	}
	current.set_tempNodeNumber(current.get_nodeNumber());			// Store the old node number for the ack - also differentiates between unconfigured node and time set

	if (current.get_nodeNumber() > 10) {							// An unconfigured node
		current.set_nodeNumber(findNodeNumber(nodeDeviceID));		// Look up the new node number
		Log.info("Join request from Unconfigured node %s will change node number from %d to %d",  nodeDeviceID, current.get_tempNodeNumber(), current.get_nodeNumber());
	}
	else  {															// Node is already configured - set the clock and record the sensorType - note sensorType is ignored until the node number is set
		buf[27] = getType(current.get_nodeNumber());
		Log.info("Join request from %s with node %d - setting clock and setting sensorType to %d", LoRA_Functions::instance().findDeviceID(1).c_str(), current.get_nodeNumber(), buf[27]);
	}
	return true;
}

bool LoRA_Functions::acknowledgeJoinRequestGateway() {

	// This is a response to a data message it has a length of 9 and a specific payload and message flag
	// Send a reply back to the originator client
     
	buf[0] = highByte(sysStatus.get_magicNumber());					// Magic number - so you can trust me
	buf[1] = lowByte(sysStatus.get_magicNumber());					// Magic number - so you can trust me
	buf[2] = ((uint8_t) ((Time.now()) >> 24));  					// Fourth byte - current time
	buf[3] = ((uint8_t) ((Time.now()) >> 16));						// Third byte
	buf[4] = ((uint8_t) ((Time.now()) >> 8));						// Second byte
	buf[5] = ((uint8_t) (Time.now()));		    					// First byte		
	buf[6] = highByte(sysStatus.get_frequencyMinutes());			// Frequency of reports - for Gateways
	buf[7] = lowByte(sysStatus.get_frequencyMinutes());	
	buf[8] = current.get_nodeNumber();
	buf[9] = getType(current.get_nodeNumber());
	
	Log.info("Sent response to sensorType %d node %d, time = %s and frequency %d minutes",  buf[9], current.get_tempNodeNumber(), Time.timeStr().c_str(), sysStatus.get_frequencyMinutes());

	digitalWrite(BLUE_LED,HIGH);			        // Sending data

	if (manager.sendtoWait(buf, 10, current.get_tempNodeNumber(), JOIN_ACK) == RH_ROUTER_ERROR_NONE) {
		Log.info("Response received successfully");
		current.set_tempNodeNumber(current.get_nodeNumber());		// Temp no longer needed
		digitalWrite(BLUE_LED,LOW);
		return true;
	}
	else {
		Log.info("Response not acknowledged");							// Acknowledgement not received - this needs more attention as node is in undefined state
		digitalWrite(BLUE_LED,LOW);
		return false;
	}
}

// These are the receive and respond messages for Alert Reports
bool LoRA_Functions::decipherAlertReportGateway() {
	current.set_alertCodeNode(buf[2]);
	current.set_alertTimestampNode(Time.now());
	current.set_RSSI(driver.lastRssi() );
	Log.info("Deciphered alert report from node %d", current.get_nodeNumber());

	lora_state = ALERT_ACK;		// Prepare to respond

	return true;
}

bool LoRA_Functions::acknowledgeAlertReportGateway() {
	// uint16_t nextSecondsShort = (uint16_t)nextSeconds;

	// This is a response to a data message it has a length of 9 and a specific payload and message flag
	// Send a reply back to the originator client

	buf[0] = highByte(sysStatus.get_magicNumber());				// Magic number - so you can trust me
	buf[1] = lowByte(sysStatus.get_magicNumber());				// Magic number - so you can trust me
	buf[2] = ((uint8_t) ((Time.now()) >> 24));  				// Fourth byte - current time
	buf[3] = ((uint8_t) ((Time.now()) >> 16));					// Third byte
	buf[4] = ((uint8_t) ((Time.now()) >> 8));					// Second byte
	buf[5] = ((uint8_t) (Time.now()));		    				// First byte	
	buf[6] = highByte(sysStatus.get_frequencyMinutes());		// Frequency of reports - for Gateways
	buf[7] = lowByte(sysStatus.get_frequencyMinutes());			
	buf[8] = 0;													// Send a zero for the alertcode to reset it
	
	digitalWrite(BLUE_LED,HIGH);			        // Sending data

	if (manager.sendtoWait(buf, 9, current.get_nodeNumber(), ALERT_ACK) == RH_ROUTER_ERROR_NONE) {
		Log.info("Sent acknowledgment to Node %d, time = %s and frequency %d minutes", current.get_nodeNumber(), Time.timeStr().c_str(), sysStatus.get_frequencyMinutes());
		digitalWrite(BLUE_LED,LOW);
		return true;
	}

	Log.info("Response not acknowledged");
	digitalWrite(BLUE_LED,LOW);
	return false;
}

// These functions access data in the nodeID JSON
uint8_t LoRA_Functions::findNodeNumber(const char* deviceID) {

	int index=1;															// Variables to hold values for the function
	String nodeDeviceID;
	int nodeNumber;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i=0; i<10; i++) {												// Iterate through the array looking for a match
		nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
		if(nodeObjectContainer == NULL) {
			Log.info("findNodeNumber ran out of entries at i = %d",i);
			break;								// Ran out of entries - no match found
		} 
		jp.getValueByKey(nodeObjectContainer, "dID", nodeDeviceID);			// Get the deviceID and compare
		if (nodeDeviceID == deviceID) {
			jp.getValueByKey(nodeObjectContainer, "node", nodeNumber);		// A match!
			return nodeNumber;												// All is good - return node number for the deviceID passed to the function
		}
		index++;															// This will be the node number for the next node if no match is found
	}

	JsonModifier mod(jp);

	Log.info("New node will be assigned number %d, deviceID of %s and type of 3",index, deviceID);

	mod.startAppend(jp.getOuterArray());
		mod.startObject();
		mod.insertKeyValue("node", (int)index);
		mod.insertKeyValue("dID", deviceID);
		mod.insertKeyValue("last", Time.now());
		mod.insertKeyValue("type", (int)3);
		mod.finishObjectOrArray();
	mod.finish();

	nodeID.set_nodeIDJson(jp.getBuffer());									// This should backup the nodeID database - now updated to persistent storage

	Log.info("Returning index %d",index);

	return index;
}

String LoRA_Functions::findDeviceID(uint8_t nodeNumber)  {
	String nodeDeviceID;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) return "null";							// Ran out of entries - no match found
	jp.getValueByKey(nodeObjectContainer, "dID", nodeDeviceID);	// Get the deviceID and compare
	if (nodeDeviceID == NULL) return "null";
		
	return nodeDeviceID;
}

bool LoRA_Functions::nodeConfigured(int nodeNumber)  {

	int nodeNumberConfigured;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) return false;							// Ran out of entries - no match found
	jp.getValueByKey(nodeObjectContainer, "node", nodeNumberConfigured);	// Get the deviceID and compare
	if (nodeNumberConfigured == nodeNumber) return true;

	return true;
}

byte LoRA_Functions::getType(int nodeNumber) {

	int type;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) {
		Log.info("Node number not found");
		return 255;								// Ran out of entries 
	} 

	jp.getValueByKey(nodeObjectContainer, "type", type);

	return type;

}

bool LoRA_Functions::changetype(int nodeNumber, int Newtype) {
	int type;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) return false;								// Ran out of entries 

	jp.getValueByKey(nodeObjectContainer, "type", type);

	Log.info("Changing sensor type from %d to %d", type, Newtype);

	const JsonParserGeneratorRK::jsmntok_t *value;

	jp.getValueTokenByKey(nodeObjectContainer, "type", value);

	JsonModifier mod(jp);

	mod.startModify(value);

	mod.insertValue((int)Newtype);
	mod.finish();

	return true;

}

void LoRA_Functions::printNodeData() {

	int nodeNumber;
	String nodeDeviceID;
	int lastConnect;
	int sensorType;
	char data[128];

	Log.info("Print Node result:");

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i=0; i<10; i++) {												// Iterate through the array looking for a match
		nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
		if(nodeObjectContainer == NULL) {
			Log.info("Last entry at i = %d", i);
			break;								// Ran out of entries 
		} 
		jp.getValueByKey(nodeObjectContainer, "dID", nodeDeviceID);
		jp.getValueByKey(nodeObjectContainer, "node", nodeNumber);
		jp.getValueByKey(nodeObjectContainer, "last", lastConnect);
		jp.getValueByKey(nodeObjectContainer, "type", sensorType);

		snprintf(data, sizeof(data), "Node %d, deviceID: %s, lastConnected: %s, type %d", nodeNumber, nodeDeviceID.c_str(),Time.timeStr(lastConnect).c_str(), sensorType);
		Log.info(data);
		if (Particle.connected()) Particle.publish("nodeData", data, PRIVATE);
	}

	//#endif

}



