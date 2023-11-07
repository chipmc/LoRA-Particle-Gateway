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
// ******** JSON Object - Scoped to LoRA_Functions Class        ***********
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
const uint8_t GATEWAY_ADDRESS = 0;
// const double RF95_FREQ = 915.0;				 	// Frequency - ISM
const double RF95_FREQ = 926.84;				// Center frequency for the omni-directional antenna I am using

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
	LoRA_Functions::initializeRadio();

	if (gatewayID == true) {
		sysStatus.set_nodeNumber(GATEWAY_ADDRESS);							// Gateway - Manager is initialized by default with GATEWAY_ADDRESS - make sure it is stored in FRAM
		Log.info("LoRA Radio initialized as a gateway (address %d) with a deviceID of %s", GATEWAY_ADDRESS, System.deviceID().c_str());
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
	jp.addString(nodeDatabase.get_nodeIDJson());				// Read in the JSON string from memory
	Log.info("The node string is: %s",nodeDatabase.get_nodeIDJson().c_str());

	if (jp.parse()) Log.info("Parsed Successfully");
	else {
		nodeDatabase.resetNodeIDs();
		Log.info("Parsing error resetting nodeID database");
	}
	return true;
}

void LoRA_Functions::loop() {
												
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
	driver.sleep();                             	// Here is where we will power down the LoRA radio module
}

bool  LoRA_Functions::initializeRadio() {  			// Set up the Radio Module
	digitalWrite(RFM95_RST,LOW);					// Reset the radio module before setup
	delay(10);
	digitalWrite(RFM95_RST,HIGH);
	delay(10);

	if (!manager.init()) {
		Log.info("init failed");					// Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
		return false;
	}
	driver.setFrequency(RF95_FREQ);					// Frequency is typically 868.0 or 915.0 in the Americas, or 433.0 in the EU - Are there more settings possible here?
	driver.setTxPower(23, false);                   // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then you can set transmitter powers from 5 to 23 dBm (13dBm default).  PA_BOOST?
	// driver.setModemConfig(RH_RF95::Bw125Cr45Sf2048);  // This is the setting appropriate for parks
	driver.setModemConfig(RH_RF95::Bw500Cr45Sf128);	 // Optimized for fast transmission and short range - MAFC
	//driver.setModemConfig(RH_RF95::Bw125Cr48Sf4096);	// This optimized the radio for long range - https://www.airspayce.com/mikem/arduino/RadioHead/classRH__RF95.html
	driver.setLowDatarate();						// https://www.airspayce.com/mikem/arduino/RadioHead/classRH__RF95.html#a8e2df6a6d2cb192b13bd572a7005da67
	manager.setTimeout(1000);						// 200mSec is the default - may need to extend once we play with other settings on the modem - https://www.airspayce.com/mikem/arduino/RadioHead/classRHReliableDatagram.html
return true;
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
	if (manager.recvfromAck(buf, &len, &from, &dest, &id, &messageFlag, &hops))	{	// We have received a message - need to validate it
		buf[len] = 0;

		// First we will validate that this node belongs in this network by checking the magic number
		if (!((buf[0] << 8 | buf[1]) == sysStatus.get_magicNumber())) {
			Log.info("Node %d message magic number of %d did not match the Magic Number in memory %d - Ignoring", current.get_nodeNumber(),(buf[0] << 8 | buf[1]), sysStatus.get_magicNumber());
			return false;
		}
		current.set_nodeNumber(from);												// Captures the nodeNumber 
		current.set_tempNodeNumber(0);												// Clear for new response
		current.set_hops(hops);														// How many hops to get here
		current.set_nodeID(buf[2] << 8 | buf[3]);									// Captures the nodeID for Data or Alert reports
		lora_state = (LoRA_State)(0x0F & messageFlag);								// Strip out the overhead byte to get the message flag
		Log.info("Node %d with ID %d a %s message with RSSI/SNR of %d / %d in %d hops", current.get_nodeNumber(), current.get_nodeID(), loraStateNames[lora_state], driver.lastRssi(), driver.lastSNR(), current.get_hops());

		// Next we need to test the nodeNumber / deviceID to make sure this node is properly configured
		if (current.get_nodeNumber() < 11 && !LoRA_Functions::instance().nodeConfigured(current.get_nodeNumber(),current.get_nodeID())) {
			Log.info("Node not properly configured, resetting node number");
			current.set_tempNodeNumber(current.get_nodeNumber());					// Store node number in temp for the repsonse
			current.set_nodeNumber(11);												// Set node number to 11
		}
		else if (current.get_nodeNumber() >= 11) {
			current.set_tempNodeNumber(from);										// We need this address for the reply					
			current.set_nodeNumber(11);												// This way an unconfigured nor invalid node ends up wtih a node number of 11
		}

		if (lora_state == DATA_RPT) {if(!LoRA_Functions::instance().decipherDataReportGateway()) return false;}
		else if (lora_state == JOIN_REQ) {if(!LoRA_Functions::instance().decipherJoinRequestGateway()) return false;}
		else {Log.info("Invalid message flag, returning"); return false;}

		// At this point the message is valid and has been deciphered - now we need to send a response - if there is a change in freuqency, it is applied here
		if (sysStatus.get_updatedFrequencyMinutes() > 0) {              			// If we are to change the update frequency, we need to tell the nodes (or at least one node) about it.
			sysStatus.set_frequencyMinutes(sysStatus.get_updatedFrequencyMinutes());// This was the temporary value from the particle function
			sysStatus.set_updatedFrequencyMinutes(0);
			Log.info("We are updating the publish frequency to %i minutes", sysStatus.get_frequencyMinutes());
		}
		// The response will be specific to the message type
		if (lora_state == DATA_ACK) { if(LoRA_Functions::instance().acknowledgeDataReportGateway()) return true;}
		else if (lora_state == JOIN_ACK) { if(LoRA_Functions::instance().acknowledgeJoinRequestGateway()) return true;}
		else {Log.info("Invalid message flag"); return false;}
	}
	else LoRA_Functions::clearBuffer();
	return false;

}

