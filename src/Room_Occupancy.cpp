#include "Room_Occupancy.h"
#include "JsonDataManager.h"

Room_Occupancy *Room_Occupancy::_instance;
uint16_t roomGrossArray[64];             // Declared as extern in the header file, defined as an actual array here.

// [static]
Room_Occupancy &Room_Occupancy::instance() {
  if (!_instance) {
    _instance = new Room_Occupancy();
  }
  return *_instance;
}

Room_Occupancy::Room_Occupancy() {
}

Room_Occupancy::~Room_Occupancy() {
}

void Room_Occupancy::setup() {
  // Code to run at setup here
}

void Room_Occupancy::loop() {
  // Put your code to run during the application thread loop here
}

bool Room_Occupancy::resetAllCounts() {
  return JsonDataManager::instance().resetOccupancyCounts();                                                       
}

bool Room_Occupancy::resetNetCounts() {
  return JsonDataManager::instance().resetOccupancyNetCounts();                                                       
}

bool Room_Occupancy::setOccupancyNetForNode(int nodeNumber, int newOccupancyNet) {
  return JsonDataManager::instance().setOccupancyNetForNode(nodeNumber, newOccupancyNet);                                                       
}

int Room_Occupancy::getRoomNet(int space) {
  return JsonDataManager::instance().getOccupancyNetBySpace(space);                                                       
}

int Room_Occupancy::getRoomGross(int space) {
  return JsonDataManager::instance().getOccupancyGrossBySpace(space);                                                                 
}