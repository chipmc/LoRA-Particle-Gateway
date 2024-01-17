#include "LoRA_Functions.h"
#include "Room_Occupancy.h"							// Aggregates node data to get net room occupancy for Occupancy Nodes

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
JsonParserStatic<1024, 550> jp;						// Make this global - reduce possibility of fragmentation
// So, here is how we arrived at this number:
// 1.  We need to store 50 nodes - each node is 6 bytes (nodeNumber, uniqueID, sensorType, payload, pendingAlerts) - 300 bytes
// 2.  We have one token for each object - 50 tokens and two tokens for each key value pair (50 * 5 * 2) - 500 tokens for a total of 550 tokens


// ************************************************************************
// *****                      LoRA Setup                              *****
// ************************************************************************
// In this implementation - we have one gateway with a fixed node  number of 0 and up to 10 nodes with node numbers 1-10 ...
// In an unconfigured node, there will be a node number of greater than 10 triggering a join request
// Configured nodes will be stored in a JSON object by the gateway with three fields: node number, Particle deviceID and Time stamp of last contact
//
const uint8_t GATEWAY_ADDRESS = 0;
// const double RF95_FREQ = 915.0;				 	// Frequency - ISM
const double RF95_FREQ = 92684;				// Center frequency for the omni-directional antenna I am using x100 as per Jeff's modification of the RFM95 Library

