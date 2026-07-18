#ifndef __MYPERSISTENTDATA_H
#define __MYPERSISTENTDATA_H

#include "Particle.h"
#include "MB85RC256V-FRAM-RK.h"
#include "StorageHelperRK.h"
#include "config.h"

//Macros(#define) to swap out during pre-processing (use sparingly). This is typically used outside of this .H and .CPP file within the main .CPP file or other .CPP files that reference this header file. 
// This way you can do "data.setup()" instead of "MyPersistentData::instance().setup()" as an example
#define current currentStatusData::instance()
#define sysStatus sysStatusData::instance()
#define nodeDatabase nodeIDData::instance()

// We use the 64kbit part so we have 8k bytes of storage
// SysStatus Object - starts at 0
// Current Object - starts at 100
// Node Object - starts at 200

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * MyPersistentData::instance().setup();
 * 
 * From global application loop you must call:
 * MyPersistentData::instance().loop();
 */

// *******************  SysStatus Storage Object **********************
//
// ********************************************************************

class sysStatusData : public StorageHelperRK::PersistentDataFRAM {
public:

    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use MyPersistentData::instance() to instantiate the singleton.
     */
    static sysStatusData &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use MyPersistentData::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use MyPersistentData::instance().loop();
     */
    void loop();

	/**
	 * @brief Validates values and, if valid, checks that data is in the correct range.
	 * 
	 */
	bool validate(size_t dataSize);

	/**
	 * @brief Will reinitialize data if it is found not to be valid
	 * 
	 * Be careful doing this, because when MyData is extended to add new fields,
	 * the initialize method is not called! This is only called when first
	 * initialized.
	 * 
	 */
	void initialize();


	class SysData {
	public:
		// This structure must always begin with the header (16 bytes)
		StorageHelperRK::PersistentDataBase::SavedDataHeader sysHeader;
		// Your fields go here. Once you've added a field you cannot add fields
		// (except at the end), insert fields, remove fields, change size of a field.
		// Doing so will cause the data to be corrupted!
		// Size of ssyStatus = 30 bytes + 16 for the header for 46 bytes total
		uint8_t nodeNumber;                               // Assigned by the gateway on joining the network
		uint8_t structuresVersion;                        // Version of the data structures (system and data)
		uint16_t magicNumber;							  // A way to identify nodes and gateways so they can trust each other
		uint8_t connectivityMode;                         // 0 - standard LoRA and Cellular, 1 - Long LoRA  2 - Always on LoRA, 3 - Always on LoRA and Cellular, 4 - Always on WiFi and LoRA
		uint8_t resetCount;                               // reset count of device (0-255)
		uint16_t messageCount;							  // This is how many messages the Gateay has composed for the day
		time_t lastHookResponse;                   		  // Last time we got a valid Webhook response
		time_t lastConnection;                     		  // Last time we successfully connected to Particle
		uint16_t lastConnectionDuration;                  // How long - in seconds - did it take to last connect to the Particle cloud
		uint16_t frequencySeconds;                        // When we are reporing at minute increments - what are they - for Gateways
		uint16_t updatedfrequencySeconds;				  // When we update the reporting frequency, it is stored here
		uint8_t alertCodeGateway;                         // Alert code for Gateway Alerts
		time_t alertTimestampGateway;              		  // When was the last alert
		uint8_t openTime;                                 // Open time 24 hours
		uint8_t closeTime;                                // Close time 24 hours
		uint8_t breakTime;                                // Break time 24 hours, set this to 24 if no break time is needed for this gateway.
		uint8_t breakLengthMinutes;                       // Break length 1-60 minutes.
		uint8_t weekendBreakTime;                         // Break time 24 hours (weekends), set this to 24 if no break time is needed for this gateway.
		uint8_t weekendBreakLengthMinutes;                // Break length 1-60 minutes (weekends).
		uint8_t tokenCore;								  // This is the random part of the daily token
	};
	SysData sysData;

	// 	******************* Get and Set Functions for each variable in the storage object ***********
    
	/**
	 * @brief For the Get functions, used to retrieve the value of the variable
	 * 
	 * @details Specific to the location in the object and the type of the variable
	 * 
	 * @param Nothing needed
	 * 
	 * @returns The value of the variable in the corret type
	 * 
	 */

