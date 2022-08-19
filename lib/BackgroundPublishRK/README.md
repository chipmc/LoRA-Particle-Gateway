# Background Publish

Background publish is a Particle firmware library that makes it easy to do `Particle.publish` from a background thread. This can be helpful in assuring that your loop() stays responsive, even if you have connection problems. This is most commonly an issue with cellular devices in fringe areas and for mobile cellular devices.

Doing a `Particle.publish` from regular loop code can cause delays, ranging from a few seconds to at worst nearly 5 minutes. For near-real-time applications this can be unacceptable. This library assures that you can request a publish and it will not block.

This library does not support queuing of multiple events; that will be handled by a different library. This only handles the basic case of single-event background publish and is very light-weight.

## Simple Example

Here's a simple example:

```
#include "Particle.h"

#include "BackgroundPublishRK.h"

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;

const char *eventName = "publishTest";
int counter = 0;
const unsigned long PUBLISH_INTERVAL_MS = 30000;
unsigned long lastPublish = 0;

void publishCallback(publish_status_t status, const char *eventName, const char *eventData, const void *context);

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

void publishCallback(publish_status_t status, const char *eventName, const char *eventData, const void *context) {
	Log.info("publishCallback status=%d", status);
}

``` 

Basically, instead of calling:

```
bool bResult = Particle.publish(eventName, data, PRIVATE | WITH_ACK);
```

You instead call:

```
bool bResult = backgroundPublish.publish(eventName, data, PRIVATE | WITH_ACK, publishCallback);
```

This uses an optional asynchronous callback function to know whether the publish went out or not. This typically takes up to 20 seconds, though in some rare cases it could take up to 5 minutes.

There are a few cases with `backgroundPublish.publish()` returns `false` immediately:

- If the library has not been started or `name` is NULL, then this function returns false.
- If there is already a publish in progress, then this function returns false.

Otherwise, the function returns `true` and the optional callback will be called later with a boolean `succeeded` value.

It's common to use `WITH_ACK`, which will yield a fairly reliable definition of success. If you use `NO_ACK` then success merely means an attempt was made to publish the event, not that it was actually sent successfully.

Failure will occur if:

- The cloud is not connected. This should return failure quickly with 1.4.x. It may take longer with older versions of Device OS.
- The event cannot be sent by the timeout (about 20 seconds).

## Full API

Background publish class. You typically instantiate one of these as a global variable.

## Members

---

### void BackgroundPublishRK::start() 

Start the background publish thread. Required!

```
void start()
```

You typically call this from setup() using:

```cpp
BackgroundPublishRK::instance().start();
```

---

### void BackgroundPublishRK::stop() 

Stop the background publish thread.

```
void stop()
```

Normally you start it and never stop it, but this method is provided for special cases.

---

### bool BackgroundPublishRK::publish(const char * name, const char * data, PublishFlags flags, PublishCompletedCallback cb, const void * context) 

Publish method. Use this instead of Particle.publish().

```
bool publish(const char * name, const char * data, PublishFlags flags, PublishCompletedCallback cb, const void * context)
```

#### Parameters
* `name` Event name to publish (required)

* `data` Event data (optional). Must be a c-string (null-terminated) if non-NULL. Maximum length varies by Device OS, currently 622 bytes. Note that the data must be UTF-8; you cannot send arbitrary binary data!

* `flags` The publish flash. Default = PRIVATE. You will often use `PRIVATE | WITH_ACK` but can also use `PRIVATE | NO_ACK`.

* `cb` The callback function to call when the publish completes. Optional. Pass NULL or omit the parameter if you don't need a callback. It can be a C function or a C++11 lambda.

* `context` Optional parameter passed to the callback. You can store a C++ object instance or a state structure pointer here.

---

### void BackgroundPublishRK::lock() 

Used internally to mutex lock to safely access data structures from multiple threads.

```
void lock()
```

You do not need to use this under normal circumstances as publish() handles this internally.

---

### void BackgroundPublishRK::unlock() 

Used internally to mutex lock to safely access data structures from multiple threads.

```
void unlock()
```


## Revision History

### 0.0.2 (2022-01-28)

- Rename BackgroundPublishRK class to BackgroundPublishRK to avoid conflict with a class of the same name in Tracker Edge.

### 0.0.1 (2021-04-16)

- Initial version

