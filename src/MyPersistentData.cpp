#include "Particle.h"
#include "MB85RC256V-FRAM-RK.h"
#include "StorageHelperRK.h"
#include "MyPersistentData.h"
#include <stack>
#include <cstring>


MB85RC64 fram(Wire, 0);
// We use the 64kbit part so we have 8k bytes of storage
// SysStatus Object - starts at 0
// Current Object - starts at 100
// Node Object - starts at 200

// *******************  SysStatus Storage Object **********************
//
// ********************************************************************

sysStatusData *sysStatusData::_instance;

// [static]
sysStatusData &sysStatusData::instance() {
    if (!_instance) {
        _instance = new sysStatusData();
    }
    return *_instance;
}

sysStatusData::sysStatusData() : StorageHelperRK::PersistentDataFRAM(::fram, 0, &sysData.sysHeader, sizeof(SysData), SYS_DATA_MAGIC, SYS_DATA_VERSION) {

};

sysStatusData::~sysStatusData() {
}

void sysStatusData::setup() {
    fram.begin();
    sysStatus
    //  .withLogData(true)
        .withSaveDelayMs(500)
        .load();

    // Log.info("sizeof(SysData): %u", sizeof(SysData));
    sysStatus.set_tokenCore(random(1,255));
    Log.info("Token Core is %d", sysStatus.get_tokenCore());
}

void sysStatusData::loop() {
    sysStatus.flush(false);
}

bool sysStatusData::validate(size_t dataSize) {
    bool valid = PersistentDataFRAM::validate(dataSize);
    if (valid) {
        // If test1 < 0 or test1 > 100, then the data is invalid
        if (sysStatus.get_openTime() > 12 || sysStatus.get_closeTime() <12) {
            Log.info("data not valid openTime=%d and closeTime=%d", sysStatus.get_openTime(), sysStatus.get_closeTime());
            valid = false;
        }
        else if (sysStatus.get_frequencyMinutes() <=0 || sysStatus.get_frequencyMinutes() > 60) {
            Log.info("data not valid frequency minutes =%d", sysStatus.get_frequencyMinutes());
            valid = false;
        }
        else if (sysStatus.get_nodeNumber() != 0) {
            Log.info("data not valid node number =%d", sysStatus.get_nodeNumber());
            valid = false;
        }
    }
    if (!valid) Log.info("sysStatus data is %s",(valid) ? "valid": "not valid");
    return valid;
}

void sysStatusData::initialize() {
    PersistentDataFRAM::initialize();

    Log.info("data initialized");

    // Initialize the default value to 10 if the structure is reinitialized.
    // Be careful doing this, because when MyData is extended to add new fields,
    // the initialize method is not called! This is only called when first
    // initialized.
    sysStatus.set_nodeNumber(0);                     // Default for a Gateway
    sysStatus.set_structuresVersion(1);
    sysStatus.set_magicNumber(27617);
    sysStatus.set_connectivityMode(0);
    sysStatus.set_resetCount(0);
    sysStatus.set_messageCount(0);
    sysStatus.set_lastHookResponse(0);
    sysStatus.set_frequencyMinutes(60);
    sysStatus.set_updatedFrequencyMinutes(0);
    sysStatus.set_alertCodeGateway(0);
    sysStatus.set_alertTimestampGateway(0);
    sysStatus.set_openTime(6);
    sysStatus.set_closeTime(22);
    sysStatus.set_breakTime(14);   // set to 2pm by default
    sysStatus.set_breakLengthMinutes(30);   // set to 30 minutes by default

    // If you manually update fields here, be sure to update the hash
    updateHash();
}

uint8_t sysStatusData::get_nodeNumber() const {
    return getValue<uint8_t>(offsetof(SysData, nodeNumber));
}

void sysStatusData::set_nodeNumber(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, nodeNumber), value);
}

uint8_t sysStatusData::get_structuresVersion() const {
    return getValue<uint8_t>(offsetof(SysData, structuresVersion));
}

void sysStatusData::set_structuresVersion(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, structuresVersion), value);
}

uint16_t sysStatusData::get_magicNumber() const {
    return getValue<uint16_t>(offsetof(SysData, magicNumber));
}

void sysStatusData::set_magicNumber(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData, magicNumber), value);
}

uint8_t sysStatusData::get_connectivityMode() const {
    return getValue<uint8_t>(offsetof(SysData, connectivityMode));
}

