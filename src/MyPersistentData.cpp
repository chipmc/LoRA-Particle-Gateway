#include "Particle.h"
#include "MB85RC256V-FRAM-RK.h"
#include "StorageHelperRK.h"
#include "MyPersistentData.h"


MB85RC64 fram(Wire, 0);

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
    //    .withLogData(true)
        .withSaveDelayMs(500)
        .load();

    // Log.info("sizeof(SysData): %u", sizeof(SysData));
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

uint8_t sysStatusData::get_messageCount() const {
    return getValue<uint8_t>(offsetof(SysData, messageCount));
}

void sysStatusData::set_messageCount(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, messageCount), value);
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

uint8_t sysStatusData::get_sensorType() const {
    return getValue<uint8_t>(offsetof(SysData, sensorType));
}

void sysStatusData::set_sensorType(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, sensorType), value);
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
    if (valid) {
        if (current.get_hourlyCount() < 0 || current.get_hourlyCount() > 1024 || current.get_hourlyPIRInterrupts() < 0 || current.get_hourlyPIRInterrupts() > 1024) {
            Log.info("current data not valid hourlyCount=%d hourlyPIRInterrupts=%d" , current.get_hourlyCount(), current.get_hourlyPIRInterrupts());
            valid = false;
        }
    }
    if (!valid) Log.info("current data is %s",(valid) ? "valid": "not valid");
    return valid;
}

void currentStatusData::initialize() {
    PersistentDataFRAM::initialize();

    Log.info("Current Data Initialized");

    currentStatusData::resetEverything();

    // If you manually update fields here, be sure to update the hash
    updateHash();
}


void currentStatusData::resetEverything() {                             // The device is waking up in a new day or is a new install
  Log.info("A new day - resetting everything");
  current.set_nodeNumber(11);
  current.set_tempNodeNumber(0);
  current.set_nodeID(0);
  current.set_alertCodeNode(0);
  current.set_alertTimestampNode(0);
  current.set_dailyCount(0);                                            // Reset the counts in FRAM as well
  current.set_hourlyCount(0);
  current.set_dailyPIRInterrupts(0);                                    // Reset the PIR interrupt counts in FRAM as well
  current.set_hourlyPIRInterrupts(0);
  current.set_messageCount(0);
  current.set_successCount(0);
  current.set_lastCountTime(Time.now());
  sysStatus.set_resetCount(0);                                          // Reset the reset count as well
  sysStatus.set_messageCount(0);
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

uint16_t currentStatusData::get_nodeID() const {
    return getValue<uint16_t>(offsetof(CurrentData, nodeID));
}

void currentStatusData::set_nodeID(uint16_t value) {
    setValue<uint16_t>(offsetof(CurrentData, nodeID), value);
}

uint8_t currentStatusData::get_internalTempC() const {
    return getValue<uint8_t>(offsetof(CurrentData, internalTempC));
}

void currentStatusData::set_internalTempC(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, internalTempC), value);
}

double currentStatusData::get_stateOfCharge() const {
    return getValue<double>(offsetof(CurrentData, stateOfCharge));
}

void currentStatusData::set_stateOfCharge(double value) {
    setValue<double>(offsetof(CurrentData, stateOfCharge), value);
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

uint8_t currentStatusData::get_messageCount() const {
    return getValue<uint8_t>(offsetof(CurrentData, messageCount));
}

void currentStatusData::set_messageCount(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, messageCount), value);
}

uint8_t currentStatusData::get_successCount() const {
    return getValue<uint8_t>(offsetof(CurrentData, successCount));
}

void currentStatusData::set_successCount(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, successCount), value);
}

time_t currentStatusData::get_lastCountTime() const {
    return getValue<time_t>(offsetof(CurrentData, lastCountTime));
}

void currentStatusData::set_lastCountTime(time_t value) {
    setValue<time_t>(offsetof(CurrentData, lastCountTime), value);
}

uint16_t currentStatusData::get_hourlyCount() const {
    return getValue<uint16_t>(offsetof(CurrentData, hourlyCount));
}

void currentStatusData::set_hourlyCount(uint16_t value) {
    setValue<uint16_t>(offsetof(CurrentData, hourlyCount), value);
}

uint16_t currentStatusData::get_dailyCount() const {
    return getValue<uint16_t>(offsetof(CurrentData, dailyCount));
}

void currentStatusData::set_dailyCount(uint16_t value) {
    setValue<uint16_t>(offsetof(CurrentData, dailyCount), value);
}

uint16_t currentStatusData::get_hourlyPIRInterrupts() const {
    return getValue<uint16_t>(offsetof(CurrentData, hourlyPIRInterrupts));
}

void currentStatusData::set_hourlyPIRInterrupts(uint16_t value) {
    setValue<uint16_t>(offsetof(CurrentData, hourlyCount), value);
}

uint16_t currentStatusData::get_dailyPIRInterrupts() const {
    return getValue<uint16_t>(offsetof(CurrentData, dailyPIRInterrupts));
}

void currentStatusData::set_dailyPIRInterrupts(uint16_t value) {
    setValue<uint16_t>(offsetof(CurrentData, dailyCount), value);
}

uint8_t currentStatusData::get_alertCodeNode() const {
    return getValue<uint8_t>(offsetof(CurrentData, alertCodeNode));
}

void currentStatusData::set_alertCodeNode(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, alertCodeNode), value);
}

time_t currentStatusData::get_alertTimestampNode() const {
    return getValue<time_t>(offsetof(CurrentData, alertTimestampNode));
}

void currentStatusData::set_alertTimestampNode(time_t value) {
    setValue<time_t>(offsetof(CurrentData, alertTimestampNode), value);
}

bool currentStatusData::get_openHours() const {
    return getValue<bool>(offsetof(CurrentData, openHours));
}

void currentStatusData::set_openHours(bool value) {
    setValue<bool>(offsetof(CurrentData, openHours), value);
}

uint8_t currentStatusData::get_sensorType() const {
    return getValue<uint8_t>(offsetof(CurrentData, sensorType));
}

void currentStatusData::set_sensorType(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, sensorType), value);
}

uint8_t currentStatusData::get_hops() const {
    return getValue<uint8_t>(offsetof(CurrentData, hops));
}

void currentStatusData::set_hops(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, hops), value);
}

uint16_t currentStatusData::get_productVersion() const {
    return getValue<uint16_t>(offsetof(CurrentData, productVersion));
}

void currentStatusData::set_productVersion(uint16_t value) {
    setValue<uint16_t>(offsetof(CurrentData, productVersion), value);
}

float currentStatusData::get_soilVWC() const {
    return getValue<float>(offsetof(CurrentData, soilVWC));
}

void currentStatusData::set_soilVWC(float value) {
    setValue<float>(offsetof(CurrentData,soilVWC), value);
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

bool nodeIDData::set_nodeIDJson(const char *str) {
	return setValueString(offsetof(NodeData, nodeIDJson), sizeof(NodeData::nodeIDJson), str);
}
