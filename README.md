# LoRA-Particle-Gateway

This is a lightweight and extensible gateway between LoRA and Cellular networks using the Particle platform.  This work is possible because of the enhancement of the Particle community libraries RF9X-RK and AB1805_RK  

## Initial Implementation - Adding LoRA to Cellular for more complete coverage!

The itent of this project is to show how data collected from remote LoRA nodes can be relayed to a server and on to the internet using a cellular connection.  This is especially useful in three scenarios:
    - There is no cellular signal where you want to place sensor nodes
    - There is no LoRA or LoRAWAN networks where you want to place sensor nodes
    - There is a cellular network but the density of sensor nodes makes it more economical to create a private LoRA network

To get Started, you will need to setup a couple devices:
* Server(s) using a Particle Boron which has an RF-95 Module attached.
* Client(s) which can be an inexpensive microcontroller with the RF-95 module and a sensor

## Modular Program Structure

To make this code easier to maintain, I am breaking out the main c++ file into the following:
* device_pinout - this is the file where I will capture the pinout between the uC and the sensors and initialize the pins and main system functions
* Particle_Functions - Particle-specific implementation definitions and initializations like Particle variables, functions and system settings
* storage_objects - By defining system and current objects, I can easily share data and implement persistent storage - in my case FRAM
* take_measurements - Most applications involve loading drivers and accessing data from sensors - all this is done here
* LoRA_Functions - In this application, we will use a LoRA modem and the Radiohead libraries (with Jeff's extensions) all LoRA stuff is in this one place
* LoRA_Particle_Gateway - This is the top-level of the structure and where all the modules come together to realize the progam's intent.

## Concept of operations

With this program, we will collect data from remote nodes connected via LoRA and relay that data to Particle via the Publish function.  
From there, the Particle Integrations will send this data to its final destination via a Webhook.  The idea of breaking out this code this way
is to allow you to easily swap out - sensors, the platform, the persistent storage media, the connectivity and still reuse code from this effort.
Over time, these elements will mature and become building blocks for whatever comes next (your imagination here).

## Alert Codes

The gateway controls the nodes via alerts and alertContext, a 1 byte unsigned integer that contains metadata to be sent with the alert. These are intended to allow for all the configuration of the nodes so a new node can get all the needed configuration through interacting with the gateway.  

The Alert Codes and their actions are key to this process:

**Alert Code 0 - No alerts - Nominal State** 
* No Alert Context

**Alert Code 1 - Unconfigured Node / Rejoin has been triggered**
* No Alert Context. 
    * Utilizes the JSON node database instead to persist configuration values. 
* Nodes may be: missing a proper node number, sensorType, space, placement, or multiEntrance designation, not having a valid uniqueID. or not knowing the time - or all of the above!
* Prompts the node to contact the gateway using a "Join command" which will remedy all three issues above.
* Alert code 1 is assigned as part of the "type" and "mountConfig" particle functions and the node Join payload triggers a configuration update based on the node's sensorType.
* Part of the configuration process, the counts from the node are assigned to a "space" (could be a room, entrance, field, court) stored in the node Join payload so we know where to send data / counts / occupancy on the back end. Their mounting variables (placement, multi, etc.) are also set from the node Join payload. 
    * Node Join payload contains a payload of 4 bytes compressed into 1 byte. Data in the payload varies by sensorType. See chart for "Join Payload" at the bottom of this README for details.

**Alert Code 2 - Time not Synced (CURRENTLY AN INTERNAL CODE FOR THE NODE)**
* No Alert Context
* If the time is not synced on a node, the node rejoins with Alert 2

**Alert Code 3 - Power cycle the node (CURRENTLY AN INTERNAL CODE FOR THE NODE)**
* No Alert Context
* Currently only used internally on a node when something goes wrong
* Power cycles the device to recover from any errors that occur internally on a node.

**Alert Code 4 - Reinitialize the modem (CURRENTLY AN INTERNAL CODE FOR THE NODE)**
* No Alert Context
* Currently only used internally on a node when something goes wrong
* Used when the node has retried sending enough that it is time to reinitilize the modem

**Alert Code 5 - Resets ALL data of a node**
* No Alert Context
* Resets both system and current data stored on a node

**Alert Code 6 - Resets CURRENT data of a node**
* No Alert Context
* Resets only the current data stored on a node

**Alert Code 7 - Sets the "Zone Mode" of a TOF Occupancy Sensor**
* Alert Context - zoneMode(uint8_t)
* Currently does not check for sensorType before sending the alert and alertContext
* A "Zone Mode" is a predefined SPAD configuration and has options:   
    * 0 = default - zones are 8 SPADS deep and 16 SPADS wide such that they activate all SPADS in the front and back panels.
    * 1 = separated - zones are 6 SPADS deep and 16 SPADS wide such that they activate the front 6 rows of the front panel and back 6 rows of the back panel. (Useful when zones overlap too much in "default" and occupancyState 3 occurs often without 1 or 2 occurring first)
    * 2 = verySeparated - zones are 4 SPADS deep and 16 SPADS wide such that they activate the front 4 rows of the front panel and back 4 rows of the back panel. (Useful when zones overlap too much in "separated" and occupancyState 3 occurs often without 1 or 2 occurring first)
    * 3 = frontFocused - zones are 4 SPADS deep and 16 SPADS wide such that they activate all SPADS in the front panel only, splitting the front panel evenly into 2 zones.
    * 4 = backFocused - zones are 4 SPADS deep and 16 SPADS wide such that they activate all SPADS in the back panel only, splitting the back panel evenly into 2 zones.
    * 5 = thin - zones are 8 SPADS deep and 8 SPADS wide such that they activate a thinner matrix of SPADS, splitting the thinner matrix depth-wise evenly into 2 zones like zoneMode 0's depth.
    * 6 = veryThin - zones are 8 SPADS deep and 4 SPADS wide such that they activate a much thinner matrix of SPADS, splitting the much thinner matrix depth-wise evenly into 2 zones like zoneMode 0's depth.
* See TOF Occupancy Module within Config.h in SeeInsights-LoRA-Node repository for exact configuration specifics.
* More Zone Modes can be defined by adding to the switch statement of TOFSensor.cpp in SeeInsights_LoRA_Node. Be sure to expand the bounds of the zoneMode particle function in LoRA_Particle_Gateway's Particle_Functions.cpp file.

**Alert Code 8 - Sets the "Distance Mode" of a TOF Occupancy Sensor**
* Alert Context - distanceMode(uint8_t)
* Currently does not check for sensorType before sending the alert and alertContext
* Specifies the ranging mode of the VL53L1X TOF Sensor.
* Distance mode can have values 0 (Short/1.3m), 1 (Medium/3m) or 2 (Long/4m)
* Defaults to Long
* Short distance mode can range every 20ms, while Medium and Long can range every 33ms. (TODO:: explore power saving capabilities of slower ranging speeds)

**Alert Code 9 - Sets the "Floor Interference Buffer" of a TOF Occupancy Sensor**
* Alert Context - interferenceBuffer(uint8_t)
* Currently does not check for sensorType before sending the alert and alertContext
* The Floor Interference Buffer is a value from 0-255 in which we reduce the effective distance of a range.
* Raising the Floor Interference Buffer makes calibration restart less frequently and can quiet noise that occurs due to the slight differences in measurements of the same object at the same distance.

**Alert Code 10 - Sets the number of calibration loops for a TOF Occupancy Sensor**
* Alert Context - occupancyCalibrationLoops(uint8_t)
* Currently does not check for sensorType before sending the alert and alertContext
* The number of calibration loops is a value from 0-255 that represents the number of measurements to take (for each zone) during calibration.
* Raising the number of calibration loops makes calibration slower and more accurate.
* Raising the number of calibration loops can quiet down noisy occupancy measurements.
    * The maximum distance that is measured during calibration for each zone is used as the baseline value for the zone.

**Alert Code 11 - Triggers recalibration for a TOF Occupancy Sensor**
* No Alert Context
* Currently does not check for sensorType before sending the alert and alertContext
* Initiates recalibration for a sensor. Often used after setting different sensor configuration values or a new zoneMode.

**Alert Code 12 - Gateway override to occupancyNet value of a node.**
* Alert Context - occupancyNet(int16_t)
* Sets the value of the current net count to the value sent in the alert context on the data acknowledgement.

## Particle Function Commands 

Particle Function Commands are executed through the Particle Console for a device (TODO:: or through the Ubidots dashboard widget frontend). The UniqueID of a node is specified as the value for the "node" key of the command and thus, a node must be present in the JSON database of the gateway before executing them. 

To add a new node to the database, attempt to join to the gateway once by pressing the User button to trigger a data payload to be sent. This will cause the Gateway to store the node in the database and respond with Alert Code 1 to trigger a Join request.

**Reset**
* {"cmd":[{"node":0,"var":"all","fn":"reset"}]} - Resets the Gateway entirely
* {"cmd":[{"node":0,"var":"nodeData","fn":"reset"}]} - Resets the nodeData JSON of the Gateway
* {"cmd":[{"node":*node uniqueID here*,"var":"all","fn":"reset"}]} - Triggers Alert Code 5 for the node with the given uniqueID
* {"cmd":[{"node":*node uniqueID here*,"var":"current","fn":"reset"}]} - Triggers Alert Code 6 for the node with the given uniqueID

**Frequency**
* {"cmd":[{"node":0,"var":2,"fn":"freq"}]} - Change reporting frequency of the Gateway (minutes)
    * var can be any value 2-60 that is divisible by two (minutes)

**Stay**
* {"cmd":[{"node":0,"var":"true","fn":"stay"}]} - Causes the Gateway to stay online
* {"cmd":[{"node":0,"var":"false","fn":"stay"}]} - Returns the Gateway to normal connectivity

**Report**
* {"cmd":[{"node":0,"var":" ","fn":"rpt"}]} - Forces the Gateway to report its nodeID data to Particle

**Opening Hour**
* {"cmd":[{"node":0,"var":"6","fn":"open"}]} - Changes the opening hour of the Gateway
    * var can be any value 0-12

**Closing Hour**
* {"cmd":[{"node":0,"var":"6","fn":"open"}]} - Changes the closing hour of the Gateway
    * var can be any value 13-24

**Break Start Hour**
* {"cmd":[{"node":0,"var":"2","fn":"break"}]} - Changes the break start hour for the Gateway - set to 24 to denote having no break
    * var can be any value 0-24, set to 24 to denote having no break

**Break Length Minutes**
* {"cmd":[{"node":0,"var":"30","fn":"breakLengthMinutes"}]} - Changes the length (in minutes) for the 'break' on the Gateway
    * var can be any value 0-240

**Weekend Break Start Hour**
* {"cmd":[{"node":0,"var":"2","fn":"weekendBreak"}]} - Changes the break start hour for the Gateway - set to 24 to denote having no break
    * var can be any value 0-24, set to 24 to denote having no break

**Weekend Break Length Minutes**
* {"cmd":[{"node":0,"var":"30","fn":"weekendBreakLengthMinutes"}]} - Changes the length (in minutes) for the 'break' on the Gateway
    * var can be any value 0-240

**Power Cycle**
* {"cmd":[{"node":0,"var":"1","fn":"pwr"}]} - Forces a power cycle for the Gateway
    * var can *only* be 1 

**Set Sensor Type for a Node**
* {"cmd":[{"node":*node uniqueID here*,"var":"10","fn":"type"}]} - Sets the "type" variable in the node JSON database and sends Alert Code 1, all for the node with the given uniqueID
    * var must be equal to the device "type", See - https://seeinsights.freshdesk.com/support/solutions/articles/154000101712-sensor-types-and-identifiers
    * var must be between 0 and 29

**Set Mounting Configuration for an Occupancy Node**
* {"cmd":[{"var":["5","true","true"],"fn":"mountConfig","node":*node uniqueID here*}]} - Sets the Join Payload for an Occupancy node in the node JSON database and sends Alert Code 1, all for the node with the given uniqueID
    * var is an array - [INT (space), BOOL (placement), BOOL (multi)]
    * the space value in the array must be a value between 0 and 63 (6 bits)
    * the placement and multi values in the array must be either "true" or "false"

**Set Zone Mode for an Occupancy Node**
* {"cmd":[{"node":*node uniqueID here*, "var":"1","fn":"zoneMode"}]} - Sets the zoneMode for the node with the given uniqueID by sending Alert Code 7 with Alert Context == var
    * var must be 0-4

**Set Distance Mode for an Occupancy Node**
* {"cmd":[{"node":*node uniqueID here*, "var":"2","fn":"distanceMode"}]} - Sets the distanceMode for the node with the given uniqueID by sending Alert Code 8 with Alert Context == var
    * var must be 0-2

**Set Floor Interference Buffer for an Occupancy Node**
* {"cmd":[{"node":*node uniqueID here*, "var":"150","fn":"interferenceBuffer"}]} - Sets the interferenceBuffer for the node with the given uniqueID by sending Alert Code 9 with Alert Context == var
    * var must be 0-2000

**Set Number of Calibration Loops for an Occupancy Node**
* {"cmd":[{"node":*node uniqueID here*, "var":"50","fn":"occupancyCalibrationLoops"}]} - Sets the occupancyCalibrationLoops for the node with the given uniqueID by sending Alert Code 10 with Alert Context == var
    * var must be 0-1000

**Recalibrate an Occupancy Node**
* {"cmd":[{"node":*node uniqueID here*, "var":"true","fn":"recalibrate"}]} - Triggers a recalibration of the TOF Sensor on the node with the given uniqueID by sending Alert Code 11 
    * var must *only* be "true"

**Reset all Room Occupancy**
* {"cmd":[{"node":0, "var":"true","fn":"resetRoomCounts"}]} - Resets the Room Occupancy (net and gross) numbers to 0 for all nodes. 
    * var must *only* be "true"

**Reset a Space**
* {"cmd":[{"node":"0", "var":"5","fn":"resetSpace"}]} - Resets the values in a space and updates ubidots.
    * var must be 1-64

**Set Net Count for an Occupancy Node**
* {"cmd":[{"node":*node uniqueID here*, "var":"5","fn":"setOccupancyNetForNode"}]} - Sets the Net Occupancy number for a node and updates the node's space value on Ubidots
    * var must be an integer value

## Payload Assignments for Data Report by Sensor Type

| Type  | Payload1   | Payload2   | Payload3   | Payload4   | Payload5   | Payload6   | Payload7   | Payload8   |
| ---------- | ----------- | ----------- | ----------- | ----------- | ----------- | ----------- | ----------- | ----------- | 
| Counter | Hourly - H | Hourly - L | Daily - H | Daily - L | TBD | TBD | TBD | TBD |
| Occupancy | Gross-H | Gross-L | Net - H | Net - L | Space | Placement | Multi | Zone Mode | 
| Sensor | Data1 - H | Data1 - L | Data2 - H | Data2 - L | Data3 - H | Data3 - L | Data4 - H | Data4 - L | 

## Payload Assignments for Join Request by Sensor Type (4 values compressed to 1 byte)

| Type  | Payload1   | Payload2   | Payload3   | Payload4   |
| ---------- | ----------- | ----------- | ----------- | ----------- |
| Counter | 2-Way (1 bit) | TBD | TBD | TBD |
| Occupancy | Space (6 bits) | Placement (1 bit) | Multi (1 bit) | TBD |
| Sensor | Space (6 bits)  | Placement (1 bit) | TBD | TBD |

## Additional Data stored in JSON database by Sensor Type

| Type  | JsonData1 (jd1)   | JsonData2(jd2)   |
| ---------- | ----------- | ----------- |
| TBD | TBD | TBD |
| Occupancy | OccupancyNet | OccupancyGross | 
| TBD | TBD | TBD |