void sysStatusData::set_connectivityMode(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, connectivityMode), value);
}

uint8_t sysStatusData::get_resetCount() const {
    return getValue<uint8_t>(offsetof(SysData, resetCount));
}

void sysStatusData::set_resetCount(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, resetCount), value);
}

uint16_t sysStatusData::get_messageCount() const {
    return getValue<uint16_t>(offsetof(SysData, messageCount));
}

void sysStatusData::set_messageCount(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData, messageCount), value);
}

time_t sysStatusData::get_lastHookResponse() const {
    return getValue<time_t>(offsetof(SysData, lastHookResponse));
}

void sysStatusData::set_lastHookResponse(time_t value) {
    setValue<time_t>(offsetof(SysData, lastHookResponse), value);
}

time_t sysStatusData::get_lastConnection() const {
    return getValue<time_t>(offsetof(SysData, lastConnection));
}

void sysStatusData::set_lastConnection(time_t value) {
    setValue<time_t>(offsetof(SysData, lastConnection), value);
}

uint16_t sysStatusData::get_lastConnectionDuration() const {
    return getValue<uint16_t>(offsetof(SysData,lastConnectionDuration));
}

void sysStatusData::set_lastConnectionDuration(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData,lastConnectionDuration), value);
}

uint16_t sysStatusData::get_frequencyMinutes() const {
    return getValue<uint16_t>(offsetof(SysData,frequencyMinutes));
}

void sysStatusData::set_frequencyMinutes(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData, frequencyMinutes), value);
}

uint16_t sysStatusData::get_updatedFrequencyMinutes() const {
    return getValue<uint16_t>(offsetof(SysData,updatedFrequencyMinutes));
}

void sysStatusData::set_updatedFrequencyMinutes(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData, updatedFrequencyMinutes), value);
}

uint8_t sysStatusData::get_alertCodeGateway() const {
    return getValue<uint8_t>(offsetof(SysData, alertCodeGateway));
}

void sysStatusData::set_alertCodeGateway(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, alertCodeGateway), value);
}

time_t sysStatusData::get_alertTimestampGateway() const {
    return getValue<time_t>(offsetof(SysData, alertTimestampGateway));
}

void sysStatusData::set_alertTimestampGateway(time_t value) {
    setValue<time_t>(offsetof(SysData, alertTimestampGateway), value);
}

uint8_t sysStatusData::get_openTime() const {
    return getValue<uint8_t>(offsetof(SysData, openTime));
}

void sysStatusData::set_openTime(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, openTime), value);
}

uint8_t sysStatusData::get_closeTime() const {
    return getValue<uint8_t>(offsetof(SysData, closeTime));
}

void sysStatusData::set_closeTime(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, closeTime), value);
}

uint8_t sysStatusData::get_breakTime() const {
    return getValue<uint8_t>(offsetof(SysData, breakTime));
}

void sysStatusData::set_breakTime(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, breakTime), value);
}

uint8_t sysStatusData::get_breakLengthMinutes() const {
    return getValue<uint8_t>(offsetof(SysData, breakLengthMinutes));
}

void sysStatusData::set_breakLengthMinutes(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, breakLengthMinutes), value);
}

uint8_t sysStatusData::get_tokenCore() const {
    return getValue<uint8_t>(offsetof(SysData, tokenCore));
}

void sysStatusData::set_tokenCore(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, tokenCore), value);
}

// *****************  Current Status Storage Object *******************
// Offset of 100 bytes - make room for SysStatus
// ********************************************************************

currentStatusData *currentStatusData::_instance;

// [static]
currentStatusData &currentStatusData::instance() {
    if (!_instance) {
        _instance = new currentStatusData();
    }
    return *_instance;
}

currentStatusData::currentStatusData() : StorageHelperRK::PersistentDataFRAM(::fram, 100, &currentData.currentHeader, sizeof(CurrentData), CURRENT_DATA_MAGIC, CURRENT_DATA_VERSION) {
};

currentStatusData::~currentStatusData() {
}

void currentStatusData::setup() {
    fram.begin();

    current
    //    .withLogData(true)
        .withSaveDelayMs(500)
        .load();

    // Log.info("sizeof(CurrentData): %u", sizeof(CurrentData));
}

void currentStatusData::loop() {
    current.flush(false);
}

bool currentStatusData::validate(size_t dataSize) {
    bool valid = PersistentDataFRAM::validate(dataSize);
    if (!valid) Log.info("current data is %s",(valid) ? "valid": "not valid");
    return valid;
}

