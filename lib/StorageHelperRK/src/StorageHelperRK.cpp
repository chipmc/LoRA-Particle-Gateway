#include "StorageHelperRK.h"



//
// PersistentDataBase
//

void StorageHelperRK::PersistentDataBase::setup() {
    // Load data at boot
    load();
}

bool StorageHelperRK::PersistentDataBase::load() {
    WITH_LOCK(*this) {
        if (!validate(savedDataSize)) {
            initialize();
        }
    }

    return true;
}


void StorageHelperRK::PersistentDataBase::flush(bool force) {
    if (lastUpdate) {
        if (force || (millis() - lastUpdate >= saveDelayMs)) {
            save();
            lastUpdate = 0;
        }
    }
}

void StorageHelperRK::PersistentDataBase::saveOrDefer() {
    if (saveDelayMs) {
        lastUpdate = millis();
    }
    else {
        save();
    }
}



bool StorageHelperRK::PersistentDataBase::getValueString(size_t offset, size_t size, String &value) const {
    bool result = false;

    WITH_LOCK(*this) {
        if (offset <= (savedDataSize - (size - 1))) {
            const char *p = (const char *)savedDataHeader;
            p += offset;
            value = p; // copies string
            result = true;
        }
    }
    return result;
}

bool StorageHelperRK::PersistentDataBase::setValueString(size_t offset, size_t size, const char *value) {
    bool result = false;

    WITH_LOCK(*this) {
        if (offset <= (savedDataSize - (size - 1)) && strlen(value) < size) {
            char *p = (char *)savedDataHeader;
            p += offset;

            if (strcmp(value, p) != 0) {
                memset(p, 0, size);
                strcpy(p, value);
                savedDataHeader->hash = getHash();
                saveOrDefer();
            }
            result = true;
        }
    }
    return result;
}

uint32_t StorageHelperRK::PersistentDataBase::getHash() const {
    uint32_t hash;

    WITH_LOCK(*this) {
        // hash value is calculated over the whole data header and data, but with the hash field set to 0
        uint32_t savedHash = savedDataHeader->hash;
        savedDataHeader->hash = 0;
        hash = StorageHelperRK::murmur3_32((const uint8_t *)savedDataHeader, savedDataHeader->size, HASH_SEED);
        savedDataHeader->hash = savedHash;
    }
    return hash;
}


bool StorageHelperRK::PersistentDataBase::validate(size_t dataSize) {
    bool isValid = false;

    uint32_t hash = getHash();

    if (dataSize >= 12 && 
        savedDataHeader->magic == savedDataMagic && 
        savedDataHeader->version == savedDataVersion &&
        savedDataHeader->size <= (uint16_t) dataSize &&
        savedDataHeader->hash == hash) {                
        if ((size_t)dataSize < savedDataSize) {
            // Current structure is larger than what's in the file; pad with zero bytes
            uint8_t *p = (uint8_t *)savedDataHeader;
            for(size_t ii = (size_t)dataSize; ii < savedDataSize; ii++) {
                p[ii] = 0;
            }
        }
        savedDataHeader->size = (uint16_t) savedDataSize;
        savedDataHeader->hash = getHash();
        isValid = true;
    }   
    if (!isValid && dataSize != 0 && savedDataHeader->magic != 0) {
        // Only log if the data is not empty and was not zeroed out, to avoid logging when doing a load operation on
        // blank data (empty file or zeroed retained memory)
        Log.trace("got: magic=%08x version=%04x size=%04x hash=%08x", (int)savedDataHeader->magic, (int)savedDataHeader->version, (int)savedDataHeader->size, (int)savedDataHeader->hash);
        Log.trace("exp: magic=%08x version=%04x size=%04x hash=%08x", (int)savedDataMagic, (int)savedDataVersion, (int)dataSize, (int)hash);
    }
    return isValid;
}

void StorageHelperRK::PersistentDataBase::initialize() {
    memset(savedDataHeader, 0, savedDataSize);
    savedDataHeader->magic = savedDataMagic;
    savedDataHeader->version = savedDataVersion;
    savedDataHeader->size = (uint16_t) savedDataSize;
    savedDataHeader->hash = getHash();
}


#ifndef UNITTEST
//
// PersistentDataEEPROM
//
bool StorageHelperRK::PersistentDataEEPROM::load() {
    WITH_LOCK(*this) {
        for(int idx = 0; idx < (int)savedDataSize; idx++) {
            ((uint8_t *)savedDataHeader)[idx] = EEPROM.read(idx);
        }

        if (!validate(savedDataHeader->size)) {
            initialize();
        }
    }

    return true;
}

void StorageHelperRK::PersistentDataEEPROM::save() {
    WITH_LOCK(*this) {
        for(int idx = 0; idx < (int) savedDataSize; idx++) {
            EEPROM.write(idx, ((const uint8_t *)savedDataHeader)[idx]);
        }
        
        /*
        Log.info("saving EEPROM size=%d, offset=%d", (int)savedDataSize, (int)eepromOffset);
        Log.dump((const uint8_t *)savedDataHeader, savedDataSize);
        Log.print("\n");

        uint8_t test[16] = {0xff};
        for(int idx = 0; idx < (int) sizeof(test); idx++) {
            test[idx] = EEPROM.read(idx);
        }
        Log.info("read back header");
        Log.dump(test, sizeof(test));
        Log.print("\n");
        */
    }
}
#endif // UNITTEST

bool StorageHelperRK::PersistentDataFileSystem::load() {
    WITH_LOCK(*this) {
        bool loaded = false;

        int dataSize = 0;

        int fd = fs->open(filename, O_RDONLY);
        if (fd != -1) {
            dataSize = fs->read((uint8_t *)savedDataHeader, savedDataSize);

            // Log.info("request to read %d, got %d bytes", (int)savedDataSize, (int) dataSize);
            // Log.dump((const uint8_t *)savedDataHeader, dataSize);

            if (validate(dataSize)) {
                loaded = true;
            }
            fs->close();
        }
        else {
            Log.trace("did not open file %s", filename.c_str());
        }
        
        if (!loaded) {
            initialize();
        }
    }

    return true;
}

void StorageHelperRK::PersistentDataFileSystem::save() {
    WITH_LOCK(*this) {
        int fd = fs->open(filename, O_RDWR | O_CREAT | O_TRUNC);
        if (fd != -1) {            
            size_t count = fs->write((const uint8_t *)savedDataHeader, savedDataSize);

            // Log.info("request to write %d, wrote %d bytes", (int)savedDataSize, (int) count);
            // Log.dump((const uint8_t *)savedDataHeader, savedDataSize);

            fs->close();
        }
    }
}


uint32_t StorageHelperRK::murmur3_32(const uint8_t* key, size_t len, uint32_t seed) {
    // https://en.wikipedia.org/wiki/MurmurHash
	uint32_t h = seed;
    uint32_t k;
    /* Read in groups of 4. */
    for (size_t i = len >> 2; i; i--) {
        // Here is a source of differing results across endiannesses.
        // A swap here has no effects on hash properties though.
        memcpy(&k, key, sizeof(uint32_t));
        key += sizeof(uint32_t);
        h ^= murmur_32_scramble(k);
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64;
    }
    /* Read the rest. */
    k = 0;
    for (size_t i = len & 3; i; i--) {
        k <<= 8;
        k |= key[i - 1];
    }
    // A swap is *not* necessary here because the preceding loop already
    // places the low bytes in the low places according to whatever endianness
    // we use. Swaps only apply when the memory is copied in a chunk.
    h ^= murmur_32_scramble(k);
    /* Finalize. */
	h ^= len;
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;
	return h;
}
