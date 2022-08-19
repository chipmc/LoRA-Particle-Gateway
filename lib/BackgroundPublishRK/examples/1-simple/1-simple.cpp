#include "Particle.h"

#include "BackgroundPublishRK.h"

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

const char *eventName = "publishTest";
int counter = 0;
const unsigned long PUBLISH_INTERVAL_MS = 30000;
unsigned long lastPublish = 0;

void publishCallback(bool succeeded, const char *eventName, const char *eventData, const void *context);

void setup() {
	// This must be called from setup() to start the background publishing thread
	BackgroundPublishRK::instance().start();

	Particle.connect();
}

void loop() {
	if (millis() - lastPublish >= PUBLISH_INTERVAL_MS) {
		lastPublish = millis();

		// This code runs every PUBLISH_INTERVAL_MS (currently 30 seconds)

		char data[64];
		snprintf(data, sizeof(data), "test %d", ++counter);

		// Use BackgroundPublishRK::instance().publish instead of Particle.publish
	    bool bResult = BackgroundPublishRK::instance().publish(eventName, data, PRIVATE | WITH_ACK, publishCallback);
	    Log.info("publish returned %d", bResult);
	}
}

void publishCallback(bool succeeded, const char *eventName, const char *eventData, const void *context) {
	Log.info("publishCallback succeeded=%d", succeeded);
}

