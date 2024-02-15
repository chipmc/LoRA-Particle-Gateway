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
buf[6 - 9] uniqueID                         // This is a 4-byte identifier that is unique to each node and is only set once
*** Payload Section - 12 Bytes but interpretion is different for each sensor type
buf[10 - 17] payload                        // Payload - 8 bytes sensor type determines interpretation
*** Status Data - Common to all Nodes
buf[18] temp;                               // Enclosure temp (single byte signed integer -127 to 127 degrees C)
buf[19] battChg;                            // State of charge (single byte signed integer -1 to 100%)
buf[20] battState;                          // Battery State (single byte signed integer 0 to 6)
buf[21] resets                              // Reset count
buf[22-23] RSSI                             // From the Node's perspective
buf[24-25] SNR                              // From the Node's perspective
*** Re-Transmission Data - Common to all Nodes
buf[26] Re-Tries                            // This byte is dedicated to RHReliableDatagram.cpp to update the number of re-transmissions
buf[27] Re-Transmission Delay               // This byte is dedicated to RHReliableDatagram.cpp to update the accumulated delay with each re-transmission
*/

// Format of a data acknowledgement - From the Gateway to the Node - Most common message from gatewat to node
/*    
    buf[0 - 1] magicNumber                  // Magic Number
    buf[2] nodeNumber                       // Node number (unique for the network)
    buf[3 - 4] Token                        // Parrot the token back to the node
    buf[5 - 8] Time.now()                   // Set the time 
    buf[9 - 10] Seconds to next Report      // The gateway tells the node how many seconds until next transmission window - up to 18 hours
    buf[11] alertCodeNode                   // This lets the Gateway trigger an alert on the node - typically a join request
    buf[12-13] alertContextNode                // This lets the Gateway send context with an alert code if needed
    buf[14] sensorType                      // Let's the Gateway reset the sensor if needed 
    buf[15] Re-Tries                        // This byte is dedicated to RHReliableDatagram.cpp to update the number of re-transmissions
    buf[16] Re-Transmission Delay           // This byte is dedicated to RHReliableDatagram.cpp to update the accumulated delay with each re-transmission
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
    buf[12] alertContextNode                // This lets the Gateway send context with an alert code if needed
    buf[13]  sensorType				        // Gateway confirms sensor type
    buf[14 - 17] uniqueID                   // This is a 4-byte identifier that is unique to each node and is only set once by the gateway on 1st joining
    buf[18] new nodeNumber                  // Gateway assigns a node number
    buf[19-22] Payload                      // Payload - 4 bytes sensor type determines interpretation                         
    buf[23]  Re-Tries                       // This byte is dedicated to RHReliableDatagram.cpp to update the number of re-transmissions
    buf[24] Re-Transmission Delay           // This byte is dedicated to RHReliableDatagram.cpp to update the accumulated delay with each re-transmission
*/

#ifndef __LORA_FUNCTIONS_H
#define __LORA_FUNCTIONS_H

