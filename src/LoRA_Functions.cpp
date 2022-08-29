//Includes required for this file
#include "Particle.h"
#include "LoRA_Functions.h"
#include <RHMesh.h>
#include <RH_RF95.h>						        // https://docs.particle.io/reference/device-os/libraries/r/RH_RF95/
#include "device_pinout.h"
#include "storage_objects.h"

// Format of a data report
/*
buf[0] = 0;                                     // to be replaced/updated
buf[1] = 0;                                     // to be replaced/updated
buf[2] = highByte(devID);                       // Set for device
buf[3] = lowByte(devID);
buf[4] = firmVersion;                           // Set for code release
buf[5] = highByte(hourly);				       	// Hourly count
buf[6] = lowByte(hourly); 
buf[7] = highByte(daily);				        // Daily Count
buf[8] = lowByte(daily); 
buf[9] = temp;							        // Enclosure temp
buf[10] = battChg;						        // State of charge
buf[11] = battState; 					        // Battery State
buf[12] = resets						        // Reset count
buf[13] = 1						        		// Reserve for later
buf[14] = highByte(rssi);				        // Signal strength
buf[15] = lowByte(rssi); 
buf[16] = msgCnt++;						       	// Sequential message number
*/
// Format of a data acknowledgement
/*    
buf[0] = buf[16];							// Message number
buf[1] = ((uint8_t) ((Time.now()) >> 24)); // Fourth byte - current time
buf[2] = ((uint8_t) ((Time.now()) >> 16));	// Third byte
buf[3] = ((uint8_t) ((Time.now()) >> 8));	// Second byte
buf[4] = ((uint8_t) (Time.now()));		    // First byte	
buf[5] = highByte(sysStat.freqencyMinutes);	// For the Gateway minutes on the hour
buf[6] = lowByte(sysStatus.frequencyMinutes)'		
buf[5] = highByte(sysStatus.nextReportSeconds);	// Seconds until next report - 16-bit number can go over 2 days
buf[6] = lowByte(sysStatus.nextReportSeconds);	
*/

// ************************************************************************
// *****                      LoRA Setup                              *****
// ************************************************************************
// In this implementation - we have one gateway and two nodes (will generalize later) - nodes are numbered 2 to ...
// 2- 9 for new nodes making a join request
// 10 - 255 nodes assigned to the mesh
const uint8_t GATEWAY_ADDRESS = 0;				        // The gateway for each mesh is always zero
const uint8_t NODE1_ADDRESS = 10;					    // First Node
const uint8_t NODE2_ADDRESS = 11;					    // Second Node
const double RF95_FREQ = 915.0;							// Frequency - ISM

// Define the message flags
typedef enum { NULL_STATE, JOIN_REQ, JOIN_ACK, DATA_RPT, DATA_ACK, ALERT_RPT, ALERT_ACK} LoRA_State;
char loraStateNames[7][16] = {"Null", "Join Req", "Join Ack", "Data Report", "Data Ack", "Alert Rpt", "Alert Ack"};
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

// ************************************************************************
// *****                      Common Functions                        *****
// ************************************************************************

/**
 * @brief Initialize the LoRA radio - this is a common step for nodes and gateways
 * 
 * @return true - initialization successful
 * @return false - initialization failed
 */
bool initializeLoRA() {
 	// Set up the Radio Module
	if (!manager.init()) {
		Log.info("init failed");	// Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
		return false;
	}
	driver.setFrequency(RF95_FREQ);					// Frequency is typically 868.0 or 915.0 in the Americas, or 433.0 in the EU - Are there more settings possible here?
	driver.setTxPower(23, false);                   // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then you can set transmitter powers from 5 to 23 dBm (13dBm default).  PA_BOOST?
	Log.info("LoRA Radio initialized as NodeNumber of %i and DeviceID of %i", sysStatus.nodeNumber, sysStatus.deviceID);
	return true;
}

// ************************************************************************
// *****                      Gateway Functions                       *****
// ************************************************************************

