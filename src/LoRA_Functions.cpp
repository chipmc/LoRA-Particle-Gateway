//Includes required for this file
#include "Particle.h"
#include "LoRA_Functions.h"
#include <RHMesh.h>
#include <RH_RF95.h>						        // https://docs.particle.io/reference/device-os/libraries/r/RH_RF95/
#include "device_pinout.h"
#include "storage_objects.h"


/*
payload[0] = 0;                                     // to be replaced/updated
payload[1] = 0;                                     // to be replaced/updated
payload[2] = highByte(devID);                       // Set for device
payload[3] = lowByte(devID);
payload[4] = firmVersion;                           // Set for code release
payload[5] = highByte(hourly);				       	// Hourly count
payload[6] = lowByte(hourly); 
payload[7] = highByte(daily);				        // Daily Count
payload[8] = lowByte(daily); 
payload[9] = temp;							        // Enclosure temp
payload[10] = battChg;						        // State of charge
payload[11] = battState; 					        // Battery State
payload[12] = resets						        // Reset count
Payload[13] = alerts						        // Current alerts
payload[14] = highByte(rssi);				        // Signal strength
payload[15] = lowByte(rssi); 
payload[16] = msgCnt++;						       	// Sequential message number
*/


//  ***************  LoRA Setup Section ********************
// In this implementation - we have one gateway and two nodes (will generalize later) - nodes are numbered 2 to ...
// 2- 9 for new nodes making a join request
// 10 - 255 nodes assigned to the mesh
const uint8_t GATEWAY_ADDRESS = 1;				        // The gateway for each mesh is #1
const uint8_t NODE2_ADDRESS = 10;					        // First Node
const uint8_t NODE3_ADDRESS = 11;					        // Second Node
const double RF95_FREQ = 915.0;							// Frequency - ISM

// Define the message flags
const uint8_t joinRequest = 1;
const uint8_t joinAcknowledge = 2;
const uint8_t dataReport = 3;
const uint8_t dataAcknowledge = 4;
const uint8_t alertReport = 5;
const uint8_t alertAcknowledge = 6;
typedef enum { NULL_STATE, JOIN_REQ, JOIN_ACQ, DATA_RPT, DATA_ACQ, ALERT_RPT, ALERT} LoRA_State;
char loraStateNames[7][16] = {"Null", "Join Req", "Join Ack", "Data Report", "Data Acq", "Alert Rpt", "Alert Ack"};
LoRA_State lora_state = NULL_STATE;

// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHMesh manager(driver, GATEWAY_ADDRESS);

// Mesh has much greater memory requirements, and you may need to limit the
// max message length to prevent wierd crashes
#ifndef RH_MAX_MESSAGE_LEN
#define RH_MAX_MESSAGE_LEN 255
#endif

// 
uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];               // Related to max message size - RadioHead example note: dont put this on the stack:
uint8_t len = sizeof(buf);
uint8_t from;
uint8_t messageFlag = 0;
//********************************************************


bool initializeLoRA() {
 	// Set up the Radio Module
	if (!manager.init()) {
		Log.info("init failed");	// Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
		return false;
	}

	driver.setFrequency(RF95_FREQ);					// Frequency is typically 868.0 or 915.0 in the Americas, or 433.0 in the EU - Are there more settings possible here?

	driver.setTxPower(23, false);                   // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then you can set transmitter powers from 5 to 23 dBm (13dBm default).  PA_BOOST?

	return true;
}



bool listenForLoRAMessage() {

	if (manager.recvfromAckTimeout(buf, &len, 3000, &from,__null,__null,&messageFlag))	{	// We have received a message
		buf[len] = 0;
		lora_state = (LoRA_State)(0x0F & messageFlag);				// Strip out the overhead byte
		Log.info("Received from node %d with rssi=%d - a %s message of length %d", from, driver.lastRssi(), loraStateNames[messageFlag] ,len);

		if (loRAStateMachine()) {
			// flashTheLEDs();
			return true;
		}
	}
	return false; 
}

bool sendLoRAMessage() {
	if (loRAStateMachine()) {
		return true;
	}
	return false;
}

bool loRAStateMachine() {
	switch (lora_state) {
	case NULL_STATE:
		Log.info("Error - no message flag received");
	break;

	case DATA_RPT:
		Log.info("Decipering data report"); 	// Gateway or a mesh node
		if (deciperDataReport()) {
			lora_state = DATA_ACQ;
			return true;
		}
	break;

	case DATA_ACQ:
		Log.info("Acknowledging Data Report"); // Typically a node or mesh node
		if (acknowledgeDataReport()) {
			lora_state = NULL_STATE;
			return true;
		}
	break;
	}
	return false;
}

bool acknowledgeDataReport() {
	Log.info("Sent response to client message = %d, time = %s, next report = %u minutes", buf[2], Time.timeStr(buf[3] << 24 | buf[4] << 16 | buf[5] <<8 | buf[6]).c_str(), (buf[7] << 8 | buf[8]));

	digitalWrite(BLUE_LED,HIGH);			        // Sending data

	if (manager.sendtoWait(buf, 9, from, dataAcknowledge) == RH_ROUTER_ERROR_NONE) {
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

bool deciperDataReport() {
		sysStatus.nodeNumber = buf[2] << 8 | buf[3]; // One time kluge until I implement the join process.

		if(sysStatus.nodeNumber == (buf[2] << 8 | buf[3])) {
			sysStatus.nodeNumber = buf[2] << 8 | buf[3];
			current.hourly = buf[5] << 8 | buf[6];
			current.daily = buf[7] << 8 | buf[8];
			current.stateOfCharge = buf[10];
			current.batteryState = buf[11];
			current.internalTempC = buf[9];
			sysStatus.resetCount = buf[12];
			sysStatus.lastAlertCode = buf[13];
			current.rssi = (buf[14] << 8 | buf[15]) - 65535;
			current.messageNumber = buf[16];
		}
		else Log.info("Message intended for another node");

		// This is a response to a data message it has a length of 9 and a specific payload and message flag
		// Send a reply back to the originator client
		buf[0] = 0;                                // to be replaced/updated
		buf[1] = 0;                                // to be replaced/updated       
		buf[2] = buf[16];							// Message number
		buf[3] = ((uint8_t) ((Time.now()) >> 24)); // Fourth byte - current time
		buf[4] = ((uint8_t) ((Time.now()) >> 16));	// Third byte
		buf[5] = ((uint8_t) ((Time.now()) >> 8));	// Second byte
		buf[6] = ((uint8_t) (Time.now()));		    // First byte			
		buf[7] = highByte(sysStatus.frequencyMinutes);	// Time till next report
		buf[8] = lowByte(sysStatus.frequencyMinutes);		

		return true;
}


void flashTheLEDs() {
	time_t lastChange = 0;
	int flashes = 0;

	while (flashes <= 6) {
		if (millis() - lastChange > 1000) {
			lastChange = millis();
			digitalWrite(BLUE_LED, !digitalRead(BLUE_LED));
			flashes++;
		}
	}
	digitalWrite(BLUE_LED, LOW);
}