void currentStatusData::initialize() {
    PersistentDataFRAM::initialize();
    Log.info("Current Data Initialized");
    // If you manually update fields here, be sure to update the hash
    updateHash();
}

uint8_t currentStatusData::get_nodeNumber() const {
    return getValue<uint8_t>(offsetof(CurrentData, nodeNumber));
}

void currentStatusData::set_nodeNumber(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, nodeNumber), value);
}

uint8_t currentStatusData::get_tempNodeNumber() const {
    return getValue<uint8_t>(offsetof(CurrentData, tempNodeNumber));
}

void currentStatusData::set_tempNodeNumber(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, tempNodeNumber), value);
}

uint16_t currentStatusData::get_token() const {
    return getValue<uint16_t>(offsetof(CurrentData, token));
}

void currentStatusData::set_token(uint16_t value) {
    setValue<uint16_t>(offsetof(CurrentData, token), value);
}

uint8_t currentStatusData::get_sensorType() const {
    return getValue<uint8_t>(offsetof(CurrentData, sensorType));
}

void currentStatusData::set_sensorType(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, sensorType), value);
}

uint32_t currentStatusData::get_uniqueID() const {
    return getValue<uint32_t>(offsetof(CurrentData, uniqueID));
}

void currentStatusData::set_uniqueID(uint32_t value) {
    setValue<uint32_t>(offsetof(CurrentData, uniqueID), value);
}

uint8_t currentStatusData::get_payload1() const {
    return getValue<uint8_t>(offsetof(CurrentData, payload1));
}

void currentStatusData::set_payload1(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, payload1), value);
}   

uint8_t currentStatusData::get_payload2() const {
    return getValue<uint8_t>(offsetof(CurrentData, payload2));
}

void currentStatusData::set_payload2(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, payload2), value);
}   

uint8_t currentStatusData::get_payload3() const {
    return getValue<uint8_t>(offsetof(CurrentData, payload3));
}

void currentStatusData::set_payload3(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, payload3), value);
}

uint8_t currentStatusData::get_payload4() const {
    return getValue<uint8_t>(offsetof(CurrentData, payload4));
}

void currentStatusData::set_payload4(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, payload4), value);
}   

uint8_t currentStatusData::get_payload5() const {
    return getValue<uint8_t>(offsetof(CurrentData, payload5));
}

void currentStatusData::set_payload5(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, payload5), value);
}

uint8_t currentStatusData::get_payload6() const {
    return getValue<uint8_t>(offsetof(CurrentData, payload6));
}

void currentStatusData::set_payload6(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, payload6), value);
}

uint8_t currentStatusData::get_payload7() const {
    return getValue<uint8_t>(offsetof(CurrentData, payload7));
}

void currentStatusData::set_payload7(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, payload7), value);
}

uint8_t currentStatusData::get_payload8() const {
    return getValue<uint8_t>(offsetof(CurrentData, payload8));
}

void currentStatusData::set_payload8(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, payload8), value);
}

uint8_t currentStatusData::get_internalTempC() const {
    return getValue<uint8_t>(offsetof(CurrentData, internalTempC));
}

void currentStatusData::set_internalTempC(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, internalTempC), value);
}

int8_t currentStatusData::get_stateOfCharge() const {
    return getValue<int8_t>(offsetof(CurrentData, stateOfCharge));
}

void currentStatusData::set_stateOfCharge(int8_t value) {
    setValue<int8_t>(offsetof(CurrentData, stateOfCharge), value);
}

uint8_t currentStatusData::get_batteryState() const {
    return getValue<uint8_t>(offsetof(CurrentData, batteryState));
}

void currentStatusData::set_batteryState(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, batteryState), value);
}

uint8_t currentStatusData::get_resetCount() const {
    return getValue<uint8_t>(offsetof(CurrentData, resetCount));
}

void currentStatusData::set_resetCount(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, resetCount), value);
}

int16_t currentStatusData::get_RSSI() const {
    return getValue<int16_t>(offsetof(CurrentData, RSSI));
}

void currentStatusData::set_RSSI(int16_t value) {
    setValue<int16_t>(offsetof(CurrentData, RSSI), value);
}

int16_t currentStatusData::get_SNR() const {
    return getValue<int16_t>(offsetof(CurrentData, SNR));
}

void currentStatusData::set_SNR(int16_t value) {
    setValue<int16_t>(offsetof(CurrentData, SNR), value);
}