// Define the message flags
typedef enum { NULL_STATE, JOIN_REQ, JOIN_ACK, DATA_RPT, DATA_ACK, ALERT_RPT, ALERT_ACK} LoRA_State;
char loraStateNames[7][16] = {"Null", "Join Req", "Join Ack", "Data Report", "Data Ack", "Alert Rpt", "Alert Ack"};
static LoRA_State lora_state = NULL_STATE;

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);
Speck myCipher;                             // Class instance for Speck block ciphering     
RHEncryptedDriver driver(rf95, myCipher);   // Class instance for Encrypted RFM95 driver

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
	else if (sysStatus.get_nodeNumber() > 0 && sysStatus.get_nodeNumber() <= 255) {
		manager.setThisAddress(sysStatus.get_nodeNumber());	// Node - use the Node address in valid range from memory
		Log.info("LoRA Radio initialized as node %i and a deviceID of %s", manager.thisAddress(), System.deviceID().c_str());
	}
	else {																						// Else, we will set as an unconfigured node
		sysStatus.set_nodeNumber(255);
		manager.setThisAddress(255);
		Log.info("LoRA Radio initialized as an unconfigured node %i and a deviceID of %s", manager.thisAddress(), System.deviceID().c_str());
	}

	// Here is where we load the JSON object from memory and parse
	jp.addString(nodeDatabase.get_nodeIDJson());				// Read in the JSON string from memory
	Log.info("The node string is: %s",nodeDatabase.get_nodeIDJson().c_str());

	if (jp.parse()) Log.info("Parsed Successfully");
	else {
		nodeDatabase.resetNodeIDs();
		Log.info("Parsing error");
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

bool LoRA_Functions::initializeRadio() {  			// Set up the Radio Module
	digitalWrite(RFM95_RST,LOW);					// Reset the radio module before setup
	delay(10);
	digitalWrite(RFM95_RST,HIGH);
	delay(10);

	if (!manager.init()) {
		Log.info("init failed");					// Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
		return false;
	}
	rf95.setFrequency(RF95_FREQ);					// Frequency is typically 868.0 or 915.0 in the Americas, or 433.0 in the EU - Are there more settings possible here?
	rf95.setTxPower(23, false);                   // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then you can set transmitter powers from 5 to 23 dBm (13dBm default).  PA_BOOST?
	// driver.setModemConfig(RH_RF95::Bw125Cr45Sf2048);  // This is the setting appropriate for parks
	rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128);	 // Optimized for fast transmission and short range - MAFC
	//driver.setModemConfig(RH_RF95::Bw125Cr48Sf4096);	// This optimized the radio for long range - https://www.airspayce.com/mikem/arduino/RadioHead/classRH__RF95.html
	rf95.setLowDatarate();						// https://www.airspayce.com/mikem/arduino/RadioHead/classRH__RF95.html#a8e2df6a6d2cb192b13bd572a7005da67
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

		current.set_nodeNumber(from);												// Captures the nodeNumber
		uint16_t current_magicNumber = (buf[0] << 8 | buf[1]);								// Magic number

		// First we will validate that this node belongs in this network by checking the magic number
		if (current_magicNumber != sysStatus.get_magicNumber()) {
			Log.info("Node %d message magic number of %d did not match the Magic Number in memory %d - Ignoring", current.get_nodeNumber(), current_magicNumber, sysStatus.get_magicNumber());
			return false;
		}
		current.set_alertCodeNode(0);												// Clear the alert code for the node - Alert codes are set in the response
		current.set_tempNodeNumber(0);												// Clear for new response - this is used for join requests
		current.set_hops(hops);														// How many hops to get here
		current.set_token(buf[3] << 8 | buf[4]);									// The token sent by the note - need to check it is valid
		current.set_sensorType(buf[5]);												// Sensor type reported by the node
		current.set_uniqueID(buf[6] << 24 | buf[7] << 16 | buf[8] << 8 | buf[9]);	// Unique ID of the node - this is like the Particle deviceID

		lora_state = (LoRA_State)(0x0F & messageFlag);								// Strip out the overhead byte to get the message flag
		Log.info("Node %d with uniqueID %lu a %s message with RSSI/SNR of %d / %d in %d hops", current.get_nodeNumber(), current.get_uniqueID(), loraStateNames[lora_state], rf95.lastRssi(), rf95.lastSNR(), current.get_hops());

		// Next we need to test the nodeNumber / deviceID to make sure this node is properly configured
		if (!(current.get_nodeNumber() < 255 && LoRA_Functions::checkForValidToken(current.get_nodeNumber(), current.get_token()))) {  // This not is a valid node number and token combo

			if (LoRA_Functions::nodeConfigured(current.get_nodeNumber(), current.get_uniqueID())) {	// This will return true if the node is configured
				Log.info("Node %d is configured but the token is invalid - resetting token", current.get_nodeNumber());
				current.set_token(setNodeToken(current.get_nodeNumber()));	// This is a valid node number but the token is not valid - reset the token
			}
			else {																// This is an unconfigured node - need to assign it a node number
				current.set_tempNodeNumber(current.get_nodeNumber());			// Store node number in temp for the repsonse
				current.set_nodeNumber(255);									// Set node number to unconfigured
				current.set_alertCodeNode(255);									// This will ensure we get a join request to the node
				Log.info("Node %d is unconfigured so setting alertCode to %d", current.get_nodeNumber(), current.get_alertCodeNode());
			}
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
	// buf[0] - buf[1] Magic number processed above
	// buf[2] - nodeNumber processed above
	// buf[3] - buf[4] is token - processed above
	// buf[5] - Sensor type - processed above
	// buf[6] - buf[9] is the unique ID of the node - processed above
	current.set_payload1(buf[10]);
	current.set_payload2(buf[11]);
	current.set_payload3(buf[12]);
	current.set_payload4(buf[13]);
	current.set_payload5(buf[14]);
	current.set_payload6(buf[15]);
	current.set_payload7(buf[16]);
	current.set_payload8(buf[17]);
	// Then, we will get the rest of the data from the payload
	current.set_internalTempC(buf[18]);
	current.set_stateOfCharge(buf[19]);
	current.set_batteryState(buf[20]);
	current.set_resetCount(buf[21]);
	current.set_RSSI(buf[22] << 8 | buf[23]);		// These values are from the node based on the last successful data report
	current.set_SNR(buf[24] << 8 | buf[25]);
	current.set_retryCount(buf[26]);
	current.set_retransmissionDelay(buf[27]);

	// Log.info("Data recieved from the report: sensorType %d, temp %d, battery %d, batteryState %d, resets %d, message count %d, RSSI %d, SNR %d", current.get_sensorType(), current.get_internalTempC(), current.get_stateOfCharge(), current.get_batteryState(), current.get_resetCount(), sysStatus.get_messageCount(), current.get_RSSI(), current.get_SNR());

	lora_state = DATA_ACK;		// Prepare to respond
	return true;
}

bool LoRA_Functions::acknowledgeDataReportGateway() { 		// This is a response to a data message 
	char messageString[128];

	// buf[0] - buf[1] is magic number - processed above
	// buf[2] is nodeNumber - processed above
	buf[3] = highByte(current.get_token());			// Token - May have changed in the listening function above
	buf[4] = lowByte(current.get_token());			// Token - so I can trust you
	buf[5] = ((uint8_t) ((Time.now()) >> 24)); 		// Fourth byte - current time
	buf[6] = ((uint8_t) ((Time.now()) >> 16));		// Third byte
	buf[7] = ((uint8_t) ((Time.now()) >> 8));		// Second byte
	buf[8] = ((uint8_t) (Time.now()));		    	// First byte	

	// Here we calculate the seconds to the next report
	buf[9] = highByte(sysStatus.get_frequencyMinutes());	// Frequency of reports set by the gateway
	buf[10] = lowByte(sysStatus.get_frequencyMinutes());	

	// Next we have to determine if there is an alert code to send
	// If the node is not configured, we will set an alert code of 1
	if (current.get_alertCodeNode() == 255) {
		Log.info("Node %d is not configured so setting alert code to 1 - again!", current.get_nodeNumber());
		current.set_alertCodeNode(1);
	}
	// If the node is configured, we will check for an alert code in the nodeID database
	else current.set_alertCodeNode(LoRA_Functions::getAlertCode(current.get_nodeNumber()));		// Get the alert code from the nodeID database if one is not already set

	Log.info("In the data message ack composition, alert code for node %d is %d", current.get_nodeNumber(), current.get_alertCodeNode());
	buf[11] = current.get_alertCodeNode();	    // Send alert code to the node

	uint16_t alertContext = LoRA_Functions::instance().getAlertContext(current.get_nodeNumber()); // Set the alert context if any
	buf[12] = highByte(alertContext);
	buf[13] = lowByte(alertContext);

	buf[14] = current.get_sensorType();			// Set the sensor type - this is the sensor type reported by the node
	buf[15] = 0;
	buf[16] = 0;								// Will be over-written if needed

	current.flush(true);							// Save values reported by the nodes
	digitalWrite(BLUE_LED,HIGH);			       	// Sending data

	byte nodeAddress = (current.get_tempNodeNumber() == 0) ? current.get_nodeNumber() : current.get_tempNodeNumber();  // get the return address right

	if (manager.sendtoWait(buf, 16, nodeAddress, DATA_ACK) == RH_ROUTER_ERROR_NONE) {
		digitalWrite(BLUE_LED,LOW);

		snprintf(messageString,sizeof(messageString),"Node %d data report %d acknowledged with alert %d, and RSSI / SNR of %d / %d", current.get_nodeNumber(), sysStatus.get_messageCount(), current.get_alertCodeNode(), current.get_RSSI(), current.get_SNR());
		Log.info(messageString);
		if (Particle.connected()) Particle.publish("status", messageString,PRIVATE);
		sysStatus.set_messageCount(sysStatus.get_messageCount() + 1); // Increment the message count
		LoRA_Functions::instance().setAlertCode(current.get_nodeNumber(), 0); // Clear pending alert, as you just sent it  
		LoRA_Functions::instance().setAlertContext(current.get_nodeNumber(), 0); // Clear pending alert context, as you just sent it  

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
	// buf[2] - nodeNumber processed above
	// buf[3] - buf[4] is token - processed above
	// buf[5] - Sensor type - processed above
	// buf[6] - buf[9] is the unique ID of the node - processed above
	current.set_payload1(buf[10]);					
	current.set_payload2(buf[11]);
	current.set_payload3(buf[12]);
	current.set_payload4(buf[13]);
	current.set_retryCount(buf[14]);
	current.set_retransmissionDelay(buf[15]);

	if (buf[6] == 255 && buf[7] == 255) {			// assign a uniqueID					// This is a virgin node - need to assign it a uniqueID
		uint8_t random1 = random(0,254);													// Not to 255 so we can see if it is a virgin node
		uint8_t random2 = random(0,254);
		uint8_t time1 = ((uint8_t) ((Time.now()) >> 8));									// Second byte
		uint8_t time2 = ((uint8_t) (Time.now()));											// First byte
		current.set_uniqueID(random1 << 24 | random2 << 16 | time1 << 8 | time2);	// This should be unique for the numbers we are talking
		current.set_sensorType(10);												// This is a virgin node - set the sensor type to 10
		Log.info("Node %d is a virgin node, assigning uniqueID of %lu", current.get_nodeNumber(), current.get_uniqueID());
	}

	current.set_nodeNumber(LoRA_Functions::findNodeNumber(current.get_nodeNumber(), current.get_uniqueID()));	// This will return the nodeNumber for the nodeID passed to the function

	if (!LoRA_Functions::checkForValidToken(current.get_nodeNumber(), current.get_token())) {
		current.set_token(sysStatus.get_tokenCore() * Time.day() + current.get_nodeNumber());	// This is the token for the node - it is a function of the core token and the day of the month
	}

	Log.info("Node %d join request will change node number to %d with a token of %d", current.get_tempNodeNumber(), current.get_nodeNumber(), current.get_token());

	current.set_alertCodeNode(1);									// This is a join request so alert code is 1

	lora_state = JOIN_ACK;			// Prepare to respond
	return true;
}

bool LoRA_Functions::acknowledgeJoinRequestGateway() {
	char messageString[128];
	Log.info("Acknowledge Join Request");
	// Gateway's response to a join request from a node
	buf[0] = highByte(sysStatus.get_magicNumber());					// Magic number - so you can trust me
	buf[1] = lowByte(sysStatus.get_magicNumber());					// Magic number - so you can trust me
	buf[2] = current.get_nodeNumber();
	buf[3] = highByte(current.get_token());							// Token - so I can trust you
	buf[4] = lowByte(current.get_token());							// Token - so I can trust you
	buf[5] = ((uint8_t) ((Time.now()) >> 24));  					// Fourth byte - current time
	buf[6] = ((uint8_t) ((Time.now()) >> 16));						// Third byte
	buf[7] = ((uint8_t) ((Time.now()) >> 8));						// Second byte
	buf[8] = ((uint8_t) (Time.now()));		    					// First byte		
	// Need to calculate the seconds to the next report
	buf[9] = highByte(sysStatus.get_frequencyMinutes());			// Frequency of reports - for Gateways
	buf[10] = lowByte(sysStatus.get_frequencyMinutes());	
	buf[11] = (current.get_nodeNumber() != 255) ?  0 : 1;			// Clear the alert code for the node unless the nodeNumber process failed
	buf[13] = current.get_sensorType();
	buf[14] = current.get_uniqueID() >> 24;							// Unique ID of the node
	buf[15] = current.get_uniqueID() >> 16;	
	buf[16] = current.get_uniqueID() >> 8;
	buf[17] = current.get_uniqueID();
	buf[18] = current.get_nodeNumber();

	if (LoRA_Functions::uniqueIDExistsInDatabase(current.get_uniqueID())) {		// Check to make sure the node's uniqueID is in the database
		uint8_t nodeNumber = LoRA_Functions::instance().getNodeNumberForUniqueID(current.get_uniqueID());
		buf[13] = LoRA_Functions::getType(nodeNumber);							// Make sure type is up to date
		LoRA_Functions::getJoinPayload(nodeNumber);										// Get the payload values from the nodeID database
		buf[19] = current.get_payload1();					
		buf[20] = current.get_payload2();
		buf[21] = current.get_payload3();
		buf[22] = current.get_payload4();
		Log.info("Node %d join request will update sensorType to %d", current.get_tempNodeNumber(), (int)buf[13]);
		Log.info("Node %d join request will update with payload [%d, %d, %d, %d]", current.get_tempNodeNumber(), current.get_payload1(), current.get_payload2(), current.get_payload3(), current.get_payload4());
	}
	else buf[19] = buf[20] = buf[21] = buf[22] = 0;			// Else, we will send 0's for the payload

	buf[23] = 0;											// buf[23] and buf[24] are reserved for future use
	buf[24] = 0;

	byte nodeAddress = (current.get_tempNodeNumber() == 0) ? current.get_nodeNumber() : current.get_tempNodeNumber();  // get the return address right
					
	digitalWrite(BLUE_LED,HIGH);			        				// Sending data

	Log.info("Sending response to %d with free memory = %li", nodeAddress, System.freeMemory());

	if (manager.sendtoWait(buf, 25, nodeAddress, JOIN_ACK) == RH_ROUTER_ERROR_NONE) {
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
		{uID: (uint32_t)uniqueID},
		{type: (int)sensorType},
		{p: (int)compressedPayload},
		{pend: (int)pendingAlerts},
		{cont: (int)pendingAlertContext}
	},
	....]
}

Let's try something new here:

*/

// These functions access data in the nodeID JSON
uint8_t LoRA_Functions::findNodeNumber(int nodeNumber, uint32_t uniqueID) {
	int index=1;															// Variables to hold values for the function
	int nodeDeviceNumber;
	uint32_t nodeDeviceID;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i=0; i<100; i++) {												// Iterate through the array looking for a match
		nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
		if(nodeObjectContainer == NULL) {
			Log.info("findNodeNumber ran out of entries at i = %d",i);
			break;															// Ran out of entries - no match found
		} 
		jp.getValueByKey(nodeObjectContainer, "uID", nodeDeviceID);			// Get the deviceID and compare
		if (nodeDeviceID == uniqueID) {
			jp.getValueByKey(nodeObjectContainer, "node", nodeDeviceNumber);		// A match!
			return nodeDeviceNumber;												// All is good - return node number for the deviceID passed to the function
		}
		index++;															// This will be the node number for the next node if no match is found
	}
	// If we got to here, the nodeID was not a match for any entry and a new nodeNumer will be assigned
	nodeNumber = index;
	JsonModifier mod(jp);

	Log.info("New node will be assigned node number %d, nodeID of %lu ",nodeNumber, uniqueID);

	mod.startAppend(jp.getOuterArray());
		mod.startObject();
		mod.insertKeyValue("node", nodeNumber);
		mod.insertKeyValue("uID", uniqueID);
		mod.insertKeyValue("type", (int)current.get_sensorType());			// This is the sensor type reported by the node		
		mod.insertKeyValue("p",(int)0);
		mod.insertKeyValue("pend",(int)0);
		mod.insertKeyValue("cont",(int)0);
		mod.finishObjectOrArray();
	mod.finish();

	nodeDatabase.set_nodeIDJson(jp.getBuffer());									// This should backup the nodeID database - now updated to persistent storage

	return index;
}


