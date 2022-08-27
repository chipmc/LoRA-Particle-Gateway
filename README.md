# LoRA-Particle-Gateway

This is an implementation of Jeff Skarda's mesh extension to the Radiohead RF-95 Library.

## Initial Implementation - Adding LoRA to Cellular for more complete coverage!

The itent of this project is to show how data collected from remote LoRA nodes can be relayed to a server and on to the internet using a cellular connection

To get Started, you will need to setup a couple devices:
* Server(s) using a Particle Boron which has an RF-95 Module attached.
* Client(s) which can be an inexpensive microcontroller with the RF-95 module and a sensor

To make this code easier to maintain, I am breaking out the main c++ file into the following:
* device_pinout - this is the file where I will capture the pinout between the uC and the sensors and initialize the pins and main system functions
* particle_fn - Particle-specific implementation definitions and initializations like Particle variables, functions and system settings
* storage_objects - By defining system and current objects, I can easily share data and implement persistent storage - in my case FRAM
* take_measurements - Most applications involve loading drivers and accessing data from sensors - all this is done here
* LoRA_Functions - In this application, we will use a LoRA modem and the Radiohead libraries (with Jeff's extensions) all LoRA stuff is in this one place
* LoRA_Particle_Gateway - This is the top-level of the structure and where all the modules come together to realize the progam's intent:


With this program, we will collect data from remote nodes connected via LoRA and relay that data to Particle via the Publish function.  
From there, the Particle Integrations will send this data to its final destination via a Webhook.  The idea of breaking out this code this way
is to allow you to easily swap out - sensors, the platform, the persistent storage media, the connectivity and still reuse code from this effort.
Over time, these elements will mature and become building blocks for whatever comes next (your imagination here).

I hope this is helpful and please let me know if you have comments / suggestions as I am always looking for better ideas.

Chip

