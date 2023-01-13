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
buf[0 - 1] magicNumber                      // Magic number for devices
buf[2 - 3] nodeID                            // nodeID for verification
buf[4 - 5] hourly                           // Hourly count
buf[6 - 7] = daily                          // Daily Count
buf[8] sensorType                           // What sensor type is it
buf[9] temp;                                // Enclosure temp
buf[10] battChg;                             // State of charge
buf[11] battState;                          // Battery State
buf[12] resets                              // Reset count
buf[13] messageCount;                       // Sequential message number
buf[14] successCount;
*/

// Format of a data acknowledgement
/*    
    buf[0 - 1 ] magicNumber                 // Magic Number
    buf[2 - 5 ] Time.now()                  // Set the time 
    buf[6 - 7] frequencyMinutes             // For the Gateway minutes on the hour
    buf[8] alertCode                        // This lets the Gateway trigger an alert on the node - typically a join request
    buf[9] sensorType                       // Let's the Gateway reset the sensor if needed 
    buf[10] openHours                        // From the Gateway to the node - is the park open?
    buf[11] message number                  // Parrot this back to see if it matches
*/

// Format of a join request
/*
buf[0-1] magicNumber;                       // Magic Number
buf[2 - 3] nodeID                            // nodeID for verification
buf[4- 28] Particle deviceID;               // deviceID is unique to the device
buf[29] sensorType				            // Identifies sensor type to Gateway
*/

// Format for a join acknowledgement
/*
    buf[0 - 1 ]  magicNumber                // Magic Number
    buf[2 - 5 ] Time.now()                  // Set the time 
    buf[6 - 7] frequencyMinutes             // For the Gateway minutes on the hour  
    buf[8] alertCodeNode                   // Gateway can set an alert code here
    buf[9]  newNodeNumber                   // New Node Number for device
    buf[10]  sensorType				        // Gateway confirms sensor type

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

    // Common Functions
    /**
     * @brief Clear whatever message is in the buffer - good for waking
     * 
     * @details calls the driver and iterates through the message queue
     * 
    */
    void clearBuffer();

    /**
     * @brief Class to put the LoRA Radio to sleep when we exit the LoRa state
     * 
     * @details May help prevent the radio locking up based on local LoRA traffic interference
     * 
    */
    void sleepLoRaRadio();

    /**
     * @brief Initialize the LoRA radio
     * 
     */
   bool initializeRadio();


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
    bool respondToLoRAMessageGateway();  

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
    bool acknowledgeDataReportGateway();    // Gateway- acknowledged receipt of a data report
    /**
     * @brief Sends an acknolwedgement from the gateway to the node after successfully unpacking a data report.
     * Also sends the number of seconds until next transmission window.
     * @param nextSeconds 
     * @return true 
     * @return false 
     */
    bool acknowledgeJoinRequestGateway();   // Gateway - acknowledged receipt of a join request
    /**
     * @brief Sends an acknolwedgement from the gateway to the node after successfully unpacking a join request.
     * Also sends the number of seconds until next transmission window.
     * @param nextSeconds 
     * @return true 
     * @return false 
     */
    bool acknowledgeAlertReportGateway();   // Gateway - acknowledged receipt of an alert report
    /**
     * @brief Returns the node number for the deviceID provided.  This is used in join requests
     * 
     * @param deviceID
     * @returns the node number for this deviceID
     * 
     */
    uint8_t findNodeNumber(const char* deviceID, int radioID);
    /**
     * @brief Returns the deviceID for a provided node number.  this is used in composing Particle publish payloads
     *
     * @param nodeNumber
     * @returns device ID
     * 
     */
    String findDeviceID(int nodeNumber, int radioID);
    /**
     * @brief Get Type is a function that returns the sensor Type for a given node number
     * 
     * @param nodeNumber
     * @returns sensor type
     */
    byte getType(int nodeNumber);
    /**
     * @brief change the sensor type for a given node number
     * 
     * @param nodeNumber and new sensor type number
     * @returns sensor type
     * 
     */
    bool changeType(int nodeNumber, int newType);

    /**
     * @brief Get the current alert pending value from the nodeID data strcuture
     * 
     * @param nodeNumber 
     * @return byte 
     */
    byte getAlert(int nodeNumber);

    /**
     * @brief Changes the alert value that is pending for the next report from the node
     * 
     * @param nodeNumber 
     * @param newAlert 
     * @return true 
     * @return false 
     */
    bool changeAlert(int nodeNumber, int newAlert);

    /**
     * @brief Primarily used for debugging
     * 
     */
    void printNodeData(bool publish);
    /**
     * @brief Returns true if node is configured and false if it is not
     * 
     * @param nodeNumber and a float for the % of successful messages delivered
     * @returns true or false
     */
    bool nodeConfigured(int nodeNumber, int radioID);
    /**
     * @brief Update the success percent and last connect time for a node
     * 
     * @param nodeNumber 
     * @param successPercent 
     * @return true 
     * @return false 
     */
    bool nodeUpdate(int nodeNumber, float successPercent);
    /**
     * @brief Checks to see if the nodes have checked in recently
     * 
     * @returns true if healthy and false if not
     * 
     * @details Note: this function will reset the LoRA radio if no nodes have checked in over last two periods
     * 
     */
    bool nodeConnectionsHealthy();

    /**
     * @brief computes a two digit checksum based on the Particle deviceID
     * 
     * @param str - a 24 character hex number string
     * @return int - a value from 0 to 360 based on the character string
     */
    int stringCheckSum(String str);

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