bool LoRA_Functions::nodeConfigured(int nodeNumber, uint32_t uniqueID)  {	// node is 'configured' if a uniqueID for it exists in the payload is set
	if (nodeNumber == 0 || nodeNumber == 255) return false;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) return false;							// Ran out of entries - no match found
	
	jp.getValueByKey(nodeObjectContainer, "uID", uniqueID);					// Get the uniqueID for the node number in question

	if (uniqueID == current.get_uniqueID()) return true;
	else {
		Log.info("Node number is found but uniqueID is not a match - unconfigured");  // See the raw JSON string
		return false;
	}
}

bool LoRA_Functions::uniqueIDExistsInDatabase(uint32_t uniqueID)  {			// node is 'configured' if a uniqueID for it exists in the payload is set
	uint32_t nodeDeviceID;
	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i=0; i < 100; i++) {												// Iterate through the array looking for a match
		nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
		if(nodeObjectContainer == NULL) {
			Log.info("uniqueIDExistsInDatabase ran out of entries at i = %d",i);
			break;															// Ran out of entries - no match found
		} 
		jp.getValueByKey(nodeObjectContainer, "uID", nodeDeviceID);			// Get the deviceID and compare
		if (nodeDeviceID == uniqueID) {										// A match!		
			Log.info("uniqueIDExistsInDatabase returned true");
			return true;													// All is good - return node number for the deviceID passed to the function
		}
	}
	return false;
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

