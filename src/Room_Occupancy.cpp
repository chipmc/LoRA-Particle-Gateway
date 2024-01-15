#include "Room_Occupancy.h"
#include "LoRA_Functions.h"

Room_Occupancy *Room_Occupancy::_instance;
uint16_t roomCountArray[10][2];             // Declared as extern in the header file, defined as an actual array here.

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

void Room_Occupancy::resetEverything() {
  Log.info("Resetting the Room Occupancy Array");
  for (uint16_t i=0; i < 10; i++) {
    roomCountArray[i][0] = roomCountArray[i][1] = 0;
  }
}

bool Room_Occupancy::setRoomCounts() {  // This is set for the current node
                        /*** TODO:: Read Me! (Alex) - Added a generalization to decipherDataReportGateway. Since it will be called right after setting payload1 - payload8, 
                                                      I like this implementation as it works with the currentData much the same as getJoinPayload and setJoinPayload do. ***/            
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
                                                                                     
      LoRA_Functions::instance().setAlertCode(12, current.get_nodeNumber());         /*** Queue up an alert code with alert context (instead of setting current right here) ***/
      LoRA_Functions::instance().setAlertContext(0, current.get_nodeNumber());       /*** These will be set to current in the Data Acknowledgement message (line 278 and 282 of LoRA_Functions.cpp) ***/
      
      current.set_payload3(0);                        // Node tried to go negative - reset - false response should trigger message back to node
      current.set_payload4(0);                        // Resets current net to zero - alert will send the low byte as the alert context
      roomCountArray[current.get_payload5()][1] = 0; // Set the room net value
      response = false;
    }
    else roomCountArray[current.get_payload5()][1] = currentNet;        // Set the room net value
    roomCountArray[current.get_payload5()][0] = currentGross;           // Set the room gross value - always the case so outside conditional
  }
  else {                                                                  // Little more complex - This is a multi sensor room current net counts can be zero
    if (roomCountArray[current.get_payload5()-1][1] + currentNet < 0) {   // Just like above, test for less than zero
      currentNet = -1 * roomCountArray[current.get_payload5()][1];        // Written for clarity
      Log.info("Received a negative occupancy - resetting");

      LoRA_Functions::instance().setAlertCode(12, current.get_nodeNumber());         /*** Queue up an alert code and alert context (instead of setting current right here) ***/
      LoRA_Functions::instance().setAlertContext(0, current.get_nodeNumber());       /*** These will be set to current in the Data Acknowledgement message (line 278 and 282 of LoRA_Functions.cpp) ***/

      current.set_payload3(highByte(currentNet));                         // Node tried to go negative - reset - false response should trigger message back to node
      current.set_payload4(lowByte(currentNet));                          // Resets current net to zero - alert will send the low byte as the alert context
                                                                              /*** TODO:: Read Me! (Alex) - Can we discuss what you mean/want to do here with "alert will send the low byte as the alert context"? 
                                                                                                            Given my understanding, Line 69(v17.40) would set it to 0, but I'm not sure if this is your intention. ***/

      roomCountArray[current.get_payload5()][1] = 0;                    // Set the room net value to zero
      response = false;
    }
    else roomCountArray[current.get_payload5()][1] += currentNet; 
    roomCountArray[current.get_payload5()][0] += currentGross;
  }
  return response;
}

int16_t Room_Occupancy::getRoomNet(int space) {

  return (roomCountArray[space][1]);            // People don't like a space to be zero so they start at one
                                                /*** (Alex) TODO:: Space is now set using var == 1-64 through particle (v17.4). Particle_Functions.cpp subtracts 1 from the command's value now.
                                                                   Since the space parameter for this function would be internal, we should index here assuming space is 0-63.
                                                                   This allows us to set space = 64 without messing up the payload and makes things more uniform throughout the application. 
                                                                   We are now displaying space on Ubidots as space + 1 and executing the particle command assuming the passed value is space + 1 ***/
}

int16_t Room_Occupancy::getRoomGross(int space) {

  return (roomCountArray[space][0]);            // People don't like a space to be zero so they start at one

}