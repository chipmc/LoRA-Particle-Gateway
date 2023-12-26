/**
 * @file LoRA-Functions.h - Singleton approach
 * @author Chip McClelland (chip@seeinisghts.com)
 * @brief This file allows me to move all the LoRA functionality out of the main program - application implementation / not general
 * @version 0.1
 * @date 2022-09-14
 * 
 */

// Data exchange formats
// Format of a data report - From the Node to the Gateway so includes a token - most common message from node to gateway
/*
*** Header Section - Common to all Nodes
buf[0 - 1] magicNumber                      // Magic number that identifies the Gateway's network
buf[2] nodeNumber                           // nodeNumber - unique to each node on the gateway's network
buf[3 - 4] Token                            // Token given to the node and good for 24 hours
buf[5] sensorType                           // What sensor type is it
buf[6 - 9] uniqueID          // This is a 4-byte identifier that is unique to each node and is only set once
*** Payload Section - 12 Bytes but interpretion is different for each sensor type
buf[10 - 21] payload                        // Payload - 12 bytes sensor type determines interpretation
*** Status Data - Common to all Nodes
buf[22] temp;                               // Enclosure temp (single byte signed integer -127 to 127 degrees C)
buf[23] battChg;                            // State of charge (single byte signed integer -1 to 100%)
buf[24] battState;                          // Battery State (single byte signed integer 0 to 6)
buf[25] resets                              // Reset count
buf[26-27] RSSI                             // From the Node's perspective
buf[28-29] SNR                              // From the Node's perspective
*** Re-Transmission Data - Common to all Nodes
buf[30] Re-Tries                            // This byte is dedicated to RHReliableDatagram.cpp to update the number of re-transmissions
buf[31] Re-Transmission Delay               // This byte is dedicated to RHReliableDatagram.cpp to update the accumulated delay with each re-transmission
*/

// Format of a data acknowledgement - From the Gateway to the Node - Most common message from gatewat to node
/*    
    buf[0 - 1] magicNumber                  // Magic Number
    buf[2] nodeNumber                       // Node number (unique for the network)
    buf[3 - 4] Token                        // Parrot the token back to the node
    buf[5 - 8] Time.now()                   // Set the time 
    buf[9 - 10] Seconds to next Report      // The gateway tells the node how many seconds until next transmission window - up to 18 hours
    buf[11] alertCode                       // This lets the Gateway trigger an alert on the node - typically a join request
    buf[12] sensorType                      // Let's the Gateway reset the sensor if needed 
    buf[13] Re-Tries                        // This byte is dedicated to RHReliableDatagram.cpp to update the number of re-transmissions
    buf[14] Re-Transmission Delay           // This byte is dedicated to RHReliableDatagram.cpp to update the accumulated delay with each re-transmission
*/

// Format of a join request - From the Node to the Gateway
// SensorType - 0/pressure, 1/PIR, 2/Magnetometer, 4/Soil

/*
buf[0-1] magicNumber;                       // Magic Number
buf[2] nodeNumber;                          // nodeNumber - typically 255 for a join request
buf[3 - 4] token                            // token for validation - may not be valid - if it is valid - response is to set the clock only
buf[5] sensorType				            // Identifies sensor type to Gateway
buf[6 - 9] Unique Node Identifier           // This is a 4-byte identifier that is unique to each node and is only set once
buf[10-13] Payload                          // Payload - 4 bytes sensor type determines interpretation
buf[14]  Re-Tries                           // This byte is dedicated to RHReliableDatagram.cpp to update the number of re-transmissions
buf[15]  Re-Transmission Delay              // This byte is dedicated to RHReliableDatagram.cpp to update the accumulated delay with each re-transmission
*/

// Format for a join acknowledgement -  From the Gateway to the Node
/*
    buf[0 - 1]  magicNumber                 // Magic Number
    buf[2] nodeNumber;                      // nodeNumber - assigned by the Gateway
    buf[3 - 4] token                        // token for validation - good for the day
    buf[5 - 8] Time.now()                   // Set the time 
    buf[9 - 10] Seconds till next report    // For the Gateway minutes on the hour  
    buf[11] alertCodeNode                   // Gateway can set an alert code here
    buf[12]  sensorType				        // Gateway confirms sensor type
    buf[13 - 16] uniqueID                   // This is a 4-byte identifier that is unique to each node and is only set once by the gateway on 1st joining
    buf[17] new nodeNumber                  // Gateway assigns a node number
    buf[18-21] Payload                      // Payload - 4 bytes sensor type determines interpretation                         
    buf[22]  Re-Tries                       // This byte is dedicated to RHReliableDatagram.cpp to update the number of re-transmissions
    buf[23] Re-Transmission Delay           // This byte is dedicated to RHReliableDatagram.cpp to update the accumulated delay with each re-transmission
*/

#ifndef __LORA_FUNCTIONS_H
#define __LORA_FUNCTIONS_H

#include "Particle.h"
#include "Particle_Functions.h"
#include <RH_RF95.h>
#include <RHEncryptedDriver.h>
#include <RHMesh.h>
#include <Speck.h>
#include <base64RK.h>
#include "device_pinout.h"
#include "MyPersistentData.h"
#include "JsonParserGeneratorRK.h"
#include "LocalTimeRK.h"

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
     * @brief Returns the node number for the deviceID provided.  This is used in join requests
     * 
     * @param deviceID
     * @returns the node number for this deviceID
     * 
     */
    uint8_t findNodeNumber(int nodeNumber, uint32_t nodeID);

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
    bool setType(int nodeNumber, int newType);

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
    bool setAlert(int nodeNumber, int newAlert);

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
    bool nodeConfigured(int nodeNumber, uint32_t radioID);
 

    /** 
     * @brief This function returns true if the nodeNumber / Token combination is valid
    */
    bool checkForValidToken(uint8_t nodeNumber, uint16_t token);

    /**
     * @brief This function calculates a valid token for a node
     *
     */
    uint16_t setNodeToken(uint8_t nodeNumber);

    /**
     * @brief Loads the current payload values with the values from the node
    */
   void getPayload(uint8_t nodeNumber);


   /**
    * @brief Changes the payload values for a given node
   */
   void setPayload(uint8_t nodeNumber);


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