uint8_t currentStatusData::get_alertCodeNode() const {
    return getValue<uint8_t>(offsetof(CurrentData, alertCodeNode));
}

void currentStatusData::set_alertCodeNode(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, alertCodeNode), value);
}

uint8_t currentStatusData::get_openHours() const {
    return getValue<uint8_t>(offsetof(CurrentData, openHours));
}

void currentStatusData::set_openHours(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, openHours), value);
}

byte currentStatusData::get_onBreak() const {
    return getValue<byte>(offsetof(CurrentData, openHours));
}

void currentStatusData::set_onBreak(byte value) {
    setValue<byte>(offsetof(CurrentData, openHours), value);
}

uint8_t currentStatusData::get_hops() const {
    return getValue<uint8_t>(offsetof(CurrentData, hops));
}

void currentStatusData::set_hops(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, hops), value);
}

uint8_t currentStatusData::get_retryCount() const {
    return getValue<uint8_t>(offsetof(CurrentData, retryCount));
}

void currentStatusData::set_retryCount(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, retryCount), value);
}

uint8_t currentStatusData::get_retransmissionDelay() const {
    return getValue<uint8_t>(offsetof(CurrentData, retransmissionDelay));
}

void currentStatusData::set_retransmissionDelay(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, retransmissionDelay), value);
}



// *******************  nodeID Storage Object **********************
//
// ******************** Offset of 200         **********************

nodeIDData *nodeIDData::_instance;

// [static]
nodeIDData &nodeIDData::instance() {
    if (!_instance) {
        _instance = new nodeIDData();
    }
    return *_instance;
}

nodeIDData::nodeIDData() : StorageHelperRK::PersistentDataFRAM(::fram, 200, &nodeData.nodeHeader, sizeof(NodeData), NODEID_DATA_MAGIC, NODEID_DATA_VERSION) {

};

nodeIDData::~nodeIDData() {
}

void nodeIDData::setup() {
    fram.begin();

    nodeDatabase
    //    .withLogData(true)
        .withSaveDelayMs(500)
        .load();

    // Log.info("sizeof(NodeData): %u", sizeof(NodeData)); 
}

void nodeIDData::loop() {
    nodeDatabase.flush(false);
}

void nodeIDData::resetNodeIDs() {
    String blank = "{\"nodes\":[]}";
    Log.info("Resettig NodeID config to: %s", blank.c_str());
    nodeDatabase.set_nodeIDJson(blank);
    nodeDatabase.flush(true);
    Log.info("NodeID data is now %s", nodeDatabase.get_nodeIDJson().c_str());
}

bool nodeIDData::validate(size_t dataSize) {
    bool valid = PersistentDataFRAM::validate(dataSize);
    if (!valid) Log.info("nodeID data is %s",(valid) ? "valid": "not valid");
    return valid;
}

void nodeIDData::initialize() {

    Log.info("Erasing FRAM region");
    for (unsigned int i=0; i < sizeof(NodeData); i++) {
        fram.writeData(i+200,(uint8_t *)0xFF,2);
    }

    Log.info("Initializing data");
    PersistentDataFRAM::initialize();
    nodeIDData::resetNodeIDs();
    updateHash();                                       // If you manually update fields here, be sure to update the hash
}


String nodeIDData::get_nodeIDJson() const {
	String result;
	getValueString(offsetof(NodeData, nodeIDJson), sizeof(NodeData::nodeIDJson), result);
	return result;
}

bool nodeIDData::set_nodeIDJson(const char* str) {

    // Set the cleaned JSON value
    bool result = setValueString(offsetof(NodeData, nodeIDJson), sizeof(NodeData::nodeIDJson), str);

    if (result && Particle.connected()) {
        const size_t maxChunkSize = 622; // max report size
        size_t messageLength = strlen(str);

        size_t offset = 0;
        while (offset < messageLength) {
            // Calculate chunk size for the current iteration
            size_t chunkSize = std::min(maxChunkSize, messageLength - offset);

            // Create a buffer for the current chunk
            char chunk[maxChunkSize + 1]; // +1 for null terminator
            snprintf(chunk, sizeof(chunk), "%.*s", static_cast<int>(chunkSize), str + offset);

            // Publish the current chunk
            Particle.publish("Node Database Updated:", chunk, PRIVATE);

            // Move to the next chunk
            offset += chunkSize;
        }
    }

    return result;
}