bool LoRA_Functions::setType(int nodeNumber, int newType) {
	if (nodeNumber == 0 || nodeNumber == 255) return false;
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

byte LoRA_Functions::getNodeNumberForUniqueID(uint32_t uniqueID) {
	int nodeNumber;
	int index = 1;
	uint32_t nodeDeviceID;
	Log.info("searching array for node with unique id = %lu", uniqueID);
	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i=0; i<100; i++) {												// Iterate through the array looking for a match
		nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
		if(nodeObjectContainer == NULL) {
			Log.info("getNodeNumberForUniqueID ran out of entries at i = %d",i);
			break;															// Ran out of entries - no match found
		} 
		jp.getValueByKey(nodeObjectContainer, "uID", nodeDeviceID);			// Get the deviceID and compare
		Log.info("comparing id %lu to uniqueID %lu", nodeDeviceID, uniqueID);
		if (nodeDeviceID == uniqueID) {
			jp.getValueByKey(nodeObjectContainer, "node", nodeNumber);		// A match!
			return nodeNumber;												// All is good - return node number for the deviceID passed to the function
		}
		index++;															// This will be the node number for the next node if no match is found
	}
	return 0;
}

bool LoRA_Functions::getJoinPayload(uint8_t nodeNumber) {
	bool result;
	if (nodeNumber == 0 || nodeNumber == 255) return false;
	uint8_t sensorType = LoRA_Functions::getType(nodeNumber);
	int compressedPayloadInt;
	uint8_t compressedPayload;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) {
		Log.info("From getJoinPayload function Node number not found so returning false");
		return false;
	} 
	// Else we will load the current values from the node
	jp.getValueByKey(nodeObjectContainer, "p", compressedPayloadInt);
	compressedPayload = static_cast<uint8_t>(compressedPayloadInt);

	result = LoRA_Functions::hydrateJoinPayload(sensorType, compressedPayload);

	if(result) Log.info("Loaded payload values of %d, %d, %d, %d", current.get_payload1(), current.get_payload2(), current.get_payload3(), current.get_payload4());
	return result;
}

