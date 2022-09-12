#include "Particle.h"
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
    sysStatus.load();
}

void sysStatusData::loop() {
    sysStatus.flush(false);
}

uint16_t sysStatusData::get_deviceID() const {
    return getValue<uint16_t>(offsetof(SysData, deviceID));
}

void sysStatusData::set_deviceID(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData, deviceID), value);
}

uint16_t sysStatusData::get_nodeNumber() const {
    return getValue<uint16_t>(offsetof(SysData, nodeNumber));
}

void sysStatusData::set_nodeNumber(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData, nodeNumber), value);
}

uint8_t sysStatusData::get_structuresVersion() const {
    return getValue<uint8_t>(offsetof(SysData, structuresVersion));
}

void sysStatusData::set_structuresVersion(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, structuresVersion), value);
}

uint8_t sysStatusData::get_firmwareRelease() const {
    return getValue<uint8_t>(offsetof(SysData, firmwareRelease));
}

void sysStatusData::set_firmwareRelease(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, firmwareRelease), value);
}

bool sysStatusData::get_solarPowerMode() const {
    return getValue<bool>(offsetof(SysData, solarPowerMode));
}

void sysStatusData::set_solarPowerMode(bool value) {
    setValue<bool>(offsetof(SysData, solarPowerMode), value);
}

bool sysStatusData::get_lowPowerMode() const {
    return getValue<bool>(offsetof(SysData, lowPowerMode));
}

void sysStatusData::set_lowPowerMode(bool value) {
    setValue<bool>(offsetof(SysData, lowPowerMode), value);
}

uint8_t sysStatusData::get_resetCount() const {
    return getValue<uint8_t>(offsetof(SysData, resetCount));
}

void sysStatusData::set_resetCount(uint8_t value) {
    setValue<uint8_t>(offsetof(SysData, resetCount), value);
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

uint16_t sysStatusData::get_nextReportSeconds() const {
    return getValue<uint16_t>(offsetof(SysData,nextReportSeconds ));
}

void sysStatusData::set_nextReportSeconds(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData, nextReportSeconds), value);
}

uint16_t sysStatusData::get_frequencyMinutes() const {
    return getValue<uint16_t>(offsetof(SysData,frequencyMinutes));
}

void sysStatusData::set_frequencyMinutes(uint16_t value) {
    setValue<uint16_t>(offsetof(SysData, frequencyMinutes), value);
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

bool sysStatusData::get_sensorType() const {
    return getValue<bool>(offsetof(SysData, sensorType));
}

void sysStatusData::set_sensorType(bool value) {
    setValue<bool>(offsetof(SysData, sensorType), value);
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

bool sysStatusData::get_verizonSIM() const {
    return getValue<bool>(offsetof(SysData, verizonSIM));
}

void sysStatusData::set_verizonSIM(bool value) {
    setValue<bool>(offsetof(SysData, verizonSIM), value);
}


// *****************  Current Status Storage Object *******************
//
// ********************************************************************

currentStatusData *currentStatusData::_instance;

// [static]
currentStatusData &currentStatusData::instance() {
    if (!_instance) {
        _instance = new currentStatusData();
    }
    return *_instance;
}

currentStatusData::currentStatusData() : StorageHelperRK::PersistentDataFRAM(::fram, 0, &currentData.currentHeader, sizeof(CurrentData), CURRENT_DATA_MAGIC, CURRENT_DATA_VERSION) {
};

currentStatusData::~currentStatusData() {
}

void currentStatusData::setup() {
    fram.begin();
    sysStatus.load();
}

void currentStatusData::loop() {
    sysStatus.flush(false);
}


uint16_t currentStatusData::get_deviceID() const {
    return getValue<uint16_t>(offsetof(CurrentData, deviceID));
}

void currentStatusData::set_deviceID(uint16_t value) {
    setValue<uint16_t>(offsetof(CurrentData, deviceID), value);
}

uint16_t currentStatusData::get_nodeNumber() const {
    return getValue<uint16_t>(offsetof(CurrentData, nodeNumber));
}

void currentStatusData::set_nodeNumber(uint16_t value) {
    setValue<uint16_t>(offsetof(CurrentData, nodeNumber), value);
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

time_t currentStatusData::get_lastSampleTime() const {
    return getValue<time_t>(offsetof(CurrentData, lastSampleTime));
}

void currentStatusData::set_lastSampleTime(time_t value) {
    setValue<time_t>(offsetof(CurrentData, lastSampleTime), value);
}

uint16_t currentStatusData::get_RSSI() const {
    return getValue<uint16_t>(offsetof(CurrentData, RSSI));
}

void currentStatusData::set_RSSI(uint16_t value) {
    setValue<uint16_t>(offsetof(CurrentData, RSSI), value);
}

uint8_t currentStatusData::get_messageNumber() const {
    return getValue<uint8_t>(offsetof(CurrentData, messageNumber));
}

void currentStatusData::set_messageNumber(uint8_t value) {
    setValue<uint8_t>(offsetof(CurrentData, messageNumber), value);
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

void currentStatusData::logData(const char *msg) {
    Log.info("Current Structure values - %d, %d, %d, %4.2f", currentData.deviceID, currentData.nodeNumber, currentData.internalTempC, currentData.stateOfCharge);
}

