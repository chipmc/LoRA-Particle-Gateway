#include "Room_Occupancy.h"

Room_Occupancy *Room_Occupancy::_instance;

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

void resetEverything() {
  Log.info("Resetting the Room Occupancy Array");
  for (uint16_t i=0; i < 10; i++) {
    roomCountArray[i][0] = roomCountArray[i][1] = 0;
  }
}

bool setRoomCounts() {  // This is set for the current node - need to generalize later??
  bool response = true;
  int16_t currentGross = current.get_payload1() <<8 | current.get_payload2();
  int16_t currentNet = current.get_payload3() <<8 | current.get_payload4();

  if (current.get_nodeNumber() == 255) {
    Log.info("Error- Unconfigured ndoe");
    return 255;
  }

  if (!current.get_payload7())  {                     // Simple case - single node on the room
    if (currentNet < 0) {                             // This is a problem - can't have negative people in the room     
      currentNet = 0;                                 // Set the room net to zero
      Log.info("Received a negative occupancy - resetting");
      current.set_alertCodeNode(3);
      current.set_payload3(0);                        // Node tried to go negative - reset - false response should trigger message back to node
      current.set_payload4(0);                        // Resets current net to zero - alert will send the low byte as the alert context
      roomCountArray[current.get_payload5()-1][1] = 0; // Set the room net value
      response = false;
    }
    else roomCountArray[current.get_payload5()-1][1] = currentNet;        // Set the room net value
    roomCountArray[current.get_payload5()-1][0] = currentGross;           // Set the room gross value - always the case so outside conditional
  }
  else {                                                                  // Little more complex - This is a multi sensor room current net counts can be zero
    if (roomCountArray[current.get_payload5()-1][1] + currentNet < 0) {   // Just like above, test for less than zero
      currentNet = -1 * roomCountArray[current.get_payload5()-1][1];      // Written for clarity
      Log.info("Received a negative occupancy - resetting");
      current.set_alertCodeNode(3);
      current.set_payload3(highByte(currentNet));                         // Node tried to go negative - reset - false response should trigger message back to node
      current.set_payload4(lowByte(currentNet));                          // Resets current net to zero - alert will send the low byte as the alert context
      roomCountArray[current.get_payload5()-1][1] = 0;                    // Set the room net value to zero
      response = false;
    }
    else roomCountArray[current.get_payload5()-1][1] += currentNet;
    roomCountArray[current.get_payload5()-1][0] += currentGross;
  }
  return response;
}

int16_t getRoomNet(int space) {

  return (roomCountArray[space-1][1]);            // People don't like a space to be zero so they start at one
  
}

int16_t getRoomGross(int space) {

  return (roomCountArray[space-1][0]);            // People don't like a space to be zero so they start at one

  }