bool LoRA_Functions::setJoinPayload(uint8_t nodeNumber) {
	bool result;
	if (nodeNumber == 0 || nodeNumber == 255) return false;
	uint8_t sensorType = LoRA_Functions::getType(nodeNumber);
	uint8_t  payload1;
	uint8_t  payload2;
	uint8_t  payload3;
	uint8_t  payload4;
	int compressedPayloadInt;
	uint8_t compressedPayload;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) return false;							// Ran out of entries

	jp.getValueByKey(nodeObjectContainer, "p", compressedPayloadInt);
	compressedPayload = static_cast<uint8_t>(compressedPayloadInt);			// set compressedPayload as the compressed JSON payload variables

	result = LoRA_Functions::parseJoinPayloadValues(sensorType, compressedPayload, payload1, payload2, payload3, payload4);
	if (!result) 
	Log.info("Changing payload values from %d, %d, %d, %d", payload1, payload2, payload3, payload4);

	compressedPayload = LoRA_Functions::getCompressedJoinPayload(sensorType);	// set compressedPayload as the compressed currentData payload variables
	result = LoRA_Functions::parseJoinPayloadValues(sensorType, compressedPayload, payload1, payload2, payload3, payload4);
	Log.info("Changed payload values to %d, %d, %d, %d", payload1, payload2, payload3, payload4);

	const JsonParserGeneratorRK::jsmntok_t *value;

	jp.getValueTokenByKey(nodeObjectContainer, "p", value);
	JsonModifier mod(jp);
	mod.startModify(value);
	mod.insertValue((int)compressedPayload);
	mod.finish();

	nodeDatabase.set_nodeIDJson(jp.getBuffer());							// This should backup the nodeID database - now updated to persistent storage

	return result;
}

