#ifndef __STORAGEHELPERRK_H
#define __STORAGEHELPERRK_H

#include "Particle.h"

#include <fcntl.h>
#if HAL_PLATFORM_FILESYSTEM || defined(UNITTEST)
#include <sys/stat.h>
#endif

/**
 * @brief Class for storing data on a variety of different storage media
 */
class StorageHelperRK {
public:
    /**
     * @brief This is a wrapper around a recursive mutex, similar to Device OS RecursiveMutex
     * 
     * There are two differences:
     * 
     * - The mutex is created on first lock, instead of from the constructor. This is done because it's
     * not save to call os_mutex_recursive_create from a global constructor, and by delaying 
     * construction it makes it possible to safely construct the class as a global object.
     * - The lock/trylock/unlock methods are declared const acd nd the mutex handle mutable. This allows the
     * mutex to be locked from a const method.
     */
#ifndef UNITTEST
    class CustomRecursiveMutex
    {
        mutable os_mutex_recursive_t handle_;

    public:
        /**
         * @brief Construct a CustomRecursiveMutex wrapper object from an existing recursive mutex
         * 
         * @param handle The mutex handle
         * 
         * Ownership of the mutex handle is transferred to this object and it will be destroyed when this
         * object is deleted.
         */
        CustomRecursiveMutex(os_mutex_recursive_t handle) : handle_(handle) {
        }

        /**
         * @brief Default constructor with no mutex - one will be created on first lock
         * 
         */
        CustomRecursiveMutex() : handle_(nullptr) {
        }

        /**
         * @brief Destroys the underlying mutex object.
         * 
         */
        ~CustomRecursiveMutex() {
            dispose();
        }

        /**
         * @brief Destroys the mutex object
         */
        void dispose() {
            if (handle_) {
                os_mutex_recursive_destroy(handle_);
                handle_ = nullptr;
            }
        }

        /**
         * @brief Locks the mutex. Creates a new recursive mutex object if it does not exist yet.
         * 
         * Blocks if another thread has obtained the mutex, continues when the other thread releases it.
         * 
         * Never call lock from a SINGLE_THREADED_BLOCK since deadlock can occur.
         */
        void lock() const { 
            if (!handle_) {
                os_mutex_recursive_create(&handle_);
            }
            os_mutex_recursive_lock(handle_); 
        }
        /**
         * @brief Attempts to lock the mutex. Creates a new recursive mutex object if it does not exist yet.
         * 
         * Returns true if the mutex was locked or false if another thread has already locked it.
         */
        bool trylock() const { 
            if (!handle_) {
                os_mutex_recursive_create(&handle_);
            }
            return os_mutex_recursive_trylock(handle_)==0; 
        }

        /**
         * @brief Attempts to lock the mutex. Creates a new recursive mutex object if it does not exist yet.
         * 
         * Returns true if the mutex was locked or false if another thread has already locked it.
         */
        bool try_lock() const { 
            return trylock(); 
        }

        /**
         * @brief Unlocks the mutex
         * 
         * If another thread is blocked on locking the mutex, that thread will resume.
         */
        void unlock() const { 
            os_mutex_recursive_unlock(handle_); 
        }
    };
#else
    class CustomRecursiveMutex {
    public:
        void lock() const {};
        bool trylock() const { 
            return true;
        }
        bool try_lock() const { 
            return trylock(); 
        }
        void unlock() const { 
        }
    };
#endif /* UNITTEST */

    /**
     * @brief Abstract base class for file system-based storage
     */
    class FileSystemBase {
    public:
        /**
         * @brief Abstract base constructor. Doesn't do anything.
         */
        FileSystemBase() {
        }

        /**
         * @brief Abstract base destructor. Doesn't do anything.
         */
        virtual ~FileSystemBase() {

        }
        /**
         * @brief Open the events file
         * 
         * @param filename The name of the file to open
         * 
         * @param mode The open mode, such as O_RDWR | O_CREAT
         */
        virtual bool open(const char *filename, int mode) = 0;

        /**
         * @brief Close the events file
         */
        virtual bool close() = 0;

        /**
         * @brief Set file position
         *
         * @param seekTo The file offset to seek to if >= 0. Must be <= file length. Or pass
         * -1 to seek to the end of the file to append.
         *
         * @returns true on success or false on error
         */
        virtual bool seek(int seekTo) = 0;

