/**
 * @file LoRA-Functions.h - Singleton approach
 * @author Chip McClelland (chip@seeinisghts.com)
 * @brief This file allows me to move all the LoRA functionality out of the main program - application implementation / not general
 * @version 0.1
 * @date 2022-09-14
 * 
 */

// Data exchange formats
// Format of a data report
/*
buf[0] = highByte(deviceID);                    // Set for device
buf[1] = lowByte(deviceID);
buf[2] = highByte(nodeNumber);                  // Set for device
buf[3] = lowByte(nodeNumber);
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
	buf[0] = 128;									// Magic Number
	buf[1] = ((uint8_t) ((Time.now()) >> 24)); 		// Fourth byte - current time
	buf[2] = ((uint8_t) ((Time.now()) >> 16));		// Third byte
	buf[3] = ((uint8_t) ((Time.now()) >> 8));		// Second byte
	buf[4] = ((uint8_t) (Time.now()));		    	// First byte	
	buf[5] = highByte(sysStat.freqencyMinutes);		// For the Gateway minutes on the hour
	buf[6] = lowByte(sysStatus.frequencyMinutes);	// 16-bit minutes		
	buf[5] = highByte(sysStatus.nextReportSeconds);	// Seconds until next report - 16-bit number can go over 2 days
	buf[6] = lowByte(sysStatus.nextReportSeconds);	// 16-bit minutes
*/
// Format of a join request
/*
buf[0] = highByte(devID);                       // deviceID is unique to the device
buf[1] = lowByte(devID);
buf[2] = highByte(nodeNumber);                  // Node Number
buf[3] = lowByte(nodeNumber);
buf[2] = magicNumber;							// Needs to equal 128
buf[3] = highByte(rssi);				        // Signal strength
buf[4] = lowByte(rssi); 
*/
// Format for a join acknowledgement
/*
	buf[0] = 128;								// Magic number - so you can trust me
	buf[1] = ((uint8_t) ((Time.now()) >> 24));  // Fourth byte - current time
	buf[2] = ((uint8_t) ((Time.now()) >> 16));	// Third byte
	buf[3] = ((uint8_t) ((Time.now()) >> 8));	// Second byte
	buf[4] = ((uint8_t) (Time.now()));		    // First byte			
	buf[5] = highByte(newNodeNumber);			// New Node Number for device
	buf[6] = lowByte(newNodeNumber);	
	buf[7] = highByte(nextSecondsShort);		// Seconds until next report - for Nodes
	buf[8] = lowByte(nextSecondsShort);
*/
// Format for an alert Report
/*
buf[0] = highByte(deviceID);                    // Set for device
buf[1] = lowByte(deviceID);
buf[2] = highByte(nodeNumber);                  // Set for device
buf[3] = lowByte(nodeNumber);
buf[4] = highByte(current.alertCodeNode);   // Node's Alert Code
buf[5] = ((uint8_t) ((Time.now()) >> 24));  // Fourth byte - current time
buf[6] = ((uint8_t) ((Time.now()) >> 16));	// Third byte
buf[7] = ((uint8_t) ((Time.now()) >> 8));	// Second byte
buf[8] = ((uint8_t) (Time.now()));		    // First byte			
buf[9] = highByte(driver.lastRssi());		// Signal strength
buf[10] = lowByte(driver.lastRssi()); 
	*/
// Format for an Alert Report Acknowledgement
/*
	buf[0] = 0;									// Reserved
	buf[1] = ((uint8_t) ((Time.now()) >> 24));  // Fourth byte - current time
	buf[2] = ((uint8_t) ((Time.now()) >> 16));	// Third byte
	buf[3] = ((uint8_t) ((Time.now()) >> 8));	// Second byte
	buf[4] = ((uint8_t) (Time.now()));		    // First byte			
	buf[5] = highByte(nextSecondsShort);		// Seconds until next report - for Nodes
	buf[6] = lowByte(nextSecondsShort);
*/

#ifndef __LORA_FUNCTIONS_H
#define __LORA_FUNCTIONS_H

#include "Particle.h"

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * LoRA_Functions::instance().setup(gateway - true or false);
 * 
 * From global application loop you must call:
 * LoRA_Functions::instance().loop();
 */