byte LoRA_Functions::getAlertCode(int nodeNumber) {
	if (nodeNumber == 0 || nodeNumber == 255) return 255;					// Not a configured node

	// This function returns the pending alert code for the node - if there is one
	// If there is not one, it will check to see if the park is closed and return 6 if it is

	int pendingAlert;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) {
		Log.info("From getAlertCode function, Node number not found");
		return 255;															// Ran out of entries 
	} 

	jp.getValueByKey(nodeObjectContainer, "pend", pendingAlert);

	if (pendingAlert == 0) pendingAlert = (current.get_openHours() == 1) ?  0 : 6;	// Set an alert code if the node is not in open hours - this will reset the current data

	return pendingAlert;
}

uint16_t LoRA_Functions::getAlertContext(int nodeNumber) {
	if (nodeNumber == 0 || nodeNumber == 255) return 255;					// Not a configured node

	// This function returns the pending alert context for the node - if there is one
	int pendingAlertContext;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);
	if(nodeObjectContainer == NULL) {
		Log.info("From getAlertContext function, Node number not found");
		return 255;															// Ran out of entries 
	} 

	jp.getValueByKey(nodeObjectContainer, "cont", pendingAlertContext);

	return pendingAlertContext;
}

bool LoRA_Functions::setAlertCode(int nodeNumber, int newAlert) {
	if (nodeNumber == 0 || nodeNumber == 255) return false;
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

	nodeDatabase.set_nodeIDJson(jp.getBuffer());							// This updates the JSON object but doe not commit to to persistent storage

	return true;
}