        /**
         * @brief Truncate a file to a specified length in bytes
         *
         * @param size Size is bytes. Must be <= the current length of the file.
         *
         * Note: Do not use truncate to make the file larger! While this works for POSIX, it
         * does not work for SPIFFS so we just always assumes it does not work.
         */
        virtual bool truncate(size_t size) = 0;

        /**
         * @brief Read bytes from the file
         *
         * @param buffer Buffer to fill with data
         *
         * @param length Number of bytes to read. Can be > than the number of bytes in the file.
         *
         * @returns Number of bytes read. Returns 0 on error.
         */
        virtual size_t read(uint8_t *buffer, size_t length) = 0;

        /**
         * @brief Write bytes to the file
         *
         * @param buffer Buffer to write to the file
         *
         * @param length Number of bytes to write.
         */
        virtual size_t write(const uint8_t *buffer, size_t length) = 0;

        /**
         * @brief Get length of the file (or a negative error code on error)
         */
        virtual int getLength() = 0;

    };

    #if defined(__SPIFFSPARTICLERK_H) || defined(DOXYGEN_BUILD)

    /**
     * @brief Concrete subclass to store events on SPIFFS file system
     */
    class FileSystemSpiffs : public FileSystemBase {
    public:
        /**
         * @brief Store events on a SPIFFS file system
         *
         * @param spiffs The SpiffsParticle object for the file system to store the data on
         */
        FileSystemSpiffs(SpiffsParticle &spiffs) : spiffs(spiffs) {
        }

        /**
         * @brief Destructor
         */
        virtual ~FileSystemSpiffs() {
        }

        /**
         * @brief Translate POSIX style mode flags into SPIFFS mode flags
         * 
         * @param mode 
         * @return int 
         */
        int translateMode(int mode) {
            int spiffsMode = 0;

            switch(mode & O_ACCMODE) {
                case O_RDONLY:
                    spiffsMode |= SPIFFS_O_RDONLY;
                    break;
                case O_WRONLY:
                    spiffsMode |= SPIFFS_O_WRONLY;
                    break;
                case O_RDWR:
                default:
                    spiffsMode |= SPIFFS_O_RDWR;
                    break;
            }

            if (mode & O_APPEND) {
                spiffsMode |= SPIFFS_O_APPEND;
            }
            if (mode & O_TRUNC) {
                spiffsMode |= SPIFFS_O_TRUNC;
            }
            if (mode & O_CREAT) {
                spiffsMode |= SPIFFS_O_CREAT;
            }

            Log.info("in mode %04x out mode %04x", (int) mode, (int) spiffsMode);

            return spiffsMode;
        }

        /**
         * @brief Open the events file
         */
        virtual bool open(const char *filename, int mode = O_RDWR | O_CREAT) {
            file = spiffs.openFile(filename, translateMode(mode));
            return true;
        }

        /**
         * @brief Close the events file
         */
        virtual bool close() {
            file.close();
            return true;
        }

        /**
         * @brief Set file position
         *
         * @param seekTo The file offset to seek to if >= 0. Must be <= file length. Or pass
         * -1 to seek to the end of the file to append.
         *
         * @returns true on success or false on error
         */
        virtual bool seek(int seekTo) {
            if (seekTo >= 0) {
                return file.lseek(seekTo, SPIFFS_SEEK_SET) >= 0;
            }
            else {
                return file.lseek(0, SPIFFS_SEEK_END) >= 0;
            }
        }

        /**
         * @brief Read bytes from the file
         *
         * @param buffer Buffer to fill with data
         *
         * @param length Number of bytes to read. Can be > than the number of bytes in the file.
         *
         * @returns Number of bytes read. Returns 0 on error.
         */
        virtual size_t read(uint8_t *buffer, size_t length) {
            return file.readBytes((char *)buffer, length);
        }

        /**
         * @brief Write bytes to the file
         *
         * @param buffer Buffer to write to the file
         *
         * @param length Number of bytes to write.
         */
        virtual size_t write(const uint8_t *buffer, size_t length) {
            return file.write(buffer, length);
        }

        /**
         * @brief Get length of the file or a negative error code
         */
        virtual int getLength() {
            return (int) file.length();
        }