// These are the receive and respond messages for data reports

bool LoRA_Functions::decipherDataReportGateway() {			// Receives the data report and loads results into current object for reporting

	union floatToByte
	{
	unsigned char buf[4];
	float number;
	} floatToByte;

	current.set_sensorType(buf[8]);
	if (current.get_sensorType() <=2) {
		current.set_hourlyCount(buf[4] << 8 | buf[5]);
		current.set_dailyCount(buf[6] << 8 | buf[7]);
		Log.info("Sensor type of %d with hourly count of %d and daily count of %d", current.get_sensorType(), current.get_hourlyCount(), current.get_dailyCount());
	}
	else {
		floatToByte.buf[3] = buf[4];
		floatToByte.buf[2] = buf[5];
		floatToByte.buf[1] = buf[6];
		floatToByte.buf[0] = buf[7];
		current.set_soilVWC(floatToByte.number);
		Log.info("Soil VWC is %4.2f - %d/%d/%d/%d",current.get_soilVWC(),floatToByte.buf[3],floatToByte.buf[2],floatToByte.buf[1],floatToByte.buf[0]);
	}
	current.set_internalTempC(buf[9]);
	current.set_stateOfCharge(buf[10]);
	current.set_batteryState(buf[11]);
	current.set_resetCount(buf[12]);
	current.set_messageCount(buf[13]);
	current.set_successCount(buf[14]);
	current.set_RSSI(buf[15] << 8 | buf[16]);				// These values are from the node based on the last successful data report
	current.set_SNR(buf[17] << 8 | buf[18]);

	Log.info("Data recieved from the report: hourly %d, daily %d, sensorType %d, temp %d, battery %d, batteryState %d, resets %d, message count %d, success count %d, RSSI %d, SNR %d", (buf[4] << 8 | buf[5]), (buf[6] <<8 | buf[7]), buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15] << 8 | buf[16], buf[17] << 8 | buf[18]);

	lora_state = DATA_ACK;		// Prepare to respond
	return true;
}