bool listenForLoRAMessageGateway() {

	if (manager.recvfromAckTimeout(buf, &len, 3000, &from,__null,__null,&messageFlag))	{	// We have received a message
		buf[len] = 0;
		lora_state = (LoRA_State)(0x0F & messageFlag);				// Strip out the overhead byte
		Log.info("Received from node %d with rssi=%d - a %s message of length %d", from, driver.lastRssi(), loraStateNames[messageFlag] ,len);

		if (lora_state == DATA_RPT) { if(deciperDataReportGateway()) return true;}

	}
	return false; 
}

bool deciperDataReportGateway() {
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

	return true;
}

bool acknowledgeDataReportGateway(int nextSeconds) {

	sysStatus.nextReportSeconds = nextSeconds;

	// This is a response to a data message it has a length of 9 and a specific payload and message flag
	// Send a reply back to the originator client
     
	buf[0] = buf[16];							// Message number
	buf[1] = ((uint8_t) ((Time.now()) >> 24)); // Fourth byte - current time
	buf[2] = ((uint8_t) ((Time.now()) >> 16));	// Third byte
	buf[3] = ((uint8_t) ((Time.now()) >> 8));	// Second byte
	buf[4] = ((uint8_t) (Time.now()));		    // First byte			
	buf[5] = highByte(sysStatus.frequencyMinutes);	// Frequency of reports - for Gateways
	buf[6] = lowByte(sysStatus.frequencyMinutes);	
	buf[7] = highByte(sysStatus.nextReportSeconds);	// Seconds until next report - for Nodes
	buf[8] = lowByte(sysStatus.nextReportSeconds);
	
	Log.info("Sent response to client message = %d, time = %s, next report = %u seconds", buf[0], Time.timeStr(buf[1] << 24 | buf[2] << 16 | buf[3] <<8 | buf[4]).c_str(), (buf[7] << 8 | buf[8]));

	digitalWrite(BLUE_LED,HIGH);			        // Sending data

	if (manager.sendtoWait(buf, 9, from, DATA_ACK) == RH_ROUTER_ERROR_NONE) {
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

// ************************************************************************
// *****                         Node Functions                       *****
// ************************************************************************
bool composeDataReportNode() {
	Log.info("Sending data report to Gateway");
	digitalWrite(BLUE_LED,HIGH);

	static uint8_t msgCnt = 0;

	buf[0] = highByte(sysStatus.nodeNumber);								// to be replaced/updated
	buf[1] = lowByte(sysStatus.nodeNumber);								// to be replaced/updated
	buf[2] = highByte(sysStatus.deviceID);					// Set for device
	buf[3] = lowByte(sysStatus.deviceID);
	buf[4] = 1;						// Set for code release - fix later
	buf[5] = highByte(current.hourly);
	buf[6] = lowByte(current.hourly); 
	buf[7] = highByte(current.daily);
	buf[8] = lowByte(current.daily); 
	buf[9] = current.internalTempC;
	buf[10] = current.stateOfCharge;
	buf[11] = current.batteryState;	
	buf[12] = sysStatus.resetCount;
	buf[13] = 1;				// reserved for later
	buf[14] = highByte(driver.lastRssi());
	buf[15] = lowByte(driver.lastRssi()); 
	buf[16] = msgCnt++;

	// Send a message to manager_server
  	// A route to the destination will be automatically discovered.
	Log.info("sending message %d", buf[16]);
	if (manager.sendtoWait(buf, 17, GATEWAY_ADDRESS, DATA_RPT) == RH_ROUTER_ERROR_NONE) {
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

bool receiveAcknowledmentDataReportNode() {
	if (manager.recvfromAckTimeout(buf, &len, 3000, &from,__null,__null,&messageFlag)) {
		buf[len] = 0;
		lora_state = (LoRA_State)messageFlag;
		Log.info("Received from node %d with rssi=%d - a %s message of length %d", from, driver.lastRssi(), loraStateNames[lora_state] ,len);
		sysStatus.nextReportSeconds = ((buf[7] << 8) | buf[8]);
		uint32_t newTime = ((buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4]);
		Time.setTime(newTime);  // Set time based on response from gateway
		Log.info("Time set to %s and next report is in %u seconds", Time.timeStr(newTime).c_str(),sysStatus.nextReportSeconds);
		return true;
	}
	else {
		Log.info("No reply, are the gateways running?");
		return false;
	}
}