        /**
         * @brief Truncate a file to a specified length in bytes
         *
         * Note: Do not use truncate to make the file larger! While this works for POSIX, it
         * does not work for SPIFFS so we just always assumes it does not work.
         */
        virtual bool truncate(size_t size) {
            return file.truncate((s32_t)size) == SPIFFS_OK;
        }


    protected:
        SpiffsParticle &spiffs;		//!< SpiffsParticle object for the file system to store events on
        SpiffsParticleFile file;	//!< Object for the events file
    };

    #endif /* __SPIFFSPARTICLERK_H */


    #if defined(SdFat_h) || defined(DOXYGEN_BUILD)

    /**
     * @brief Concrete subclass for storing events on a SdFat file system
     */
    class FileSystemSdFat : public FileSystemBase {
    public:
        /**
         * @brief Store events on a SdFat file system
         *
         * @param sdFat The SdFat object for the file system to store the data on
         */
        FileSystemSdFat(SdFat &sdFat) : sdFat(sdFat){
        }

        virtual ~FileSystemSdFat() {
        }

        /**
         * @brief Open the events file
         * 
         * @param filename The filename to store the events in should be 8.3 format (events.dat, for example)
         */
        virtual bool open(const char *filename, int mode = O_RDWR | O_CREAT) {
            return file.open(filename, mode) != 0;
        }

        /**
         * @brief Close the events file
         */
        virtual bool close() {
            file.close();
            return true;
        }

        /**
         * @brief Set file position
         */
        virtual bool seek(int seekTo) {
            if (seekTo >= 0) {
                return file.seekSet(seekTo);
            }
            else {
                return file.seekEnd();
            }
        }

        /**
         * @brief Read bytes from the file
         *
         * @param buffer Buffer to fill with data
         *
         * @param length Number of bytes to read. Can be > than the number of bytes in the file.
         *
         * @returns Number of bytes read. Returns 0 on error.
         */
        virtual size_t read(uint8_t *buffer, size_t length) {
            return file.read((char *)buffer, length);
        }

        /**
         * @brief Write bytes to the file
         *
         * @param buffer Buffer to write to the file
         *
         * @param length Number of bytes to write.
         */
        virtual size_t write(const uint8_t *buffer, size_t length) {
            return file.write(buffer, length);
        }

        /**
         * @brief Get length of the file or a negative error code
         */
        virtual int getLength() {
            return (int) file.fileSize();
        }

        /**
         * @brief Truncate a file to a specified length in bytes
         *
         * Note: Do not use truncate to make the file larger! While this works for POSIX, it
         * does not work for SPIFFS so we just always assumes it does not work.
         */
        virtual bool truncate(size_t size) {
            return file.truncate((uint32_t)size);
        }


    protected:
        SdFat &sdFat;			//!< SdFat object for the file system to store the events on
        SdFile file;			//!< SdFat file object for the events file
    };
    #endif /* SdFat_h */

    #if HAL_PLATFORM_FILESYSTEM || defined(UNITTEST) || defined(DOXYGEN_BUILD)
    /**
     * @brief Concrete subclass for storing events on a Particle Gen 3 LittleFS POSIX file system
     */
    class FileSystemPosix : public FileSystemBase {
    public:
        /**
         * @brief Store events on a SdFat file system
         *
         * @param filename The filename to store the events in
         */
        FileSystemPosix() {
        }

        virtual ~FileSystemPosix() {
        }

        /**
         * @brief Open the events file
         */
        virtual bool open(const char *filename, int mode = O_RDWR | O_CREAT) {
            fd = ::open(filename, mode, 0666);

            return (fd != -1);
        }

        /**
         * @brief Close the events file
         */
        virtual bool close() {
            if (fd != -1) {
                ::close(fd);
                fd = -1;
            }
            return true;
        }

        /**
         * @brief Set file position
         */
        virtual bool seek(int seekTo) {
            if (seekTo >= 0) {
                return lseek(fd, seekTo, SEEK_SET) >= 0;
            }
            else {
                return lseek(fd, 0, SEEK_END) >= 0;
            }
        }

        /**
         * @brief Read bytes from the file

         * @param buffer Buffer to fill with data
         *
         * @param length Number of bytes to read. Can be > than the number of bytes in the file.
         *
         * @returns Number of bytes read. Returns 0 on error.
         */
        virtual size_t read(uint8_t *buffer, size_t length) {
            int count = ::read(fd, buffer, length);
            if (count > 0) {
                return count;
            }
            else {
                return 0;
            }
        }

