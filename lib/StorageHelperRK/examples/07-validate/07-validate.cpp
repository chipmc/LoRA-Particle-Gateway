#include "StorageHelperRK.h"


SerialLogHandler logHandler(LOG_LEVEL_TRACE);


SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

// Store data at the beginning of EEPROM
static const int EEPROM_OFFSET = 0;

class MyPersistentData : public StorageHelperRK::PersistentDataRetained {
public:
	class MyData {
	public:
		// This structure must always begin with the header (16 bytes)
		StorageHelperRK::PersistentDataBase::SavedDataHeader header;
		// Your fields go here. Once you've added a field you cannot add fields
		// (except at the end), insert fields, remove fields, change size of a field.
		// Doing so will cause the data to be corrupted!
		// You may want to keep a version number in your data.
		int test1;
		// OK to add more fields here 
	};

	static const uint32_t DATA_MAGIC = 0x96f17b78;
	static const uint16_t DATA_VERSION = 1;

	MyPersistentData() : PersistentDataRetained(&myData.header, sizeof(MyData), DATA_MAGIC, DATA_VERSION) {};

	int getValue_test1() const {
		return getValue<int>(offsetof(MyData, test1));
	}

	void setValue_test1(int value) {
		setValue<int>(offsetof(MyData, test1), value);
	}

    void logData(const char *msg) {
        Log.info("%s: %d", msg, myData.test1);
    }

	virtual bool validate(size_t dataSize) {
		bool valid = PersistentDataRetained::validate(dataSize);
		if (valid) {
			// If test1 < 0 or test1 > 100, then the data is invalid
			if (myData.test1 < 0 || myData.test1 > 100) {
				Log.info("data not valid test1=%d", myData.test1);
				valid = false;
			}
		}
		return valid;
	}

	void initialize() {
		PersistentDataRetained::initialize();

		Log.info("data initialized");

		// Initialize the default value to 10 if the structure is reinitialized.
		// Be careful doing this, because when MyData is extended to add new fields,
		// the initialize method is not called! This is only called when first
		// initialized.
		myData.test1 = 10;

		// If you manually update fields here, be sure to update the hash
		updateHash();
	}

	MyData myData;
};

retained MyPersistentData persistentData;

int testHandler(String cmd);

void setup() {
	waitFor(Serial.isConnected, 15000);
  	delay(1000);

	persistentData
		.withSaveDelayMs(0)
		.withLogData(true)
		.setup();

	persistentData.logData("initial value");

	Particle.function("test", testHandler);

	Particle.connect();
}

void loop() {
	persistentData.flush(false);
}

int testHandler(String cmd) {
    persistentData.setValue_test1(cmd.toInt());
	persistentData.logData("test");
	return 0;
}


