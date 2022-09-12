# StorageHelperRK

Library for storing data in EEPROM, file system, SD card, FRAM, etc. on Particle devices

- Github: https://github.com/rickkas7/StorageHelperRK
- License: MIT (free for commercial or non-commercial use, including use in closed-source projects)
- Full browsable API: https://rickkas7.github.io/StorageHelperRK/


## Persistent Data

This library makes makes it easy to store simple persistent data on a variety of storage mechanisms including:

- Retained memory
- FRAM (MB85RCxx connected by I2C)
- Emulated EEPROM in Device OS
- POSIX flash filesystem (Particle Gen 3 devices and P2)
- SdFat (Micro SD card)
- Spiffs (SPI flash chips)

It's possible to add additional storage mechanisms by creating a subclass. It's not necessary to modify the library to add new storage mechanisms.

### Data storage

- Data includes C/C++ primitive types (int, float, double, bool, and other types like uint32_t) and C-strings of up to a maximum length that you configure.

- Data is always cached in RAM, so it can be read quickly and efficiently, with no latency and low overhead.

- You typically write simple get and set accessor functions for your data fields, so the correct data type is passed and returned.

- Data is stored in binary format on your underlying storage method for efficiency. 

- Data is extensible in future versions of your code. While you cannot modify the existing fields in any way, you can add additional data at the end of the structure. If the data on disk is smaller than the current structure, it is zeroed out.

- Data integrity detection includes magic bytes, length and version information, and a 32-bit hash of the data. These must match for be considered valid.

- Deferred updates can be enabled which allow you to save values frequently, but write them to storage less often to avoid flash wear and to not slow down the thread that is updating.

- The code is thread safe and can be used from loop, worker threads, or software timers.

### Example

This is from the example 04-persistent.cpp. This is the code you're write for your data.

```cpp
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
```

#### Code walk-through

You implement a subclass for your data. In this case, it's called `MyPersistentData` but you probably will want a more application-specific name.

The second part of the statement is what you are subclassing, and will be one of:

- StorageHelperRK::PersistentDataFile data in a file (POSIX, SdFat, SPIFFS)
- StorageHelperRK::PersistentDataRetained for retained memory
- StorageHelperRK::PersistentDataEEPROM for the emulated EEPROM in Particle Device OS
- StorageHelperRK::PersistentDataFRAM for MB85RCxx I2C FRAM

```cpp
class MyPersistentData : public StorageHelperRK::PersistentDataFile {
```

The next part is the description of your actual data you want to store.

- The structure must always begin with `StorageHelperRK::PersistentDataBase::SavedDataHeader header;`. This structure adds 16 bytes of overhead and contains the magic bytes, version information, hash. etc.
- You can add additional fields are desired for your application using C/C++ primitive types.
- You can also add c-string variables. In this example test4 can be up to 9 characters. If you pass a value longer than that for set, the value will be truncated.
- Once you release software with a data structure, you must never modify the existing fields, including resizing string fields or reordering fields.
- You can, however, add additional fields at the end at any time.

```cpp
class MyData {
public:
    StorageHelperRK::PersistentDataBase::SavedDataHeader header;
    int test1;
    bool test2;
    double test3;
    char test4[10];
    // OK to add more fields here 
};

```

- The magic bytes are 4 random bytes that you pick that identify your data structure. Since retained memory, EEPROM, etc. could contain contents left over from a previous application, this helps prevent invalid data from being used. If you completely change your data structure, you may want to pick new magic bytes.
- There is also a version field. If either the magic bytes or version do not match, the existing data will be erased.
- Do not update the version when simply adding fields at the end of the structure!
- The constructor declaration is boilerplate that will typically look something like this, though the parameters before `&myData.header` may be different.
- For file systems, the first parameter is the path to the file to store the data.

```cpp
static const uint32_t DATA_MAGIC = 0x20a99e74;
static const uint16_t DATA_VERSION = 1;

MyPersistentData() : PersistentDataFile(persistentDataPath, &myData.header, sizeof(MyData), DATA_MAGIC, DATA_VERSION) {};
```