        /**
         * @brief Write bytes to the file
         *
         * @param buffer Buffer to write to the file
         *
         * @param length Number of bytes to write.
         */
        virtual size_t write(const uint8_t *buffer, size_t length) {
            int count = ::write(fd, buffer, length);
            if (count > 0) {
                return count;
            }
            else {
                return 0;
            }
        }

        /**
         * @brief Get length of the file or a negative error code
         */
        virtual int getLength() {
            struct stat sb;

            fstat(fd, &sb);
            return sb.st_size;
        }

        /**
         * @brief Truncate a file to a specified length in bytes
         *
         */
        virtual bool truncate(size_t size) {
            // Note: This requires Device OS 2.0.0-rc.3 or later!
            return ftruncate(fd, (int)size) == 0;
        }


    protected:
        int fd = -1;			//!< File descriptor for the events file
    };

    #endif /* HAL_PLATFORM_FILESYSTEM || defined(UNITTEST) || defined(DOXYGEN_BUILD) */

    /**
     * @brief Base class for storing persistent binary data to a file or retained memory
     * 
     * This class is separate from PersistentData so you can subclass it to hold your own application-specific
     * data as well.
     * 
     * See PersistentDataFile for saving data to a file on the flash file system.
     */
    class PersistentDataBase : public CustomRecursiveMutex {
    public:
        /**
         * @brief Header at the beginning of all saved data stored in RAM, retained memory, or a file.
         */
        class SavedDataHeader { // 16 bytes
        public:
            uint32_t magic;                 //!< savedDataMagic, should rarely, if ever, change
            uint16_t version;               //!< savedDataVersion, should rarely, if ever, change
            uint16_t size;                  //!< size of the whole structure, including the user data after it
            uint32_t hash;                  //!< hash value for verifying data integrity
            uint32_t reserved1;             //!< reserved for future use
            // You cannot change the size of this structure without changing the version number!
        };
        
        /**
         * @brief Base class for persistent data saved in file or RAM
         * 
         * @param savedDataHeader Pointer to the header
         * @param savedDataSize size of the whole structure, including the user data after it 
         * @param savedDataMagic Magic bytes to use for this data
         * @param savedDataVersion Version to use for this data
         */
        PersistentDataBase(SavedDataHeader *savedDataHeader, size_t savedDataSize, uint32_t savedDataMagic, uint16_t savedDataVersion) : 
            savedDataHeader(savedDataHeader), savedDataSize(savedDataSize), savedDataMagic(savedDataMagic), savedDataVersion(savedDataVersion)  {
        };
        
        /**
         * @brief Sets the wait to save delay. Default is 1000 milliseconds.
         * 
         * @param value Value is milliseconds, or 0
         * @return PersistentData& 
         * 
         * Normally, if the value is changed by a set call, then about
         * one second later the change will be saved to disk from the loop thread. The
         * storageHelperRKData is also saved before sleep or reset if changed.
         * 
         * You can change the save delay by using withSaveDelayMs(). If you set it to 0, then
         * the data is saved within the setValue call immediately, which will make all set calls
         * run more slowly.
         */
        PersistentDataBase &withSaveDelayMs(uint32_t value) {
            saveDelayMs = value;
            if (saveDelayMs == 0) {
                flush(true);
            }
            return *this;
        }
        /**
         * @brief Initialize this object for use in StorageHelperRK
         * 
         * This is used from StorageHelperRK::setup(). You should not use this if you are creating your
         * own PersistentData object; this is only used to hook this class into StorageHelperRK/
         */
        virtual void setup();

        /**
         * @brief Load the persistent data file. You normally do not need to call this; it will be loaded automatically.
         * 
         * @return true 
         * @return false 
         */
        virtual bool load();

        /**
         * @brief Save the persistent data file. You normally do not need to call this; it will be saved automatically.
         * 
         * Save does nothing in this base class, but for PersistentDataFile it saves to a file
         */
        virtual void save() {};

        /**
         * @brief Write the settings to disk if changed and the wait to save time has expired
         * 
         * @param force Pass true to ignore the wait to save time and save immediately if necessary. This
         * is used when you're about to sleep or reset, for example.
         * 
         * This call is fast if a save is not required so you can call it frequently, even every loop.
         */
        virtual void flush(bool force);