class LoRA_Functions {
public:
    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use LoRA_Functions::instance() to instantiate the singleton.
     */
    static LoRA_Functions &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use LoRA_Functions::instance().setup();
     */
    bool setup(bool gatewayID);

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use LoRA_Functions::instance().loop();
     */
    void loop();

    // Generic Gateway Functions
    /**
     * @brief This function is used to listen for all message types
     * 
     * @details - Use polling with this function - executed in main LoRA_STATE every loop transit
     * 
     * @param None
     * 
     * @return true - if a message has been returned (sets message flag value)
     * @return false - no message received
     */
    bool listenForLoRAMessageGateway();     

    /**
     * @brief Function used to respond to messages from nodes to the gateway
     * 
     * @param nextSeconds - the number of seconds till next transmission window
     * @return true - response acknowledged
     * @return false 
     */
    bool respondForLoRAMessageGateway(int nextSeconds);  

    // Specific Gateway Message Functions
    /**
     * @brief Function that unpacks a data report from a node
     * 
     * @return true 
     * @return false 
     */
    bool decipherDataReportGateway();   
    /**
     * @brief Function that unpacks a Join request from a node
     * 
     * @return true 
     * @return false 
     */
    bool decipherJoinRequestGateway();     
    /**
     * @brief Function that unpacks an alert report from a node
     * 
     * @return true 
     * @return false 
     */
    bool decipherAlertReportGateway();  
    /**    
     * @brief Sends an acknolwedgement from the gateway to the node after successfully unpacking an alert report.
     * Also sends the number of seconds until next transmission window.
     * 
     * @param nextSeconds 
     * @return true 
     * @return false 
     */
    bool acknowledgeDataReportGateway(int nextSeconds);    // Gateway- acknowledged receipt of a data report
    /**
     * @brief Sends an acknolwedgement from the gateway to the node after successfully unpacking a data report.
     * Also sends the number of seconds until next transmission window.
     * @param nextSeconds 
     * @return true 
     * @return false 
     */
    bool acknowledgeJoinRequestGateway(int nextSeconds);   // Gateway - acknowledged receipt of a join request
    /**
     * @brief Sends an acknolwedgement from the gateway to the node after successfully unpacking a join request.
     * Also sends the number of seconds until next transmission window.
     * @param nextSeconds 
     * @return true 
     * @return false 
     */
    bool acknowledgeAlertReportGateway(int nextSeconds);   // Gateway - acknowledged receipt of an alert report

    // Node Functions
    /**
     * @brief Listens for the gateway to respond to the node - takes note of message flag
     * 
     * @return true 
     * @return false 
     */
    bool listenForLoRAMessageNode();                // Node - sent a message - awiting reply
    /**
     * @brief Composes a Data Report and sends to the Gateway
     * 
     * @return true 
     * @return false 
     */
    bool composeDataReportNode();                  // Node - Composes data report
    /**
     * @brief Acknowledges the response from the Gateway that acknowledges receipt of a data report
     * 
     * @return true 
     * @return false 
     */
    bool receiveAcknowledmentDataReportNode();     // Node - receives acknolwedgement
    /**
     * @brief Composes a Join Request and sends to the Gateway
     * 
     * @return true 
     * @return false 
     */
    bool composeJoinRequesttNode();                // Node - Composes Join request
    /**
     * @brief Acknowledges the response from the Gateway that acknowledges receipt of a Join Request
     * 
     * @return true 
     * @return false 
     */
    bool receiveAcknowledmentJoinRequestNode();    // Node - received join request asknowledgement
    /**
     * @brief Composes an alert report and sends to the Gateway
     * 
     * @return true 
     * @return false 
     */
    bool composeAlertReportNode();                  // Node - Composes alert report
    /**
      * @brief Acknowledges the response from the Gateway that acknowledges receipt of an alert report
     * 
     * @return true 
     * @return false 
     */
    bool receiveAcknowledmentAlertReportNode();    // Node - received alert report asknowledgement


protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use LoRA_Functions::instance() to instantiate the singleton.
     */
    LoRA_Functions();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~LoRA_Functions();

    /**
     * This class is a singleton and cannot be copied
     */
    LoRA_Functions(const LoRA_Functions&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    LoRA_Functions& operator=(const LoRA_Functions&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static LoRA_Functions *_instance;

};
#endif  /* __LORA_FUNCTIONS_H */
