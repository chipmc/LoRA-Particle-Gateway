# LoRA-Particle-Gateway

This is an implementation of Jeff Skarda's mesh extension to the Radiohead RF-95 Library.

## Initial Implementation - Adding LoRA to Cellular for more complete coverage!

The itent of this project is to show how data collected from remote LoRA nodes can be relayed to a server and on to the internet using a cellular connection

To get Started, you will need to setup a couple devices:
* Server(s) using a Particle Boron which has an RF-95 Module attached.
* Client(s) which can be an inexpensive microcontroller with the RF-95 module and a sensor

This is v1 of this code which functions as follows:
* It listens for clients pushing data in a pre-determined formay
* Is stores webhook payloads 
* Periodically, it connects to the Particle network and sends the accumulated data