       /**
         * @brief Either saves data or immediately, or defers until later, based on saveDelayMs
         * 
         * If saveDelayMs == 0, then always saves immediately. Otherwise, waits that amount of time before saving
         * to allow multiple saves to be batch and to not block the updating thread.
         */
        virtual void saveOrDefer();

        /**
         * @brief Templated class for getting integral values (uint32_t, float, double, etc.)
         * 
         * @tparam T 
         * @param offset 
         * @return T 
         */
        template<class T>
        T getValue(size_t offset) const {
            T result = 0;

            WITH_LOCK(*this) {
                if (offset <= (savedDataSize - sizeof(T))) {
                    const uint8_t *p = (uint8_t *)savedDataHeader;
                    p += offset;
                    result = *(const T *)p;
                }
            }
            return result;
        }

        /**
         * @brief Templated class for setting integral values (uint32_t, float, double, etc.)
         * 
         * @tparam T An integral types
         * @param offset Offset into the structure, normally offsetof(field, T)
         * @param value The value to set
         */
        template<class T>
        void setValue(size_t offset, T value)  {
            WITH_LOCK(*this) {
                if (offset <= (savedDataSize - sizeof(T))) {
                    uint8_t *p = (uint8_t *)savedDataHeader;
                    p += offset;

                    T oldValue = *(T *)p;
                    if (oldValue != value) {
                        *(T *)p = value;
                        savedDataHeader->hash = getHash();
                        saveOrDefer();
                    }
                }
            }
        }

        /**
         * @brief Get the value of a string
         * 
         * @param offset 
         * @param size 
         * @param value 
         * @return true 
         * @return false 
         * 
         * The templated getValue method doesn't work with String values, so this version should be used instead.
         */
        bool getValueString(size_t offset, size_t size, String &value) const;

        /**
         * @brief Set the value of a string
         * 
         * @param offset 
         * @param size 
         * @param value 
         * @return true 
         * @return false 
         * 
         * The templated setValue method doesn't work with string values, so this version should be used instead.
         */
        bool setValueString(size_t offset, size_t size, const char *value);
        
        
        /**
         * @brief Get the hash valid for data integrity checking
         */
        uint32_t getHash() const;

        static const uint32_t HASH_SEED = 0x851c2a3f; //!< Murmur32 hash seed value (randomly generated)

    protected:
        /**
         * This class cannot be copied
         * 
         * Note that subclasses don't need to do include a definition like this because as long as the base class is 
         * copy-prevented subclasses by default cannot be copied.
         */
        PersistentDataBase(const PersistentDataBase&) = delete;

        /**
         * This class cannot be copied
         */
        PersistentDataBase& operator=(const PersistentDataBase&) = delete;

        /**
         * @brief Used to validate the saved data structure. Used internally by load(). 
         * 
         * @return true 
         * @return false
         * 
         * If you subclass this, be sure to call the superclass validate() first. If it returns false,
         * you should just return false and skip your own validation since the outer headers were not
         * valid and the structure cannot be used until reinitialized. 
         */
        virtual bool validate(size_t dataSize);

        /**
         * @brief Used to allow subclasses to initialize the saved data structure. Called internally by load(). 
         * 
         * If you subclass this, be sure to call the superclass initialize() after you update your fields short so
         * the hash value can be updated.
         */
        virtual void initialize();


        SavedDataHeader *savedDataHeader = 0; //!< Pointer to the saved data header, which is followed by the data
        uint32_t savedDataSize = 0;     //!< Size of the saved data (header + actual data)
        
        uint32_t savedDataMagic;        //!< Magic bytes for the saved data
        uint16_t savedDataVersion;      //!< Version number for the saved data

        uint32_t lastUpdate = 0; //!< Last time the file was updated. 0 = file has not changed since writing to disk.
        uint32_t saveDelayMs = 1000; //!< How long to wait to save before writing file to disk. Set to 0 to write immediately.
    };

