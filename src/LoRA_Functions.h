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
buf[2] firmVersion                          // Set for code release
buf[3 - 4] hourly                           // Hourly count
buf[5 - 6] = daily                          // Daily Count
buf[7] sensorType                           // What sensor type is it
buf[8] temp;                                // Enclosure temp
buf[9] battChg;                             // State of charge
buf[10] battState;                           // Battery State
buf[11] resets                              // Reset count
buf[12] msgCnt++;                           // Sequential message number
*/

// Format of a data acknowledgement
/*    
    buf[0 - 1 ] magicNumber                 // Magic Number
    buf[2 - 5 ] Time.now()                  // Set the time 
    buf[6 - 7] frequencyMinutes             // For the Gateway minutes on the hour
    buf[8] openHours                        // From the Gateway to the node - is the park open?
    buf[9] alertCode                        // This lets the Gateway trigger an alert on the node - typically a join request
    buf[10] message number                  // Parrot this back to see if it matches
*/

// Format of a join request
/*
buf[0-1] magicNumber;                       // Magic Number
buf[2- 26] Particle deviceID;               // deviceID is unique to the device
buf[27] sensorType				// Identifies sensor type to Gateway
*/

// Format for a join acknowledgement
/*
    buf[0 - 1 ]  magicNumber                // Magic Number
    buf[2 - 5 ] Time.now()                  // Set the time 
    buf[6 - 7] frequencyMinutes             // For the Gateway minutes on the hour  
    buf[8]  newNodeNumber                   // New Node Number for device
    buf[9] sensorType				// Gateway confirms sensor type
*/

// Format for an alert Report
/*
buf[0 - 1 ] magicNumber                     // Magic Number
buf[2] = alertCodeNode);                    // Node's Alert Code
*/

// Format for an Alert Report Acknowledgement
/*
    buf[0 - 1 ]  magicNumber                // Magic Number
    buf[2 - 5 ] Time.now()                  // Set the time 
    buf[6 - 7] frequencyMinutes             // For the Gateway minutes on the hour  
    buf[7] alertCodeNode                    // Zero for acknowledgement 
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
    uint8_t findNodeNumber(const char* deviceID);
    /**
     * @brief Returns the deviceID for a provided node number.  this is used in composing Particle publish payloads
     *
     * @param nodeNumber
     * @returns device ID
     * 
     */
    String findDeviceID(uint8_t nodeNumber);

    

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