bool LoRA_Functions::acknowledgeDataReportGateway() { 		// This is a response to a data message 
	char messageString[128];

	// buf[0] - buf[1] is magic number - processed above
	buf[2] = ((uint8_t) ((Time.now()) >> 24)); 		// Fourth byte - current time
	buf[3] = ((uint8_t) ((Time.now()) >> 16));		// Third byte
	buf[4] = ((uint8_t) ((Time.now()) >> 8));		// Second byte
	buf[5] = ((uint8_t) (Time.now()));		    	// First byte			
	buf[6] = highByte(sysStatus.get_frequencyMinutes());	// Frequency of reports set by the gateway
	buf[7] = lowByte(sysStatus.get_frequencyMinutes());	
	// The next few bytes of the response will depend on whether the node is configured or not
	if (current.get_nodeNumber() == 11) {			// This is a data report from an unconfigured node - need to tell it to rejoin
		Log.info("Node %d is invalid, setting alert code to 1", current.get_nodeNumber());
		current.set_alertCodeNode(1);				// This will ensure the node rejoins the network
		current.set_alertTimestampNode(Time.now());
		buf[8] = current.get_alertCodeNode();												
		buf[9] = current.get_sensorType();			// Since the node is unconfigured, we need to beleive it when it tells us the type
	}
	else {											// This is a data report from a configured node - will use the node database
		current.set_alertCodeNode(LoRA_Functions::getAlert(current.get_nodeNumber()));
		if (current.get_alertCodeNode() > 0) Log.info("Node %d has a pending alert %d", current.get_nodeNumber(), current.get_alertCodeNode());
	
		if (current.get_alertCodeNode() == 7) {		// if it is a change in type alert - we can do that here
			int newSensorType = LoRA_Functions::getType(current.get_nodeNumber());
			Log.info("In data acknowledge, changing type to from %d to %d", current.get_sensorType(), newSensorType );
			current.set_sensorType(newSensorType);	// Update current value for data report
			buf[9] = newSensorType;
		}
		else buf[9] = current.get_sensorType();

		if (current.get_alertCodeNode() != 0) LoRA_Functions::changeAlert(current.get_nodeNumber(),0); 	// The alert was serviced or applied - no longer pending
		buf[8] = current.get_alertCodeNode();

		// At this point we can update the last connection time in the node database
		float successPercent;
		if (current.get_messageCount()==0) successPercent = 0.0;
		else successPercent = ((current.get_successCount()+1.0)/(float)current.get_messageCount()) * 100.0;  // Add one to success because we are receving the message
		LoRA_Functions::instance().nodeUpdate(current.get_nodeNumber(), successPercent);

	}
	buf[10] = current.get_openHours();
	buf[11] = current.get_messageCount();			// Repeat back message number

	nodeDatabase.flush(true);						// Save updates to the nodID database
	current.flush(true);							// Save values reported by the nodes
	digitalWrite(BLUE_LED,HIGH);			       	// Sending data

	byte nodeAddress = (current.get_tempNodeNumber() == 0) ? current.get_nodeNumber() : current.get_tempNodeNumber();  // get the return address right

	if (manager.sendtoWait(buf, 12, nodeAddress, DATA_ACK) == RH_ROUTER_ERROR_NONE) {
		digitalWrite(BLUE_LED,LOW);

		snprintf(messageString,sizeof(messageString),"Node %d data report %d acknowledged with alert %d, and RSSI / SNR of %d / %d", current.get_nodeNumber(), buf[10], buf[8], current.get_RSSI(), current.get_SNR());
		Log.info(messageString);
		if (Particle.connected()) Particle.publish("status", messageString,PRIVATE);
		return true;
	}
	else {
		Log.info("Node %d data report response not acknowledged", nodeAddress);
		digitalWrite(BLUE_LED,LOW);
		return false;
	}
}