    /**
     * @brief Class for persistent data stored in retained memory
     * 
     * Retained memory (backup RAM, SRAM) is supported on Gen 2 and Gen 3 devices and is around 3K bytes in size.
     * It's preserved across system reset and sometimes across flashing new code. On some Gen 2 devices an external
     * battery or supercap can be added to preserve data on power-down. 
     * 
     * Retained memory is not supported on the P2 or Photon 2.
     */
    class PersistentDataRetained : public PersistentDataBase {
    public:
        /**
         * @brief Class for persistent data saved in retained memory
         * 
         * @param savedDataHeader Pointer to the saved data header in retained memory (backup RAM, SRAM)
         * @param savedDataSize size of the whole structure, including the user data after it 
         * @param savedDataMagic Magic bytes to use for this data
         * @param savedDataVersion Version to use for this data
         */
        PersistentDataRetained(SavedDataHeader *savedDataHeader, size_t savedDataSize, uint32_t savedDataMagic, uint16_t savedDataVersion) : 
            PersistentDataBase(savedDataHeader, savedDataSize, savedDataMagic, savedDataVersion) {
        };
    };

#ifndef UNITTEST
    /**
     * @brief Class for persistent data stored in emulated EEPROM
     * 
     * This is only recommended for Gen 2 devices. On Gen 3 devices, EEPROM is implemented as a file in the file
     * system anyway, so it's more efficient to just store your persistent data in its own file.
     * 
     */
    class PersistentDataEEPROM : public PersistentDataBase {
    public:
        /**
         * @brief Class for persistent data saved in emulated EEPROM
         * 
         * @param eepromOffset Offset into the EEPROM to store the data.
         * @param savedDataHeader Pointer to the saved data header
         * @param savedDataSize size of the whole structure, including the user data after it 
         * @param savedDataMagic Magic bytes to use for this data
         * @param savedDataVersion Version to use for this data
         */
        PersistentDataEEPROM(int eepromOffset, SavedDataHeader *savedDataHeader, size_t savedDataSize, uint32_t savedDataMagic, uint16_t savedDataVersion) : 
            PersistentDataBase(savedDataHeader, savedDataSize, savedDataMagic, savedDataVersion), eepromOffset(eepromOffset) {
        };
        

        /**
         * @brief Load the persistent data file. You normally do not need to call this; it will be loaded automatically.
         * 
         * @return true 
         * @return false 
         */
        virtual bool load();

        /**
         * @brief Save the persistent data file. You normally do not need to call this; it will be saved automatically.
         */
        virtual void save();
    
    protected:
        int eepromOffset; //!< Offset into EEPROM to save the data
    };
#endif // UNITTEST

#if defined(__MB85RC256V_FRAM_RK) || defined(DOXYGEN_BUILD)
    /**
     * @brief Support for MB85RC256V-FRAM-RK library.
     *
     * If you include "MB85RC256V-FRAM-RK.h" before StorageHelperRK.h, this code will be enabled
     */
    class PersistentDataFRAM : public PersistentDataBase {
    public:
        /**
         * @brief Base class for persistent data saved in file
         * 
         * @param savedDataHeader Pointer to the saved data header
         * @param savedDataSize size of the whole structure, including the user data after it 
         * @param savedDataMagic Magic bytes to use for this data
         * @param savedDataVersion Version to use for this data
         */
        PersistentDataFRAM(MB85RC &fram, int framOffset, SavedDataHeader *savedDataHeader, size_t savedDataSize, uint32_t savedDataMagic, uint16_t savedDataVersion) : 
            PersistentDataBase(savedDataHeader, savedDataSize, savedDataMagic, savedDataVersion), fram(fram), framOffset(framOffset) {
        };
        
        /**
         * @brief Load the persistent data file. You normally do not need to call this; it will be loaded automatically.
         * 
         * @return true 
         * @return false 
         */
        virtual bool load() {
            WITH_LOCK(*this) {
                fram.readData(framOffset, (uint8_t*)savedDataHeader, savedDataSize);
                if (!validate(savedDataHeader->size)) {
                    initialize();
                }
            }

            return true;
        }

        /**
         * @brief Save the persistent data file. You normally do not need to call this; it will be saved automatically.
         */
        virtual void save() {
            WITH_LOCK(*this) {
                fram.writeData(framOffset, (const uint8_t*)savedDataHeader, savedDataSize);
            }
        } 

    protected:
        MB85RC &fram; //!< Reference to FRAM object
        int framOffset; //!< Offset into FRAM to save the data
    };
    #endif // defined(__MB85RC256V_FRAM_RK) || defined(DOXYGEN_BUILD)

