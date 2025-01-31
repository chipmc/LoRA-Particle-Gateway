#include "LoRA_Functions.h"
#include "JsonDataManager.h"
#include "PublishQueuePosixRK.h"

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
// max message length to prevent weird crashes
#ifndef RH_MAX_MESSAGE_LEN
#define RH_MAX_MESSAGE_LEN 255
#endif

// Mesh has much greater memory requirements, and you may need to limit the
// max message length to prevent wierd crashes
// #define RH_MESH_MAX_MESSAGE_LEN 50
uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];               // Related to max message size - RadioHead example note: dont put this on the stack:

bool LoRA_Functions::setup(bool gatewayID) {
    // Set up the Radio Module
	LoRA_Functions::instance().initializeRadio();

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
	JsonDataManager::instance().setup();

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
		if (!(current.get_nodeNumber() < 255 && LoRA_Functions::instance().checkForValidToken(current.get_nodeNumber(), current.get_token()))) {  // This not is a valid node number and token combo

			if (JsonDataManager::instance().checkIfNodeConfigured(current.get_nodeNumber(), current.get_uniqueID())) {	// This will return true if the node is configured
				Log.info("Node %d is configured but the token is invalid - resetting token", current.get_nodeNumber());
				current.set_token(setNodeToken(current.get_nodeNumber()));		// This is a valid node number but the token is not valid - reset the token
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
		if (sysStatus.get_updatedfrequencySeconds() > 0) {              				// If we are to change the update frequency, we need to tell the nodes (or at least one node) about it.
			sysStatus.set_frequencySeconds(sysStatus.get_updatedfrequencySeconds());	// This was the temporary value from the particle function
			sysStatus.set_updatedfrequencySeconds(0);
			Log.info("We are updating the publish frequency to %i seconds", sysStatus.get_frequencySeconds());
		}
		// The response will be specific to the message type
		if (lora_state == DATA_ACK) { if(LoRA_Functions::instance().acknowledgeDataReportGateway()) return true;}
		else if (lora_state == JOIN_ACK) { if(LoRA_Functions::instance().acknowledgeJoinRequestGateway()) return true;}
		else {Log.info("Invalid message flag"); return false;}
	}
	else LoRA_Functions::instance().clearBuffer();
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
	
	// Type differentiated JSON updates 
	switch (current.get_sensorType()) {
		case 1 ... 9: {    						// Counter
			// Update the node JSON for Counter sensorTypes here
		} break;
		case 10 ... 19: {   					// Occupancy
			if(JsonDataManager::instance().getAlertCode(current.get_nodeNumber()) == 0) {  // Don't update the node's occupancy counts if we have a queued alert, we should resolve that alert first. 
				// Update the occupancy counts in the nodeJson
				JsonDataManager::instance().setJsonData1(current.get_nodeNumber(), current.get_sensorType(), static_cast<int16_t>(current.get_payload3() <<8 | current.get_payload4()));
				JsonDataManager::instance().setJsonData2(current.get_nodeNumber(), current.get_sensorType(), static_cast<int16_t>(current.get_payload1() <<8 | current.get_payload2()));
			}
		} break;
		case 20 ... 29: {   					// Sensor
			// Update the node JSON for Sensor sensorTypes here
		} break;
		default: {          		
			Log.info("Unknown sensor type in decipherDataReportGateway %d",current.get_sensorType());
			if (Particle.connected()) PublishQueuePosix::instance().publish("Alert", "Unknown sensor type in decipherDataReportGateway", PRIVATE);
		} break;
	}

	JsonDataManager::instance().setLastReport(current.get_nodeNumber(), (int)Time.now()); // save the timestamp of this report in the JSON

	lora_state = DATA_ACK;		// Prepare to respond
	return true;
}

bool LoRA_Functions::acknowledgeDataReportGateway() { 		// This is a response to a data message 
	char messageString[128];

	if(current.get_onBreak()){ // if we are on break, respond based on sensor type
		switch (current.get_sensorType()) {
			case 1 ... 9: {    						// Counter
				// Handle a break for Counter sensorTypes here
			} break;
			case 10 ... 19: {   					// Occupancy
				if((current.get_payload3() <<8 | current.get_payload4()) != 0) { // if occupancyNet isn't zero
					JsonDataManager::instance().setOccupancyNetForNode(current.get_nodeNumber(), 0);
					Log.info("On break, responding with alert code 12 and alert context 0. (resetting net count for device to 0)");
				}
			} break;
			case 20 ... 29: {   					// Sensor
				// Handle a break for Sensor sensorTypes here
			} break;
			default: {          		
				Log.info("Unknown sensor type in acknowledgeDataReportGateway %d", current.get_sensorType());
				if (Particle.connected()) PublishQueuePosix::instance().publish("Alert", "Unknown sensor type in acknowledgeDataReportGateway", PRIVATE);
			} break;
		}
	}

	// buf[0] - buf[1] is magic number - processed above
	// buf[2] is nodeNumber - processed above
	buf[3] = highByte(current.get_token());			// Token - May have changed in the listening function above
	buf[4] = lowByte(current.get_token());			// Token - so I can trust you
	uint32_t currentTime = Time.now();
	buf[5] = (uint8_t)(currentTime >> 24); 			// Fourth byte - current time
	buf[6] = (uint8_t)(currentTime >> 16); 			// Third byte
	buf[7] = (uint8_t)(currentTime >> 8);  			// Second byte
	buf[8] = (uint8_t)(currentTime);  	

	// Here we calculate the seconds to the next report
	buf[9] = highByte(sysStatus.get_frequencySeconds());	// Frequency of reports set by the gateway
	buf[10] = lowByte(sysStatus.get_frequencySeconds());
	Log.info("Frequency of reports is %d seconds", sysStatus.get_frequencySeconds());	

	// Next we have to determine if there is an alert code to send
	// If the node is not configured, we will set an alert code of 1
	if (current.get_alertCodeNode() == 255 || current.get_alertCodeNode() == 1) {
		Log.info("Node %d is not configured so setting alert code to 1 - again!", current.get_nodeNumber());
		current.set_alertCodeNode(1);
	} else {
		// If the node is configured, we will check for an alert code in the nodeID database
		current.set_alertCodeNode(JsonDataManager::instance().getAlertCode(current.get_nodeNumber()));		// Get the alert code from the nodeID database if one is not already set
	}

	Log.info("In the data message ack composition, alert code for node %d is %d", current.get_nodeNumber(), current.get_alertCodeNode());
	buf[11] = current.get_alertCodeNode();	    // Send alert code to the node

	uint16_t alertContext = JsonDataManager::instance().getAlertContext(current.get_nodeNumber()); // Set the alert context if any
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
		if (Particle.connected()) PublishQueuePosix::instance().publish("status", messageString,PRIVATE);
		sysStatus.set_messageCount(sysStatus.get_messageCount() + 1); // Increment the message count
		JsonDataManager::instance().setAlertCode(current.get_nodeNumber(), 0); // Clear pending alert, as you just sent it  
		JsonDataManager::instance().setAlertContext(current.get_nodeNumber(), 0); // Clear pending alert context, as you just sent it  
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
		current.set_uniqueID(random1 << 24 | random2 << 16 | time1 << 8 | time2);			// This should be unique for the numbers we are talking
		current.set_sensorType(10);															// This is a virgin node - set the sensor type to 10
		Log.info("Node %d is a virgin node, assigning uniqueID of %lu", current.get_nodeNumber(), current.get_uniqueID());
	}

	current.set_nodeNumber(JsonDataManager::instance().findNodeNumber(current.get_nodeNumber(), current.get_uniqueID()));	// This will return the nodeNumber for the nodeID passed to the function

	if (!LoRA_Functions::instance().checkForValidToken(current.get_nodeNumber(), current.get_token())) {
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
	
	uint32_t currentTime = Time.now();
	buf[5] = (uint8_t)(currentTime >> 24); 							// Fourth byte - current time
	buf[6] = (uint8_t)(currentTime >> 16); 							// Third byte
	buf[7] = (uint8_t)(currentTime >> 8);  							// Second byte
	buf[8] = (uint8_t)(currentTime);         						// First byte	
	// Need to calculate the seconds to the next report
	buf[9] = highByte(sysStatus.get_frequencySeconds());			// Frequency of reports - for Gateways
	buf[10] = lowByte(sysStatus.get_frequencySeconds());	
	Log.info("Frequency of reports is %d seconds", sysStatus.get_frequencySeconds());	
	buf[11] = (current.get_nodeNumber() != 255) ?  0 : 1;			// Clear the alert code for the node unless the nodeNumber process failed
	buf[13] = current.get_sensorType();
	buf[14] = current.get_uniqueID() >> 24;							// Unique ID of the node
	buf[15] = current.get_uniqueID() >> 16;	
	buf[16] = current.get_uniqueID() >> 8;
	buf[17] = current.get_uniqueID();
	buf[18] = current.get_nodeNumber();

	if (JsonDataManager::instance().uniqueIDExistsInDatabase(current.get_uniqueID())) {		// Check to make sure the node's uniqueID is in the database
		uint8_t nodeNumber = JsonDataManager::instance().getNodeNumberForUniqueID(current.get_uniqueID());
		buf[13] = JsonDataManager::instance().getType(nodeNumber);							// Make sure type is up to date
		JsonDataManager::instance().getJoinPayload(nodeNumber);								// Get the payload values from the nodeID database
		buf[19] = current.get_payload1();					
		buf[20] = current.get_payload2();
		buf[21] = current.get_payload3();
		buf[22] = current.get_payload4();
		Log.info("Node %d join request will update sensorType to %d", current.get_tempNodeNumber(), (int)buf[13]);
		Log.info("Node %d join request will update with payload [%d, %d, %d, %d]", current.get_tempNodeNumber(), current.get_payload1(), current.get_payload2(), current.get_payload3(), current.get_payload4());
	}
	else {
		if (Particle.connected()) PublishQueuePosix::instance().publish("Alert", "findNodeNumber failed to add the node to the database.", PRIVATE);
	}			// Else, we will send an alert because the uniqueID should have been set in decipherJoinPayload when findNodeNumber was called

	buf[23] = 0;											// buf[23] and buf[24] are reserved for future use
	buf[24] = 0;

	byte nodeAddress = (current.get_tempNodeNumber() == 0) ? current.get_nodeNumber() : current.get_tempNodeNumber();  // get the return address right

	digitalWrite(BLUE_LED,HIGH);			        				// Sending data

	Log.info("Sending response to %d with free memory = %li", nodeAddress, System.freeMemory());

	if (manager.sendtoWait(buf, 25, nodeAddress, JOIN_ACK) == RH_ROUTER_ERROR_NONE) {
		current.set_tempNodeNumber(0);								// Temp no longer needed
		digitalWrite(BLUE_LED,LOW);
		snprintf(messageString,sizeof(messageString),"Node %d joined. New nodeNumber %d, sensorType %s, alert %d and RSSI / SNR of %d / %d", nodeAddress, current.get_nodeNumber(), (buf[10] ==0)? "car":"person",current.get_alertCodeNode(), current.get_RSSI(), current.get_SNR());
		Log.info(messageString);
		if (Particle.connected()) PublishQueuePosix::instance().publish("status", messageString,PRIVATE);
		return true;
	}
	else {
		Log.info("Node %d join response not acknowledged", current.get_tempNodeNumber()); // Acknowledgement not received - this needs more attention as node is in undefined state
		digitalWrite(BLUE_LED,LOW);
		return false;
	}
}


/**********************************************************************
 **                         Helper Functions                         **
 **********************************************************************/

uint16_t LoRA_Functions::setNodeToken(uint8_t nodeNumber) {
	if (nodeNumber == 0 || nodeNumber == 255) return 0;
	uint16_t token = sysStatus.get_tokenCore() * nodeNumber;	// This is the token for the node - it is a function of the core token and the day of the month
	Log.info("Setting token for node %d to %d", nodeNumber, token);
	return token;
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