	/**
	 * @brief For the Set functions, used to set the value of the variable
	 * 
	 * @details Specific to the location in the object and the type of the variable
	 * 
	 * @param Value to set the variable - correct type - for Strings they are truncated if too long
	 * 
	 * @returns None needed
	 * 
	 */

	uint8_t get_nodeNumber() const;
	void set_nodeNumber(uint8_t value);

	uint8_t get_structuresVersion() const ;
	void set_structuresVersion(uint8_t value);

	uint16_t get_magicNumber() const ;
	void set_magicNumber(uint16_t value);

	uint8_t get_connectivityMode() const;
	void set_connectivityMode(uint8_t value);

	uint8_t get_resetCount() const;
	void set_resetCount(uint8_t value);

	uint16_t get_messageCount() const;
	void set_messageCount(uint16_t value);

	time_t get_lastHookResponse() const;
	void set_lastHookResponse(time_t value);

	time_t get_lastConnection() const;
	void set_lastConnection(time_t value);

	uint16_t get_lastConnectionDuration() const;
	void set_lastConnectionDuration(uint16_t value);

	uint16_t get_frequencySeconds() const;
	void set_frequencySeconds(uint16_t value);

	uint16_t get_updatedfrequencySeconds() const;
	void set_updatedfrequencySeconds(uint16_t value);

	uint8_t get_alertCodeGateway() const;
	void set_alertCodeGateway(uint8_t value);

	time_t get_alertTimestampGateway() const;
	void set_alertTimestampGateway(time_t value);

	uint8_t get_openTime() const;
	void set_openTime(uint8_t value);

	uint8_t get_closeTime() const;
	void set_closeTime(uint8_t value);

	uint8_t get_breakTime() const;
	void set_breakTime(uint8_t value);

	uint8_t get_breakLengthMinutes() const;
	void set_breakLengthMinutes(uint8_t value);

	uint8_t get_weekendBreakTime() const;
	void set_weekendBreakTime(uint8_t value);

	uint8_t get_weekendBreakLengthMinutes() const;
	void set_weekendBreakLengthMinutes(uint8_t value);

	uint8_t get_tokenCore() const;
	void set_tokenCore(uint8_t value);

	//Members here are internal only and therefore protected
protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use MyPersistentData::instance() to instantiate the singleton.
     */
    sysStatusData();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~sysStatusData();