// These are the receive and respond messages for join requests
bool LoRA_Functions::decipherJoinRequestGateway() {			// Ths only question here is whether the node with the join request needs a new nodeNumber or is just looking for a clock set
	// buf[0] - buf[1] Magic number processed above
	// buf[2] - buf[3] NodeID processed above
	current.set_sensorType(buf[4]);								// Store device type in the current data buffer 
	current.set_nodeNumber(findNodeNumber(current.get_nodeNumber(), current.get_nodeID()));		// Look up the new node number
	
	Log.info("Node %d join request from nodeID %d will change node number to %d", current.get_tempNodeNumber(), current.get_nodeID(),current.get_nodeNumber());

	current.set_alertCodeNode(1);									// This is a join request so alert code is 1
	current.set_alertTimestampNode(Time.now());

	LoRA_Functions::changeType(current.get_nodeNumber(),current.get_sensorType());  // Record the sensor type in the nodeID structure

	lora_state = JOIN_ACK;			// Prepare to respond
	return true;
}

bool LoRA_Functions::acknowledgeJoinRequestGateway() {
	char messageString[128];
	Log.info("Acknowledge Join Request");
	// This is a response to a data message and a specific payload and message flag
	// Send a reply back to the originator client
     
	buf[0] = highByte(sysStatus.get_magicNumber());					// Magic number - so you can trust me
	buf[1] = lowByte(sysStatus.get_magicNumber());					// Magic number - so you can trust me
	buf[2] = ((uint8_t) ((Time.now()) >> 24));  					// Fourth byte - current time
	buf[3] = ((uint8_t) ((Time.now()) >> 16));						// Third byte
	buf[4] = ((uint8_t) ((Time.now()) >> 8));						// Second byte
	buf[5] = ((uint8_t) (Time.now()));		    					// First byte		
	buf[6] = highByte(sysStatus.get_frequencyMinutes());			// Frequency of reports - for Gateways
	buf[7] = lowByte(sysStatus.get_frequencyMinutes());	
	buf[8] = (current.get_nodeNumber() != 11) ?  0 : 1;				// Clear the alert code for the node unless the nodeNumber process failed
	buf[9] = current.get_nodeNumber();								
	buf[10] = current.get_sensorType();								// In a join request the node type overwrites the node database value


	digitalWrite(BLUE_LED,HIGH);			        				// Sending data

	byte nodeAddress = (current.get_tempNodeNumber() == 0) ? current.get_nodeNumber() : current.get_tempNodeNumber();  // get the return address right

	Log.info("Sending response to %d with free memory = %li", nodeAddress, System.freeMemory());

	if (manager.sendtoWait(buf, 11, nodeAddress, JOIN_ACK) == RH_ROUTER_ERROR_NONE) {
		current.set_tempNodeNumber(0);								// Temp no longer needed
		digitalWrite(BLUE_LED,LOW);
		snprintf(messageString,sizeof(messageString),"Node %d joined with sensorType %s, alert %d and RSSI / SNR of %d / %d", nodeAddress, (buf[10] ==0)? "car":"person",current.get_alertCodeNode(), current.get_RSSI(), current.get_SNR());
		Log.info(messageString);
		if (Particle.connected()) Particle.publish("status", messageString,PRIVATE);
		return true;
	}
	else {
		Log.info("Node %d join response not acknowledged", current.get_tempNodeNumber()); // Acknowledgement not received - this needs more attention as node is in undefined state
		digitalWrite(BLUE_LED,LOW);
		return false;
	}
}

// ************************************************************************
// *****             Node Management  Functions                       *****
// ************************************************************************

/* NodeID JSON structure
{nodes:[
	{
		{node:(int)nodeNumber},
		{nID: (int)nodeID},
		{last: (time_t)lastConnectTime},
		{type: (int)sensorType},
		{succ: (float)successfulSent%},
		{pend: (int)pendingAlerts}
	},
	....]
}
*/