Next we define accessor functions (get/set) for each of our variables. These are straightforward mappings between the type of data and the member in the class that holds the data.

```cpp
int getValue_test1() const {
    return getValue<int>(offsetof(MyData, test1));
}

void setValue_test1(int value) {
    setValue<int>(offsetof(MyData, test1), value);
}
```

The only one that's a little different are strings. 

- In the data structure, strings are always stored as c-strings, null-terminated.
- The maximum length of the string is defined when you declare the structure.
- You cannot resize the maximum field size after releasing a version without changing the version, which erases any previously saved data of the old version when loaded.
- If you pass too long of a string to setValue, the string will be truncated.

```cpp
String getValue_test4() const {
    String result;
    getValueString(offsetof(MyData, test4), sizeof(MyData::test4), result);
    return result;
}
bool setValue_test4(const char *str) {
    return setValueString(offsetof(MyData, test4), sizeof(MyData::test4), str);
}
```

A log function is optional, but you may find it useful for debugging.

```cpp
void logData(const char *msg) {
    Log.info("%s: %d, %d, %lf, %s", msg, myData.test1, (int)myData.test2, myData.test3, myData.test4);
}
```

You generally allocate an instance of the class on the heap. You don't have to worry about global constructor ordering because essentially nothing is done in the constructor.

```cpp
MyPersistentData persistentData;
```

In setup(), however, you must initialize the object. This will load it from the file system, in this case, and initialize the structure if the file is not present or is invalid.

- If the file does not exist the file will be initialized to zero values and empty strings
- If the file has invalid magic bytes, version, or hash, it will be reinitialized to zero values
- If the file contains data that is smaller than the current version, but is otherwise valid, then it will be preserved and only new values will be set to 0.

```cpp
void setup() {
	// Load the persistent data
	persistentData.setup();
```

Finally, you can get and set values as needed in your code.

```cpp
persistentData.setValue_test1(persistentData.getValue_test1() + 1);
```

If you set a value to the same value that it was before, no attempt will be made to save since for efficiency and to limit flash wear.


## Deferred save

The library supports deferred save mode, which does not save the contents immediately after setting value. 

Use the withSaveDelayMs() method to set the number of milliseconds to wait after changing data to save it. This is especially useful if you tend to set multiple fields at the same time.

Call the flush() method with the false parameter from loop(). This will save any deferred saves if necessary. This call is very fast with the false parameter so you can call it on every loop.

The example 05-eeprom uses deferred save mode.

Ideally, you will also want to call flush with the true parameter:

- Before system reset, via a reset system event handler
- Before sleep, which will be dependent on your code

When using the SleepHelper library, all of these things are taken care of automatically.

### Manual save mode

You can also use the library in manual save mode. Use withSaveDelayMs with a non-zero value but do not call flush(false) from loop. Instead only call flush(true) when you want to save changes.


## File system abstraction

There is a very limited file system abstraction as part of this library. It includes the bare minimum of functionality:

- open (with read, write, append, truncate, and/or create)
- close
- seek
- read
- write
- truncate

There are currently adapters for:

- POSIX (Particle Gen 3 devices)
- SdFat (Micro SD card)
- Spiffs (SPI flash)

You can add your own by subclassing FileSystemBase. Since a pointer to the FileSystemBase subclass object is passed to the PersistentDataFileSystem constructor, you can add new file systems without having to modify the library.

## Non-file subclasses

You can also subclass PersistentDataBase in the same way as PersistentDataEEPROM or PersistentDataBaseFRAM for things that aren't really files on a file system. This can also be done without modifying the library. You basically only need to implement the load and save methods. 


## Version history

### 0.0.2 (2022-08-29)

- Fixed a bug that prevented EEPROM data from being saved. 

### 0.0.1 (2022-07-18)

- Pulled the code from SleepHelper and added documentation
- Added adapters for SdFat, SPIFFS, and FRAM

