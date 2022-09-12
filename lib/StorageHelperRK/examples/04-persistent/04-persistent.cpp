#include "StorageHelperRK.h"


SerialLogHandler logHandler(LOG_LEVEL_INFO);


SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);


const char *persistentDataPath = "/usr/test04.dat";


class MyPersistentData : public StorageHelperRK::PersistentDataFile {
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
		bool test2;
		double test3;
		char test4[10];
		// OK to add more fields here 
	};

	static const uint32_t DATA_MAGIC = 0x20a99e74;
	static const uint16_t DATA_VERSION = 1;

	MyPersistentData() : PersistentDataFile(persistentDataPath, &myData.header, sizeof(MyData), DATA_MAGIC, DATA_VERSION) {};

	int getValue_test1() const {
		return getValue<int>(offsetof(MyData, test1));
	}

	void setValue_test1(int value) {
		setValue<int>(offsetof(MyData, test1), value);
	}

	bool getValue_test2() const {
		return getValue<bool>(offsetof(MyData, test2));
	}

	void setValue_test2(bool value) {
		setValue<bool>(offsetof(MyData, test2), value);
	}

	double getValue_test3() const {
		return getValue<double>(offsetof(MyData, test3));
	}

	void setValue_test3(double value) {
		setValue<double>(offsetof(MyData, test3), value);
	}

	String getValue_test4() const {
		String result;
		getValueString(offsetof(MyData, test4), sizeof(MyData::test4), result);
		return result;
	}
	bool setValue_test4(const char *str) {
		return setValueString(offsetof(MyData, test4), sizeof(MyData::test4), str);
	}

    void logData(const char *msg) {
        Log.info("%s: %d, %d, %lf, %s", msg, myData.test1, (int)myData.test2, myData.test3, myData.test4);
    }


	MyData myData;
};


MyPersistentData persistentData;


void setup() {
	// Load the persistent data
	persistentData.setup();

	Particle.connect();
}

void loop() {

    static unsigned long lastCheck = 0;
    if (millis() - lastCheck >= 10000) {
        lastCheck = millis();

        persistentData.setValue_test1(persistentData.getValue_test1() + 1);
        persistentData.setValue_test2(!persistentData.getValue_test2());
        persistentData.setValue_test3(persistentData.getValue_test3() - 0.1);
        persistentData.setValue_test4("testing!"); 
        persistentData.flush(true);

        persistentData.logData("test");
    }  
}