bool LoRA_Functions::setAlertContext(int nodeNumber, int newAlertContext) {
	if (nodeNumber == 0 || nodeNumber == 255) return false;
	int currentAlertContext;

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array

	nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, nodeNumber-1);	// find the entry for the node of interest
	if(nodeObjectContainer == NULL) return false;							// Ran out of entries - node number entry not found triggers alert

	jp.getValueByKey(nodeObjectContainer, "cont", currentAlertContext);			// Now we have the oject for the specific node
	Log.info("Changing pending alert context from %d to %d", currentAlertContext, newAlertContext);

	const JsonParserGeneratorRK::jsmntok_t *value;							// Node we have the key value pair for the "pend"ing alerts	
	jp.getValueTokenByKey(nodeObjectContainer, "cont", value);

	JsonModifier mod(jp);													// Create a modifier object
	mod.startModify(value);													// Update the pending alert value for the selected node
	mod.insertValue((int)newAlertContext);								
	mod.finish();

	nodeDatabase.set_nodeIDJson(jp.getBuffer());							// This updates the JSON object but doe not commit to to persistent storage

	return true;
}

void LoRA_Functions::printNodeData(bool publish) {
	int nodeNumber;
	uint32_t uniqueID;
	int sensorType;
	uint8_t  payload1;
	uint8_t  payload2;
	uint8_t  payload3;
	uint8_t  payload4;
	int compressedPayload;
	int pendingAlertCode;
	int pendingAlertContext;
	char data[256];

	const JsonParserGeneratorRK::jsmntok_t *nodesArrayContainer;			// Token for the outer array
	jp.getValueTokenByKey(jp.getOuterObject(), "nodes", nodesArrayContainer);
	const JsonParserGeneratorRK::jsmntok_t *nodeObjectContainer;			// Token for the objects in the array (I beleive)

	for (int i=0; i<100; i++) {												// Iterate through the array looking for a match
		nodeObjectContainer = jp.getTokenByIndex(nodesArrayContainer, i);
		if(nodeObjectContainer == NULL) {
			break;								// Ran out of entries 
		} 
		jp.getValueByKey(nodeObjectContainer, "uID", uniqueID);
		jp.getValueByKey(nodeObjectContainer, "node", nodeNumber);
		jp.getValueByKey(nodeObjectContainer, "type", sensorType);
		jp.getValueByKey(nodeObjectContainer, "p", compressedPayload);
		jp.getValueByKey(nodeObjectContainer, "pend", pendingAlertCode);
		jp.getValueByKey(nodeObjectContainer, "cont", pendingAlertContext);
		
		LoRA_Functions::parseJoinPayloadValues(sensorType, compressedPayload, payload1, payload2, payload3, payload4);

		snprintf(data, sizeof(data), "Node %d, uniqueID %lu, type %d payload (%d/%d/%d/%d) with pending alert %d and alert context %d", nodeNumber, uniqueID, sensorType, payload1, payload2, payload3, payload4, pendingAlertCode, pendingAlertContext);
		Log.info(data);
		if (Particle.connected() && publish) {
			Particle.publish("nodeData", data, PRIVATE);
			delay(1000);
		}
	}
	Log.info(nodeDatabase.get_nodeIDJson());  // See the raw JSON string
}

bool LoRA_Functions::checkForValidToken(uint8_t nodeNumber, uint16_t token) {
	if (nodeNumber == 0 || nodeNumber == 255) return false;
	Log.info("Checking for a valid token - nodeNumber %d, token %d, tokenCore %d", nodeNumber, token, sysStatus.get_tokenCore());

	if (token / sysStatus.get_tokenCore() == nodeNumber) {
		Log.info("Token is valid");
		return true;
	}
	else return false;
}

uint16_t LoRA_Functions::setNodeToken(uint8_t nodeNumber) {
	if (nodeNumber == 0 || nodeNumber == 255) return 0;
	uint16_t token = sysStatus.get_tokenCore() * nodeNumber;	// This is the token for the node - it is a function of the core token and the day of the month
	Log.info("Setting token for node %d to %d", nodeNumber, token);
	return token;
}