// These functions access data in the nodeID JSON
uint8_t LoRA_Functions::findNodeNumber(int nodeNumber, int nodeID) {
	int index=1;															// Variables to hold values for the function
	int nodeDeviceNumber;
	int nodeDeviceID;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i=0; i<10; i++) {												// Iterate through the array looking for a match
		nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
		if(nodeObjectContainer == NULL) {
			Log.info("findNodeNumber ran out of entries at i = %d",i);
			break;															// Ran out of entries - no match found
		} 
		jp.getValueByKey(nodeObjectContainer, "nID", nodeDeviceID);			// Get the deviceID and compare
		if (nodeDeviceID == nodeID) {
			jp.getValueByKey(nodeObjectContainer, "node", nodeDeviceNumber);		// A match!
			return nodeDeviceNumber;												// All is good - return node number for the deviceID passed to the function
		}
		index++;															// This will be the node number for the next node if no match is found
	}
	// If we got to here, the nodeID was not a match for any entry and a new nodeNumer will be assigned
	nodeNumber = index;
	JsonModifier mod(jp);

	Log.info("New node will be assigned node number %d, nodeID of %d",nodeNumber, nodeID);

	mod.startAppend(jp.getOuterArray());
		mod.startObject();
		mod.insertKeyValue("node", nodeNumber);
		mod.insertKeyValue("nID", nodeID);
		mod.insertKeyValue("last", Time.now());
		mod.insertKeyValue("type", (int)9);									// This is a temp value that will be updated
		mod.insertKeyValue("succ",(float)0.0);								// This is a temp value that will be updated
		mod.insertKeyValue("pend",(int)0);
		mod.finishObjectOrArray();
	mod.finish();

	nodeDatabase.set_nodeIDJson(jp.getBuffer());									// This should backup the nodeID database - now updated to persistent storage

	return index;
}

String LoRA_Functions::findDeviceID(int nodeNumber, int radioID)  {
	String nodeDeviceID;
	int nodeRadioID;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) return "null";							// Ran out of entries - no match found

	jp.getValueByKey(nodeObjectContainer,"rID", nodeRadioID);				// Get the radioID to see if it is a match
	if (nodeRadioID != radioID) return "null";								// Not the right nodeNumber / nodeID combo

	jp.getValueByKey(nodeObjectContainer, "dID", nodeDeviceID);				// Get the deviceID and compare
	if (nodeDeviceID == NULL) return "null";
	else return nodeDeviceID;
}

bool LoRA_Functions::nodeConfigured(int nodeNumber, int radioID)  {
	if (nodeNumber > 10) return false;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) return false;							// Ran out of entries - no match found
	
	jp.getValueByKey(nodeObjectContainer, "rID", radioID);					// Get the radioID for the node number in question

	if (radioID == current.get_nodeID()) return true;
	else {
		Log.info("Node not configured");  // See the raw JSON string
		return false;
	}
}

bool LoRA_Functions::nodeUpdate(int nodeNumber, float successPercent)  {

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) return false;							// Ran out of entries - no match found

	const JsonParserGeneratorRK::jsmntok_t *value;

	jp.getValueTokenByKey(nodeObjectContainer, "last", value);			// Update last connection time
	JsonModifier mod(jp);
	mod.startModify(value);
	mod.insertValue((int)Time.now());
	mod.finish();

	jp.getValueTokenByKey(nodeObjectContainer, "succ", value);			// Update the success percentage value
	mod.startModify(value);
	mod.insertValue((float)successPercent);
	mod.finish();

	nodeDatabase.set_nodeIDJson(jp.getBuffer());									// This should backup the nodeID database - now updated to persistent storage
	return true;
}

byte LoRA_Functions::getType(int nodeNumber) {
	int type;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) {
		Log.info("From getType function Node number not found so returning %d",current.get_sensorType());
		return current.get_sensorType();									// Ran out of entries, go with what was reported by the node
	} 

	jp.getValueByKey(nodeObjectContainer, "type", type);
	Log.info("Returning sensor type %d",type);
	return type;
}

bool LoRA_Functions::changeType(int nodeNumber, int newType) {
	if (nodeNumber > 10) return false;
	int type;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) return false;								// Ran out of entries 

	jp.getValueByKey(nodeObjectContainer, "type", type);

	Log.info("Changing sensor type from %d to %d", type, newType);

	const JsonParserGeneratorRK::jsmntok_t *value;

	jp.getValueTokenByKey(nodeObjectContainer, "type", value);

	JsonModifier mod(jp);

	mod.startModify(value);

	mod.insertValue((int)newType);
	mod.finish();

	nodeDatabase.set_nodeIDJson(jp.getBuffer());									// This should backup the nodeID database - now updated to persistent storage

	return true;

}

