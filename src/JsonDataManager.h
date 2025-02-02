#ifndef __JSONDATAMANAGER_H
#define __JSONDATAMANAGER_H

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
 * JsonDataManager::instance().setup(gateway - true or false);
 * 
 * From global application loop you must call:
 * JsonDataManager::instance().loop();
 */
class JsonDataManager {
public:

    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use JsonDataManager::instance() to instantiate the singleton.
     */
    static JsonDataManager &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use JsonDataManager::instance().setup();
     */
    bool setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use JsonDataManager::instance().loop();
     */
    void loop();

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
     * @brief Get the current alert code pending value from the nodeID data structure
     * 
     * @param nodeNumber 
     * @return byte 
     */
    byte getAlertCode(int nodeNumber);

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
     * @brief Get the current alert context pending value from the nodeID data structure
     * 
     * @param nodeNumber 
     * @return byte 
     */
    uint16_t getAlertContext(int nodeNumber);

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
     * @brief sets the last report field to Time.now() for the specified nodeNumber
     * 
     * @param nodeNumber
     * @param newLastReport - a unix timestamp as an integer
     * @return true
     * @return false if the nodeNumber suggests an unconfigured node
     */
    bool setLastReport(int nodeNumber, int newLastReport);


    /**********************************************************************
     **           Occupancy Specific Node Management Functions           **
     **********************************************************************/

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
     * @brief Parses the JSON database to set the occupancyNet values for all nodes in ALL spaces to 0
     *        Also preemptively updates spaces in Ubidots with the new zeroed values so we don't have to wait for a new report to come in.
     *  
     * @return true if successful, false if not
     */
    bool resetOccupancyNetCounts();

    /**
     * @brief Parses the JSON database to reset the currentData (occupancyGross, occupancyNet) for all nodes in ALL spaces
     *        Also preemptively updates spaces in Ubidots with the new zeroed values so we don't have to wait for a new report to come in.
     *  
     * @return true if successful, false if not
     */
    bool resetOccupancyCounts();

    /**
     * @brief Parses the JSON database to set the occupancyNet values for a specified node to 0
     *        Also preemptively updates its space in Ubidots with the new zeroed values so we don't have to wait for a new report to come in.
     * 
     * @param nodeNumber
     * @param newOccupancyNet
     * @return true if successful, false if not
     */
    bool setOccupancyNetForNode(int nodeNumber, int newOccupancyNet);

    /**********************************************************************
     **                         Helper Functions                         **
     **********************************************************************/

    /**
     * @brief Primarily used for debugging
     * 
     */
    void printNodeData(bool publish);

    /**
     * @brief Returns the node number for the deviceID provided.  This is used in join requests
     * 
     * @param nodeID
     * @returns the node number for this deviceID
     * 
     */
    uint8_t findNodeNumber(int nodeNumber, uint32_t nodeID);
    
    /**
     * @brief Returns true if node is configured and false if it is not
     * 
     * @param nodeNumber and a float for the % of successful messages delivered
     * @param uniqueID the id to check
     * @returns true or false
     */
    bool checkIfNodeConfigured(int nodeNumber, uint32_t uniqueID);

    /**
     * @brief Returns true if the uniqueID for the node exists in the database false if it does not
     * 
     * @param radioID the id to check
     * @returns true or false
     */
    bool uniqueIDExistsInDatabase(uint32_t uniqueID);

    /**
     * @brief NodeNumberForUniqueId is a function that returns the node number for a given uniqueID
     * 
     * @param uniqueID
     * @returns node number
     */
    byte getNodeNumberForUniqueID(uint32_t uniqueID);

    /**
     * @brief Parses the JSON database to set the occupancyNet and occupancyGross values for all nodes in a space to 0
     *        Also preemptively updates Ubidots with the new zeroed values so we don't have to wait for a new report to come in.
     * 
     * @param space
     * @return true if successful, false if not
     */
    bool resetSpace(int space);

    /**
     * @brief Parses the JSON database, checking the lastReport timestamp of all nodes in order to see if any spaces have not been updated for [lengthOfInactivity] seconds.
     *        If all nodes in a space have not reported for 1 hour, this function calls resetSpace on that space to reset the node counts and update Ubidots.
     *        Resets multiple spaces if needed.
     * 
     * @param secondsInactive
     * @return true if at least 1 space was reset successfully, false if not
     */
    bool resetInactiveSpaces(int secondsInactive);

    /**
     * @brief Resets the current data for a node in the JSON, then sends alert code 6
     *        Also preemptively updates Ubidots with the new zeroed values so we don't have to wait for a new report to come in.
     * 
     * @param nodeNumber
     * @return true if successful, false if not
     */
    bool resetCurrentDataForNode(int nodeNumber);

     /**
     * @brief Resets the current data for a node in the JSON, then sends alert code 5
     *        Also preemptively updates Ubidots with the new zeroed values so we don't have to wait for a new report to come in.
     * 
     * @param nodeNumber
     * @return true if successful, false if not
     */
    bool resetAllDataForNode(int nodeNumber);

    /** 
     * @brief Saves the node database as a string to memory.
     */
    bool saveNodeDatabase(JsonParser &jp);


    /**********************************************************************
     **                         Data Compression                         **
     **********************************************************************/

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
     * Use JsonDataManager::instance() to instantiate the singleton.
     */
    JsonDataManager();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~JsonDataManager();

    /**
     * This class is a singleton and cannot be copied
     */
    JsonDataManager(const JsonDataManager&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    JsonDataManager& operator=(const JsonDataManager&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static JsonDataManager *_instance;

};
#endif  /* __LORA_FUNCTIONS_H */
