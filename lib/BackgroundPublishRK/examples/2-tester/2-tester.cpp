/*
 * Project base_sample
 * Description: Demonstrate basic non-blocking background publishes.
 * Author: Joel Lovinger
 * Date: 08/30/2019
 */

#include "Particle.h"
#include "BackgroundPublishRK.h"

// This log handler enables output of logging messages to the serial port
// Log.info/warn/error messages are preferred to straight Serial.print as they
// can be captured and redirected by various handlers and standard tools.
SerialLogHandler logHandler(Serial, LOG_LEVEL_TRACE);

// System threading is necessary to allow non-blocking behaviors for the user
// application.
SYSTEM_THREAD(ENABLED);
// In AUTOMATIC mode the user application does not run until a cloud connection
// is established. Use SEMI_AUTOMATIC to run setup/loop in order to handle
// even when the cloud connection is unavailable.
// MANUAL and SEMI_AUTOMATIC are identical when system threading is enabled.
SYSTEM_MODE(SEMI_AUTOMATIC);

uint32_t uptime_sec = 0;

// for tracking connect/disconnect duration
uint32_t connect_start_sec = 0;
uint32_t connect_duration_sec = 0;
uint32_t disconnect_start_sec = 0;
uint32_t disconnect_duration_sec = 0;
bool disconnect_publish_requested = false;

// callback for success/failure when publishing a "disconnected" event
void publish_disconnected_cb(bool succeeded,
    const char *event_name,
    const char *event_data,
    const void *event_context)
{
    if(!succeeded)
    {
        // didn't succeed! try again!
        disconnect_publish_requested = true;
    }
    Log.info("%s: %s",
        __FUNCTION__,
        succeeded ? "success" : "failure");
}

// try to publish a "disconnected" event
void publish_disconnected()
{
    bool success;

    // publish WITH_ACK and using callback so it will be retried until success
    Log.info("trying to publish \"disconnect\"");

    // Assume publish will succeed and mark as no longer requesting a publish.
    //
    // Important to set the request to false FIRST as once the publish is
    // started the background publish thread can preempt, try the publish,
    // fail, and trigger the callback which requests the retry by setting
    // the request back to true in order to retry.
    // If setting the request to false after the publish on succes would result
    // in a race condition where the retry could be canceled.
    disconnect_publish_requested = false;

    success = BackgroundPublishRK::instance().publish("disconnect",
        String::format("disconnect_duration_sec=%lu", disconnect_duration_sec),
        PRIVATE | WITH_ACK,
        publish_disconnected_cb);
    if(!success)
    {
        Log.warn("failed to initiate publish \"disconnect\"");
        disconnect_publish_requested = true; // didn't succeed! try again!
    }
}

// handler for cloud_status system events
void cloud_status_handler(system_event_t event, int data)
{
    if(data == cloud_status_connected)
    {
        connect_start_sec = System.uptime();
        disconnect_duration_sec = connect_start_sec - disconnect_start_sec;
        disconnect_publish_requested = true;
        Log.info("%s: Particle cloud connected!", Time.format().c_str());
    }
    else if(data == cloud_status_disconnected)
    {
        disconnect_start_sec = System.uptime();
        connect_duration_sec = disconnect_start_sec - connect_start_sec;
        Log.error("%s: Particle cloud disconnected!", Time.format().c_str());
    }
}

// setup() runs once, when the device is first turned on.
void setup()
{
    BackgroundPublishRK::instance().start();

    System.on(cloud_status, cloud_status_handler);

    // Particle.function, Particle.variable and Particle.subscribe will not block
    // if called BEFORE Particle.connect
    Particle.variable("uptime_sec", uptime_sec);
    Particle.variable("connect_duration_sec", connect_duration_sec);
    Particle.variable("disconnect_duration_sec", disconnect_duration_sec);

    // Particle.connect will automatically enable the cellular/wifi module
    // Particle.connect only sets a flag and is non-blocking
    Particle.connect();
}

// loop() runs over and over again, as quickly as it can execute.
void loop()
{
    static uint32_t last_connected_publish_sec = System.uptime();
    static uint32_t last_loop_log_sec = System.uptime();

    uptime_sec = System.uptime();

    // update connect/disconnect durations
    if(Particle.connected())
    {
        connect_duration_sec = uptime_sec - connect_start_sec;
    }
    else
    {
        disconnect_duration_sec = uptime_sec - disconnect_start_sec;
    }

    if(Particle.connected() && disconnect_publish_requested)
    {
        // publish a disconnected event when requested
        publish_disconnected();
    }

    if(uptime_sec - last_connected_publish_sec >= 30)
    {
        // try to publish a connected event every 30 seconds no matter what
        last_connected_publish_sec = uptime_sec;
        Log.info("trying to publish \"connect\"");
        BackgroundPublishRK::instance().publish("connect",
            String::format("connect_duration_sec=%lu", connect_duration_sec),
            PRIVATE | WITH_ACK);
    }

    // print out a loop heartbeat every second
    if(uptime_sec - last_loop_log_sec >= 1)
    {
        last_loop_log_sec = uptime_sec;
        Log.info("%s: loop heartbeat", Time.format().c_str());
    }

    // continue processing below to manage critical device operations
}