byte LoRA_Functions::getAlert(int nodeNumber) {
	if (nodeNumber > 10) return 255;										// Not a configured node

	int pendingAlert;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) {
		Log.info("From getAlert function, Node number not found");
		return 255;															// Ran out of entries 
	} 

	jp.getValueByKey(nodeObjectContainer, "pend", pendingAlert);

	return pendingAlert;

}

bool LoRA_Functions::changeAlert(int nodeNumber, int newAlert) {
	int currentAlert;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);	// find the entry for the node of interest
	if(nodeObjectContainer == NULL) return false;							// Ran out of entries - node number entry not found triggers alert

	jp.getValueByKey(nodeObjectContainer, "pend", currentAlert);			// Now we have the oject for the specific node
	Log.info("Changing pending alert from %d to %d", currentAlert, newAlert);

	const JsonParserGeneratorRK::jsmntok_t *value;							// Node we have the key value pair for the "pend"ing alerts	
	jp.getValueTokenByKey(nodeObjectContainer, "pend", value);

	JsonModifier mod(jp);													// Create a modifier object
	mod.startModify(value);													// Update the pending alert value for the selected node
	mod.insertValue((int)newAlert);
	mod.finish();

	nodeDatabase.set_nodeIDJson(jp.getBuffer());									// This updates the JSON object but doe not commit to to persistent storage

	return true;
}

void LoRA_Functions::printNodeData(bool publish) {
	int nodeNumber;
	int nodeID;
	int lastConnect;
	int sensorType;
	float successPercent;
	int pendingAlert;
	char data[256];

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i=0; i<10; i++) {												// Iterate through the array looking for a match
		nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
		if(nodeObjectContainer == NULL) {
			break;								// Ran out of entries 
		} 
		jp.getValueByKey(nodeObjectContainer, "nID", nodeID);
		jp.getValueByKey(nodeObjectContainer, "node", nodeNumber);
		jp.getValueByKey(nodeObjectContainer, "last", lastConnect);
		jp.getValueByKey(nodeObjectContainer, "type", sensorType);
		jp.getValueByKey(nodeObjectContainer, "succ", successPercent);
		jp.getValueByKey(nodeObjectContainer, "pend", pendingAlert);

		snprintf(data, sizeof(data), "Node %d, nodeID %d, lastConnected: %s, type %d, success %4.2f with pending alert %d", nodeNumber, nodeID, Time.timeStr(lastConnect).c_str(), sensorType, successPercent, pendingAlert);
		Log.info(data);
		if (Particle.connected() && publish) {
			Particle.publish("nodeData", data, PRIVATE);
			delay(1000);
		}
	}

	// Log.info(nodeDatabase.get_nodeIDJson());  // See the raw JSON string

}

bool LoRA_Functions::nodeConnectionsHealthy() {								// Connections are healthy if at least one node connected in last two periods
// Resets the LoRA Radio if not healthy
	
	int lastConnect;
	time_t secondsPerPeriod = sysStatus.get_frequencyMinutes() * 60;
	bool health = true;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i=0; i<10; i++) {												// Iterate through the array looking for a match
		nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
		if(nodeObjectContainer == NULL) break;								// Ran out of entries 

		jp.getValueByKey(nodeObjectContainer, "last", lastConnect);

		if ((Time.now() - lastConnect) > secondsPerPeriod) {				// If any of the nodes fail to connect - will extend loRA dwell time
			health = false;
			break;															// Don't need to keep checking
		}
	}

	Log.info("Node connections are %s ", (health) ? "healthy":"unhealthy");
	if(!health) LoRA_Functions::initializeRadio();
	return health;
}

int LoRA_Functions::stringCheckSum(String str){												// This function is made for the Particle DeviceID
    int result = 0;
    for(unsigned int i = 0; i < str.length(); i++){
      int asciiCode = (int)str[i];

      if (asciiCode >=48 && asciiCode <58) {              // 0-9
        result += asciiCode - 48;
      } 
      else if (asciiCode >=65 && asciiCode < 71) {        // A-F
        result += 10 + asciiCode -65;
      }
      else if (asciiCode >=97 && asciiCode < 103) {       // a - f
        result += 10 + asciiCode -97;
      }
    }
    return result;
}