#include "Particle.h"
#include "Particle_Functions.h"
#include <RH_RF95.h>
#include <RHEncryptedDriver.h>
#include <RHMesh.h>
#include <Speck.h>
#include "Base64RK.h"
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
     * @brief NodeNumberForUniqueId is a function that returns the node number for a given uniqueID
     * 
     * @param uniqueID
     * @returns node number
     */
    byte getNodeNumberForUniqueID(uint32_t uniqueID);

    /**
     * @brief Get the current alert code pending value from the nodeID data structure
     * 
     * @param nodeNumber 
     * @return byte 
     */
    byte getAlertCode(int nodeNumber);

    /**
     * @brief Get the current alert context pending value from the nodeID data structure
     * 
     * @param nodeNumber 
     * @return byte 
     */
    uint16_t getAlertContext(int nodeNumber);

    /**
     * @brief Changes the alert value that is pending for the next report from the node
     * 
     * @param nodeNumber 
     * @param newAlertCode
     * @return true 
     * @return false 
     */
    bool setAlertCode(int nodeNumber, int newAlertCode);

    /**
     * @brief Changes the jsonData1 value for a node 
     * 
     * @param nodeNumber
     * @param sensorType
     * @param newJsonData1
     * @return true
     * @return false if the nodeNumber suggests an unconfigured node or sensorType is not known
     */
    bool setJsonData1(int nodeNumber, int sensorType, int newJsonData1);

    /**
     * @brief Changes the jsonData2 value for a node
     * 
     * @param nodeNumber
     * @param sensorType
     * @param newJsonData2
     * @return true
     * @return false if the nodeNumber suggests an unconfigured node or sensorType is not known
     */
    bool setJsonData2(int nodeNumber, int sensorType, int newJsonData2);

    /**
     * @brief Parses the JSON database to return the sum of the occupancyNet values for all nodes in a space
     * 
     * @param space
     * @return occupancyNet total for the space
     */
    uint16_t getOccupancyNetBySpace(int space);

    /**
     * @brief Parses the JSON database to return the sum of the occupancyGross values for all nodes in a space
     * 
     * @param space
     * @return occupancyGross total for the space
     */
    uint16_t getOccupancyGrossBySpace(int space);

    /**
     * @brief Parses the JSON database to set the occupancyNet and occupancyGross values for all nodes in a space to 0
     * 
     * @return true if successful, false if not
     */
    bool resetOccupancyCounts();

    /**
     * @brief Changes the alert context that is pending for the next report from the node
     * 
     * @param nodeNumber 
     * @param newAlertContext
     * @return true 
     * @return false 
     */
    bool setAlertContext(int nodeNumber, int newAlertContext);

    /**
     * @brief Primarily used for debugging
     * 
     */
    void printNodeData(bool publish);
    
    /**
     * @brief Returns true if node is configured and false if it is not
     * 
     * @param nodeNumber and a float for the % of successful messages delivered
     * @param uniqueID the id to check
     * @returns true or false
     */
    bool nodeConfigured(int nodeNumber, uint32_t uniqueID);

    /**
     * @brief Returns true if the uniqueID for the node exists in the database false if it does not
     * 
     * @param radioID the id to check
     * @returns true or false
     */
    bool uniqueIDExistsInDatabase(uint32_t uniqueID);

    /** 
     * @brief This function returns true if the nodeNumber / Token combination is valid
    */
    bool checkForValidToken(uint8_t nodeNumber, uint16_t token);

    /**
     * @brief This function calculates a valid token for a node
     */
    uint16_t setNodeToken(uint8_t nodeNumber);

    /**
     * @brief Loads the current "Join Payload" values with the values from the node into the currentData struct.
     * @return true if successful, false if not
    */
    bool getJoinPayload(uint8_t nodeNumber);

    /**
    * @brief Changes the "Join Payload" values for a given node to be equal to the values in payload1-payload4 of the currentData struct.
    * @return true if successful, false if not
    */
    bool setJoinPayload(uint8_t nodeNumber);

    /**
     * @brief Compresses currentData's payload1 - payload4 (Join Payload) based on a sensor type, returns it
     *
     * @details This function is a higher-level compression function that utilizes the 
     * JSON database and compressData functions to compress the payload based on
     * the device type that is stored in the JSON database.
     * 
     * @param sensorType the sensor type of the node who's payload we are decompressing
     */
    uint8_t getCompressedJoinPayload(uint8_t sensorType);

    /**
     * @brief Decompresses the compressed payload and hydrates the currentData object for a payload based on a sensor type
     *
     * @details This function is a higher-level decompression function that utilizes the 
     * JSON database and decompressData functions to decompress the payload based on
     * a device type nd hydrates the currentData object for a payload based on a sensor type.
     * 
     * @param sensorType the sensor type of the node who's payload we are decompressing
     * @param compressedPayload the payload variables compressed to 1 byte
     * @return true if successful, false if not
     */
    bool hydrateJoinPayload(uint8_t sensorType, uint8_t compressedPayload);

    /**
     * @brief Parses a compressed payload and sets the payload1 - payload4 variables (Join Payload) with those values.
     *
     * @details Takes a compressed payload and 4 uint8_t variables. Decompresses the payload into its
     * components based on a given device type
     * 
     * @param sensorType the sensor type of the node who's payload we are decompressing
     * @param compressedJoinPayload the payload variables compressed to 1 byte
     * @param payload1 payload1 to set
     * @param payload2 payload2 to set
     * @param payload3 payload3 to set
     * @param payload4 payload4 to set
     * @return true if successful, false if not
     */
    bool parseJoinPayloadValues(uint8_t sensorType, uint8_t compressedJoinPayload, uint8_t& payload1, uint8_t& payload2, uint8_t& payload3, uint8_t& payload4);

    /**
     * @brief Compresses up to four pieces of data into a single byte.
     *
     * @details This function compresses up to four pieces of data into a single byte
     * using the specified number of bits for each data element.
     *
     * @param bitSizes An array specifying the number of bits for each data element.
     * @param data An array containing the data to be compressed.
     * @return The compressed data as a single byte.
     */
    uint8_t compressData(uint8_t bitSizes[], uint8_t data[]);

    /**
     * @brief Decompresses a single byte into up to four pieces of data.
     *
     * @details This function decompresses a single byte into up to four pieces of data
     * using the specified number of bits for each data element.
     *
     * @param compressedData The compressed data as a single byte.
     * @param bitSizes An array specifying the number of bits for each data element.
     * @param data An array to store the decompressed data.
     */
    void decompressData(uint8_t compressedData, uint8_t bitSizes[], uint8_t data[]);

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
