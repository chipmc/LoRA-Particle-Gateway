#ifndef __MYPERSISTENTDATA_H
#define __MYPERSISTENTDATA_H

#include "Particle.h"
#include "MB85RC256V-FRAM-RK.h"
#include "StorageHelperRK.h"

//Macros(#define) to swap out during pre-processing (use sparingly). This is typically used outside of this .H and .CPP file within the main .CPP file or other .CPP files that reference this header file. 
// This way you can do "data.setup()" instead of "MyPersistentData::instance().setup()" as an example
#define current currentStatusData::instance()
#define sysStatus sysStatusData::instance()
#define nodeID nodeIDData::instance()

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
		uint8_t stayConnected;                          // Version of the device firmware (integer - aligned to particle prodict firmware)
		uint8_t resetCount;                               // reset count of device (0-256)
		uint8_t messageCount;							  // This is how many messages the Gateay has composed for the day
		time_t lastHookResponse;                   		  // Last time we got a valid Webhook response
		time_t lastConnection;                     		  // Last time we successfully connected to Particle
		uint16_t lastConnectionDuration;                  // How long - in seconds - did it take to last connect to the Particle cloud
		uint16_t frequencyMinutes;                        // When we are reporing at minute increments - what are they - for Gateways
		uint16_t updatedFrequencyMinutes;				  // When we update the reporting frequency, it is stored here
		uint8_t alertCodeGateway;                         // Alert code for Gateway Alerts
		time_t alertTimestampGateway;              		  // When was the last alert
		uint8_t openTime;                                 // Open time 24 hours
		uint8_t closeTime;                                // Close time 24 hours
		bool verizonSIM;                                  // Are we using a Verizon SIM?
		uint8_t sensorType;								  // What sensor if any is on this device (0-none, 1-PIR, 2-Pressure, ...)
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

	uint8_t get_stayConnected() const;
	void set_stayConnected(uint8_t value);

	uint8_t get_resetCount() const;
	void set_resetCount(uint8_t value);

	uint8_t get_messageCount() const;
	void set_messageCount(uint8_t value);

	time_t get_lastHookResponse() const;
	void set_lastHookResponse(time_t value);

	time_t get_lastConnection() const;
	void set_lastConnection(time_t value);

	uint16_t get_lastConnectionDuration() const;
	void set_lastConnectionDuration(uint16_t value);

	uint16_t get_frequencyMinutes() const;
	void set_frequencyMinutes(uint16_t value);

	uint16_t get_updatedFrequencyMinutes() const;
	void set_updatedFrequencyMinutes(uint16_t value);

	uint8_t get_alertCodeGateway() const;
	void set_alertCodeGateway(uint8_t value);

	time_t get_alertTimestampGateway() const;
	void set_alertTimestampGateway(time_t value);

	uint8_t get_openTime() const;
	void set_openTime(uint8_t value);

	uint8_t get_closeTime() const;
	void set_closeTime(uint8_t value);

	bool get_verizonSIM() const;
	void set_verizonSIM(bool value);

	uint8_t get_sensorType() const;
	void set_sensorType(uint8_t value);

	uint16_t get_RSSI() const;
	void set_RSSI(uint16_t value);

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
		uint8_t nodeNumber;                              // The nodeNumber of the device providing the current data 
		uint8_t tempNodeNumber;                           // Used when an unconfigured node joins the network
		uint8_t internalTempC;                            // Enclosure temperature in degrees C
		double stateOfCharge;                             // Battery charge level
		uint8_t batteryState;                             // Stores the current battery state (charging, discharging, etc)
		uint8_t resetCount;								  // This is the number of resets for the node publishing data
		uint16_t RSSI;                                    // Latest LoRA signal strength value from the Node
		uint8_t messageCount;                            // What message are we on
		uint8_t successCount;							  // How many attempts have been successful - from the node
		time_t lastCountTime;                             // When did we last record a count
		uint16_t hourlyCount;                             // Current Hourly Count
		uint16_t dailyCount;                              // Current Daily Count
		uint8_t alertCodeNode;                            // Alert code from node
		time_t alertTimestampNode;                 	      // Timestamp of alert
		bool openHours; 								  // Will set to true or false based on time of dat
		uint8_t sensorType;								  // What is the sensor type of the node sending current data
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

	uint8_t get_internalTempC() const ;
	void set_internalTempC(uint8_t value);

	double get_stateOfCharge() const;
	void set_stateOfCharge(double value);

	uint8_t get_batteryState() const;
	void set_batteryState(uint8_t value);

	uint8_t get_resetCount() const;
	void set_resetCount(uint8_t value);

	time_t get_lastSampleTime() const;
	void set_lastSampleTime(time_t value);

	uint16_t get_RSSI() const;
	void set_RSSI(uint16_t value);

	uint8_t get_messageCount() const;
	void set_messageCount(uint8_t value);

	uint8_t get_successCount() const;
	void set_successCount(uint8_t value);

	time_t get_lastCountTime() const;
	void set_lastCountTime(time_t value);

	uint16_t get_hourlyCount() const;
	void set_hourlyCount(uint16_t value);

	uint16_t get_dailyCount() const;
	void set_dailyCount(uint16_t value);

	uint8_t get_alertCodeNode() const;
	void set_alertCodeNode(uint8_t value);

	time_t get_alertTimestampNode() const;
	void set_alertTimestampNode(time_t value);

	bool get_openHours() const;
	void set_openHours(bool value);

	uint8_t get_sensorType() const;
	void set_sensorType(uint8_t value);


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
	static const uint32_t CURRENT_DATA_MAGIC = 0x20a99e74;
	static const uint16_t CURRENT_DATA_VERSION = 1;
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
		char nodeIDJson[1024];                             // JSON string that stores the nodeID data
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
	static const uint32_t NODEID_DATA_MAGIC = 0x20a99e60;
	static const uint16_t NODEID_DATA_VERSION = 2;

};


#endif  /* __MYPERSISTENTDATA_H */