uint8_t LoRA_Functions::getCompressedJoinPayload(uint8_t sensorType) {
    uint8_t data[4] = {0};
    uint8_t bitSizes[4] = {0};

    switch (sensorType) {
        case 1 ... 9: {    						// Counter
            bitSizes[0] = 1; // 2-Way (1 bit)
        } break;
        case 10 ... 19: {   					// Occupancy
            bitSizes[0] = 6; // space (6 bits)
            bitSizes[1] = 1; // placement (1 bit)
            bitSizes[2] = 1; // multi (1 bit)
        } break;
        case 20 ... 29: {   					// Sensor
            bitSizes[0] = 6; // space (6 bits)
            bitSizes[1] = 1; // placement (1 bit)
        } break;
        default: {          		
            Log.info("Unknown sensor type in getCompressedJoinPayload %d", sensorType);
            if (Particle.connected()) Particle.publish("Alert", "Unknown sensor type in getCompressedJoinPayload", PRIVATE);
            return 0;
        } break;
    }

	data[0] = current.get_payload1();
	data[1] = current.get_payload2();
	data[2] = current.get_payload3();
	data[3] = current.get_payload4();

    return compressData(data, bitSizes);
}

bool LoRA_Functions::hydrateJoinPayload(uint8_t sensorType, uint8_t compressedPayload) {
    uint8_t data[4] = {0};
    uint8_t bitSizes[4] = {0};

    switch (sensorType) {
        case 1 ... 9: {    						// Counter
            bitSizes[0] = 1; // 2-Way (1 bit)
        } break;
        case 10 ... 19: {   					// Occupancy
            bitSizes[0] = 6; // space (6 bits)
            bitSizes[1] = 1; // placement (1 bit)
            bitSizes[2] = 1; // multi (1 bit)
        } break;
        case 20 ... 29: {   					// Sensor
            bitSizes[0] = 6; // space (6 bits)
            bitSizes[1] = 1; // placement (1 bit)
        } break;
        default: {
            Log.info("Unknown sensor type in hydrateJoinPayload %d", sensorType);
            if (Particle.connected()) Particle.publish("Alert", "Unknown sensor type in hydrateJoinPayload", PRIVATE);
            return false;
        } break;
    }

    decompressData(compressedPayload, data, bitSizes);

    current.set_payload1(data[0]);
    current.set_payload2(data[1]);
    current.set_payload3(data[2]);
    current.set_payload4(data[3]);

    return true;
}

bool LoRA_Functions::parseJoinPayloadValues(uint8_t sensorType, uint8_t compressedPayload, uint8_t& payload1, uint8_t& payload2, uint8_t& payload3, uint8_t& payload4) {
    uint8_t data[4] = {0};
    uint8_t bitSizes[4] = {0};

    switch (sensorType) {
        case 1 ... 9: {    						// Counter
            bitSizes[0] = 1; // 2-Way (1 bit)
        } break;
        case 10 ... 19: {   					// Occupancy
            bitSizes[0] = 6; // space (6 bits)
            bitSizes[1] = 1; // placement (1 bit)
            bitSizes[2] = 1; // multi (1 bit)
        } break;
        case 20 ... 29: {   					// Sensor
            bitSizes[0] = 6; // space (6 bits)
            bitSizes[1] = 1; // placement (1 bit)
        } break;
        default: {
            Log.info("Unknown sensor type in parseJoinPayloadValues %d", sensorType);
            if (Particle.connected()) Particle.publish("Alert", "Unknown sensor type in parseJoinPayloadValues", PRIVATE);
            return false;
        } break;
    }

    decompressData(compressedPayload, data, bitSizes);

    payload1 = data[0];
    payload2 = data[1];
    payload3 = data[2];
    payload4 = data[3];

    return true;
}

uint8_t LoRA_Functions::compressData(uint8_t data[], uint8_t bitSizes[]) {
    uint8_t compressedData = 0;
    uint8_t bitOffset = 0;
    for (uint8_t i = 0; i < 4; ++i) {
        data[i] = (data[i] < (1 << bitSizes[i])) ? data[i] : ((1 << bitSizes[i]) - 1);
        compressedData |= (data[i] * (1 << bitOffset));
        bitOffset += bitSizes[i];
    }
    return compressedData;
}

void LoRA_Functions::decompressData(uint8_t compressedData, uint8_t data[], uint8_t bitSizes[]) {
    uint8_t bitOffset = 0;
    for (uint8_t i = 0; i < 4; ++i) {
        data[i] = (compressedData >> bitOffset) & ((1 << bitSizes[i]) - 1);
        bitOffset += bitSizes[i];
    }
}