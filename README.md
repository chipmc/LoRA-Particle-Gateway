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

The gateway controls the nodes via alerts.  These are intended to allow for all the configuration of the nodes so a new node can get all the needed configuration through interacting with the gateway.  

The Alert Codes and their actions are key to this process:

Alert Code 0 - No alerts - Nominal State

Alert Code 1 - Unconfigured Node
    - Unconfigured nodes may be: missing a proper node number, space, placement, or multiEntrance designation, not having a valid uniqueID. or not knowing the time - or all of the above!
    - Alert code 1 prompts the node to contact the gateway using a "Join command" which will remedy all three issues above.
    - Part of the configuration process, the counts from the node are assigned to a "space" (could be a room, entrance, field, court) so we know where to send data / counts / occupancy on the back end. Their mounting variables (placement, multi, etc.) are also set from the node Join payload. 
    - Mounting variables set through the Join payload are initially assigned to a node by passing them, in addition to the node's uniqueID, to the mountConfig Particle Function. TODO:: set this up with the FleetManager queued-updates system.
    - Alert code 1 is assigned as part of a join request acknowledgement and the space is assocaited with the uniqueID (along with sensor type, placement, multi, etc.)

Alert Code 2 - Change the frequency of responses
    - This alert code tells the node that it  



Payload Assignments for Data Report by Sensor Type

| Type  | Payload1   | Payload2   | Payload3   | Payload4   | Payload5   | Payload6   | Payload7   | Payload8   |
| ---------- | ----------- | ----------- | ----------- | ----------- | ----------- | ----------- | ----------- | ----------- | 
| Counter | Gross-H | Gross-L | Net - H | Net - L | TBD | TBD | TBD | TBD |
| Occupancy | Hourly - H | Hourly - L | Daily - H | Daily - L | Space | Placement | Multi | TBD | 
| Sensor | Data1 - H | Data1 - L | Data2 - H | Data2 - L | Data3 - H | Data3 - L | Data4 - H | Data4 - L | 

Payload Assignments for Join Request by Sensor Type

| Type  | Payload1   | Payload2   | Payload3   | Payload4   |
| ---------- | ----------- | ----------- | ----------- | ----------- |
| Counter | 2-Way | TBD | TBD | TBD |
| Occupancy | Space | Placement | Multi | TBD |
| Sensor | Space | Placement | TBD | TBD |