    /**
     * @brief Base class for data stored to a file system (POSIX, SdFat, SPIFFS)
     * 
     * Since using the Gen 3/P2/Photon 2 POSIX file system is a common use-case, there is a concrete
     * subclass of this, PersistentDataFile, for use with POSIX file systems. 
     */
    class PersistentDataFileSystem : public PersistentDataBase {
    public:
        /**
         * @brief Class for persistent data saved to a file system
         * 
         * @param fs The FileSystemBase object subclass to use (Posix, SdFat, SPIFFS, etc.) 
         * @param filename The filename or pathname to the file used to save data
         * @param savedDataHeader Pointer to the saved data header
         * @param savedDataSize size of the whole structure, including the user data after it 
         * @param savedDataMagic Magic bytes to use for this data
         * @param savedDataVersion Version to use for this data
         * 
         * The fs object passed into this constructor is deleted when this object is destructed.
         * 
         * You will probably want to use the withFilename() method to set the filename to save the data to.
         */
        PersistentDataFileSystem(FileSystemBase *fs, const char *filename, SavedDataHeader *savedDataHeader, size_t savedDataSize, uint32_t savedDataMagic, uint16_t savedDataVersion) : 
            PersistentDataBase(savedDataHeader, savedDataSize, savedDataMagic, savedDataVersion), fs(fs), filename(filename) {
        };

        virtual ~PersistentDataFileSystem() {
            delete fs;
        }

        /**
         * @brief Sets the filename to use to store the data
         * 
         * @param filename 
         * @return PersistentDataFileSystem& 
         */
        PersistentDataFileSystem &withFilename(const char *filename) {
            this->filename = filename;
            return *this;
        }

        /**
         * @brief Load the persistent data file. You normally do not need to call this; it will be loaded automatically.
         * 
         * @return true 
         * @return false 
         */
        virtual bool load();

        /**
         * @brief Save the persistent data file. You normally do not need to call this; it will be saved automatically.
         */
        virtual void save();

 

    protected:
        FileSystemBase *fs; //!< The file system object the persistent data will be stored on
        String filename; //!<  The filename on the file system
    };

    #if HAL_PLATFORM_FILESYSTEM || defined(UNITTEST) || defined(DOXYGEN_BUILD)
    /**
     * @brief Class for persistent data stored in a file on the POSIX file system on Gen 3, P2, and Photon 2
     * 
     */
    class PersistentDataFile : public PersistentDataFileSystem {
    public:
        /**
         * @brief Base class for persistent data saved in file
         * 
         * @param filename The filename or pathname to the file used to save data
         * @param savedDataHeader Pointer to the saved data header
         * @param savedDataSize size of the whole structure, including the user data after it 
         * @param savedDataMagic Magic bytes to use for this data
         * @param savedDataVersion Version to use for this data
         */
        PersistentDataFile(const char *filename, SavedDataHeader *savedDataHeader, size_t savedDataSize, uint32_t savedDataMagic, uint16_t savedDataVersion) : 
            PersistentDataFileSystem(new FileSystemPosix(), filename, savedDataHeader, savedDataSize, savedDataMagic, savedDataVersion) {
        };
    
    protected:
    };
    #endif // HAL_PLATFORM_FILESYSTEM || defined(UNITTEST) || defined(DOXYGEN_BUILD)

    /**
     * @brief Murmur3 hash algorithm implementation
     * 
     * @param buf Pointer to the buffer to hash (const uint8_t *) 
     * @param len Length of the data in bytes
     * @param seed hash seed value (uint32_t)
     * @return uint32_t 
     * 
     * This implementation has been moved into the top-level StorageHelperRK
     * class and you should use that directly.
     */
    static uint32_t murmur3_32(const uint8_t* buf, size_t len, uint32_t seed);

private:
    /**
     * @brief Part of the algorithm used by murmur3_32(). Used internally.
     * 
     * @param k 
     * @return uint32_t 
     */
    static inline uint32_t murmur_32_scramble(uint32_t k) {
        k *= 0xcc9e2d51;
        k = (k << 15) | (k >> 17);
        k *= 0x1b873593;
        return k;
    }

};


#endif // __STORAGEHELPERRK_H