    /**
     * This class is a singleton and cannot be copied
     */
    sysStatusData(const sysStatusData&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    sysStatusData& operator=(const sysStatusData&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static sysStatusData *_instance;

    //Since these variables are only used internally - They can be private. 
	static const uint32_t SYS_DATA_MAGIC = 0x20a99e76;
	static const uint16_t SYS_DATA_VERSION = 1;

};




// *****************  Current Status Storage Object *******************
//
// ********************************************************************

class currentStatusData : public StorageHelperRK::PersistentDataFRAM {
public:

    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use MyPersistentData::instance() to instantiate the singleton.
     */
    static currentStatusData &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use MyPersistentData::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use MyPersistentData::instance().loop();
     */
    void loop();

	/**
	 * @brief Load the appropriate system defaults - good ot initialize a system to "factory settings"
	 * 
	 */
	void loadCurrentDefaults();                          // Initilize the object values for new deployments

	/**
	 * @brief Resets the current and hourly counts
	 * 
	 */
	void resetEverything();    

	/**
	 * @brief Validates values and, if valid, checks that data is in the correct range.
	 * 
	 */
	bool validate(size_t dataSize);

	/**
	 * @brief Will reinitialize data if it is found not to be valid
	 * 
	 * Be careful doing this, because when MyData is extended to add new fields,
	 * the initialize method is not called! This is only called when first
	 * initialized.
	 * 
	 */
	void initialize();           

	class CurrentData {
	public:
		// This structure must always begin with the header (16 bytes)
		StorageHelperRK::PersistentDataBase::SavedDataHeader currentHeader;
		// Your fields go here. Once you've added a field you cannot add fields
		// (except at the end), insert fields, remove fields, change size of a field.
		// Doing so will cause the data to be corrupted!
		// You may want to keep a version number in your data.
		// Size is 28 plus a header of 16
		uint8_t nodeNumber;                               // The nodeNumber of the device providing the current data 
		uint8_t tempNodeNumber;                           // Used when an unconfigured node joins the network
		uint16_t token;								  	  // Two bytes that can help us determine if a nodeNumber is misconfigured
		uint8_t sensorType;								  // What is the sensor type of the node sending current data
		uint32_t uniqueID;								  // Unique ID of the node sending current data
		uint8_t payload1;							  	  // Payload Data Byte 1
		uint8_t payload2;							  	  // Payload Data Byte 2
		uint8_t payload3;							  	  // Payload Data Byte 3
		uint8_t payload4;							  	  // Payload Data Byte 4  // First four bytes are used by both data and join messages
		uint8_t payload5;							  	  // Payload Data Byte 5
		uint8_t payload6;							  	  // Payload Data Byte 6
		uint8_t payload7;							  	  // Payload Data Byte 7
		uint8_t payload8;							  	  // Payload Data Byte 8
		uint8_t internalTempC;                            // Enclosure temperature in degrees C
		int8_t stateOfCharge;                             // Battery charge level
		uint8_t batteryState;                             // Stores the current battery state (charging, discharging, etc)
		uint8_t resetCount;								  // This is the number of resets for the node publishing data
		int16_t RSSI;                                     // Latest LoRA signal strength value from the Node
		int16_t SNR;									  // Latest LoRA Signal to Noise Ratio from the Node
		uint8_t alertCodeNode;                            // Alert code from node
		uint8_t openHours; 								  // Will set to true or false based on time of dat
		byte onBreak; 								  	  // Equal to 1 if the device is on "break", which resets all node counts
		uint8_t	hops;									  // How many hops did the message take to get to the gateway
		uint8_t retryCount;								  // How many retries did the message take to get to the gateway
		uint8_t retransmissionDelay;					  // How extra time did retransmissinos add to the time it took the message to get to the gateway
		// OK to add more fields here 
	};
	CurrentData currentData;

	// 	******************* Get and Set Functions for each variable in the storage object ***********
    
	/**
	 * @brief For the Get functions, used to retrieve the value of the variable
	 * 
	 * @details Specific to the location in the object and the type of the variable
	 * 
	 * @param Nothing needed
	 * 
	 * @returns The value of the variable in the corret type
	 * 
	 */

	/**
	 * @brief For the Set functions, used to set the value of the variable
	 * 
	 * @details Specific to the location in the object and the type of the variable
	 * 
	 * @param Value to set the variable - correct type - for Strings they are truncated if too long
	 * 
	 * @returns None needed
	 * 
	 */


	uint8_t get_nodeNumber() const;
	void set_nodeNumber(uint8_t value);

	uint8_t get_tempNodeNumber() const;
	void set_tempNodeNumber(uint8_t value);

	uint16_t get_token() const;
	void set_token(uint16_t value);

	uint8_t get_sensorType() const;
	void set_sensorType(uint8_t value);

	uint32_t get_uniqueID() const;
	void set_uniqueID(uint32_t value);

	uint8_t get_payload1() const;
	void set_payload1(uint8_t value);

	uint8_t get_payload2() const;
	void set_payload2(uint8_t value);

	uint8_t get_payload3() const;
	void set_payload3(uint8_t value);

	uint8_t get_payload4() const;
	void set_payload4(uint8_t value);

	uint8_t get_payload5() const;
	void set_payload5(uint8_t value);

	uint8_t get_payload6() const;
	void set_payload6(uint8_t value);

	uint8_t get_payload7() const;
	void set_payload7(uint8_t value);

	uint8_t get_payload8() const;
	void set_payload8(uint8_t value);

	uint8_t get_internalTempC() const ;
	void set_internalTempC(uint8_t value);

	int8_t get_stateOfCharge() const;
	void set_stateOfCharge(int8_t value);

	uint8_t get_batteryState() const;
	void set_batteryState(uint8_t value);

	uint8_t get_resetCount() const;
	void set_resetCount(uint8_t value);

	int16_t get_RSSI() const;
	void set_RSSI(int16_t value);

	int16_t get_SNR() const;
	void set_SNR(int16_t value);

	uint8_t get_alertCodeNode() const;
	void set_alertCodeNode(uint8_t value);

	uint8_t get_openHours() const;
	void set_openHours(uint8_t value);

	byte get_onBreak() const;
	void set_onBreak(byte value);

	uint8_t get_hops() const;
	void set_hops(uint8_t value);

	uint8_t get_retryCount() const;
	void set_retryCount(uint8_t value);

	uint8_t get_retransmissionDelay() const;
	void set_retransmissionDelay(uint8_t value);

	//Members here are internal only and therefore protected
protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use MyPersistentData::instance() to instantiate the singleton.
     */
    currentStatusData();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~currentStatusData();

    /**
     * This class is a singleton and cannot be copied
     */
    currentStatusData(const currentStatusData&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    currentStatusData& operator=(const currentStatusData&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static currentStatusData *_instance;

    //Since these variables are only used internally - They can be private. 
	static const uint32_t CURRENT_DATA_MAGIC = 0x20a99e81;
	static const uint16_t CURRENT_DATA_VERSION = 4;
};



// *****************  nodeID Data Storage Object **********************
//
// ********************************************************************

class nodeIDData : public StorageHelperRK::PersistentDataFRAM {
public:

    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use MyPersistentData::instance() to instantiate the singleton.
     */
    static nodeIDData &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use MyPersistentData::instance().setup();
     */
    void setup();

    /**
     * @brief Perform application loop operations; call this from global application loop()
     * 
     * You typically use MyPersistentData::instance().loop();
     */
    void loop();

	/**
	 * @brief Will reset the node database - if database is corrupted of new nodes arrive
	 * 
	 */
	void resetNodeIDs();

	/**
	 * @brief Validates values and, if valid, checks that data is in the correct range.
	 * 
	 */
	bool validate(size_t dataSize);

	/**
	 * @brief Will reinitialize data if it is found not to be valid
	 * 
	 * Be careful doing this, because when MyData is extended to add new fields,
	 * the initialize method is not called! This is only called when first
	 * initialized.
	 * 
	 */
	void initialize();


	class NodeData {
	public:
		// This structure must always begin with the header (16 bytes)
		StorageHelperRK::PersistentDataBase::SavedDataHeader nodeHeader;
		// Your fields go here. Once you've added a field you cannot add fields
		// (except at the end), insert fields, remove fields, change size of a field.
		// Doing so will cause the data to be corrupted!
		char nodeIDJson[3072];                             // JSON string that stores the nodeID data
	};
	NodeData nodeData;

	// 	******************* Get and Set Functions for each variable in the storage object ***********
    
	/**
	 * @brief For the Get functions, used to retrieve the value of the variable
	 * 
	 * @details Specific to the location in the object and the type of the variable
	 * 
	 * @param Nothing needed
	 * 
	 * @returns The value of the variable in the corret type
	 * 
	 */

	/**
	 * @brief For the Set functions, used to set the value of the variable
	 * 
	 * @details Specific to the location in the object and the type of the variable
	 * 
	 * @param Value to set the variable - correct type - for Strings they are truncated if too long
	 * 
	 * @returns None needed
	 * 
	 */

	String get_nodeIDJson() const;
	bool set_nodeIDJson(const char *str);

	/**
	 *  Removes any unformatted text from a JSON string
	 * 
	 *  @param jsonString the string to be cleaned
	 */
	void cleanJSON(char* jsonString);
	

	//Members here are internal only and therefore protected
protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use MyPersistentData::instance() to instantiate the singleton.
     */
    nodeIDData();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~nodeIDData();

    /**
     * This class is a singleton and cannot be copied
     */
    nodeIDData(const nodeIDData&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    nodeIDData& operator=(const nodeIDData&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static nodeIDData *_instance;

    //Since these variables are only used internally - They can be private. 
	static const uint32_t NODEID_DATA_MAGIC = 0x20a99e61;
	static const uint16_t NODEID_DATA_VERSION = 3;

};


#endif  /* __MYPERSISTENTDATA_H */
