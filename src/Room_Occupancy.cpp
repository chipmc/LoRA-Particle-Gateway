#include "Room_Occupancy.h"
#include "LoRA_Functions.h"

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

bool Room_Occupancy::resetRoomCounts() {
    return LoRA_Functions::instance().resetOccupancyCounts();                                                       
}

int16_t Room_Occupancy::getRoomNet(int space) {
  return LoRA_Functions::instance().getOccupancyNetBySpace(space);                                                       
}

int16_t Room_Occupancy::getRoomGross(int space) {
  return LoRA_Functions::instance().getOccupancyGrossBySpace(space);                                                                 
}