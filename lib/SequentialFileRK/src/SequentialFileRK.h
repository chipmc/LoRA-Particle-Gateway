#ifndef __SEQUENTIALFILERK_H
#define __SEQUENTIALFILERK_H

#include "Particle.h"

#include <deque>

/**
 * @brief Class for maintaining a directory of files as a queue with unique filenames
 *
 * You select a directory on the Gen 3 flash file system for use as the queue directory.
 * This directory is filled with numbered files, for example: 00000001.jpg. It's 
 * possible to store meta information in additional files. For example, 00000001.sha1
 * to hold a SHA-1 hash of the file. The file are numbered sequentially.
 * 
 * You typically instantiate this class as a global variable. 
 * 
 * In global setup(), configure it using the withXXX() methods, then call the scanDir()
 * method to read the queue from disk. In particular, you must use withDirPath() to set
 * the queue directory!
 * 
 * Files left on disk are added to the queue during scanDir() and an in-memory queue
 * is initialized with the files to process.
 * 
 * Code can reserve a space in the queue using reserveFile(). This returns the next
 * available file number but does not create the file. 
 * 
 * Once you have fully written the file, call addFileToQueue().
 * 
 * The code that processes files in the queue calls getFileFromQueue(). 
 */
class SequentialFile {
public:
    /**
     * @brief Default constructor
     * 
     * This object is often created as a global variable. 
     * 
     * In global setup(), configure it using the withXXX() methods, then call the scanDir()
     * method to read the queue from disk.
     */
    SequentialFile();

    /**
     * @brief Destructor
     * 
     * This object is often created as a global variables and never deleted.
     */
    virtual ~SequentialFile();

    /**
     * @brief Sets the directory to use as the queue directory. This is required!
     * 
     * @param dirPath the pathname, Unix-style with / as the directory separator. 
     * 
     * Typically you create your queue either at the top level ("/myqueue") or in /usr
     * ("/usr/myqueue"). The directory will be created if necessary, however only one
     * level of directory will be created. The parent must already exist.
     * 
     * The dirPath can end with a slash or not, but if you include it, it will be
     * removed.
     * 
     * You must call this as you cannot use the root directory as a queue!
     */
    SequentialFile &withDirPath(const char *dirPath);

    /**
     * @brief Gets the directory path set using withDirPath()
     * 
     * The returned path will not end with a slash.
     */
    const char *getDirPath() const { return dirPath; };

    /**
     * @brief Sets the filename pattern. Default is %08d (8-digit decimal number with leading zeros)
     * 
     * @param pattern The sprintf/sscanf pattern to use
     * 
     * This string is used when scanning the queue directory to find files. Do not include the
     * filename extension in this pattern!
     */
    SequentialFile &withPattern(const char *pattern) { this->pattern = pattern; return *this; };

    /**
     * @brief Gets the filename pattern
     */
    const char *getPattern() const { return pattern; };
    
    /**
     * @brief Filename extension for queue files. (Default: no extension)
     * 
     * @param ext The filename extension (just the extension, no preceding dot)
     * 
     * For example, for JPEG files it might be "jpg".
     */
    SequentialFile &withFilenameExtension(const char *ext) { this->filenameExtension = ext; return *this; };

    /**
     * @brief Gets the filename extension
     * 
     * Returned string does not contain the dot. For example, for JPEG files it might be "jpg".
     */
    const char *getFilenameExtension() const { return filenameExtension; };

    /**
     * @brief Scans the queue directory for files. Typically called during setup().
     */
    bool scanDir(void);

    /**
     * @brief Reserve a file number you will use to write data to
     * 
     * Use getPathForFileNum() to get the pathname to the file. Reservations are in-RAM only
     * so if the device reboots before you write the file, the reservation will be lost.
     */
    int reserveFile(void);

    /**
     * @brief Adds a previously reserved file to the queue
     * 
     * Use reserveFile() to get the next file number, addFileToQueue() to add it to the queue
     * and getFileFromQueue() to get an item from the queue.
     * 
     * It's safe to call reserveFile(), addFileToQueue(), and getFileFromQueue() from different
     * threads. Locking is handled internally.
     */
    void addFileToQueue(int fileNum);

    /**
     * @brief Gets a file from the queue
     * 
     * @param remove (optional, default true). If true, removes the file from the queue in
     * RAM. If false, calling getFileFromQueue() again will retrieve the same fileNum.
     * 
     * @return 0 if there are no items in the queue, or a fileNum for an item in the queue.
     * 
     * Use getPathForFileNum() to convert the number into a pathname for use with open().
     * 
     * The queue is stored in RAM, so if the device reboots before you delete the file it
     * will reappear in the queue when scanDir is called. This method does not need to access 
     * the filesystem. You can call getFileFromQueue() on every loop if you want to to check 
     * if there are files to process without affecting performance.
     * 
     * It's safe to call reserveFile(), addFileToQueue(), and getFileFromQueue() from different
     * threads. Locking is handled internally.
     * 
     */
    int getFileFromQueue(bool remove = true);

    /**
     * @brief Uses pattern to create a filename given a fileNum
     * 
     * @param fileNum A file number, typically from reserveFile() or getFileFromQueue()
     * 
     * @param overrideExt If non-null, use this extension instead of the configured
     * filename extension. It should not contain the preceeding dot.
     * 
     * The overrideExt is used when you create multiple files per queue entry fileNum,
     * for example a data file and a .sha1 hash for the file. Or other metadata.
     */
    String getNameForFileNum(int fileNum, const char *overrideExt = NULL);

    /**
     * @brief Gets a full pathname based on dirName and getNameForFileNum
     * 
     * @param fileNum A file number, typically from reserveFile() or getFileFromQueue()
     * 
     * @param overrideExt If non-null, use this extension instead of the configured
     * filename extension.
     * 
     * The overrideExt is used when you create multiple files per queue entry fileNum,
     * for example a data file and a .sha1 hash for the file. Or other metadata.
     */
    String getPathForFileNum(int fileNum, const char *overrideExt = NULL);

    /**
     * @brief Remove fileNum from the flash file system
     *
     * @param fileNum A file number, typically from reserveFile() or getFileFromQueue()
     * 
     * @param allExtensions If true, all files with that number regardless of extension are removed.
     * 
     */
    void removeFileNum(int fileNum, bool allExtensions);

    /**
     * @brief Removes all of the files in the queue directory
     * 
     * @brief removeDir true to remove the queue directory itself, false to just remove the contents.
     * 
     * Note this is all files, not just the filenames with the matching extension. 
     * Also removes the entries from the RAM-based queue and sets lastFileNum to 0.
     */
    void removeAll(bool removeDir);

    /**
     * @brief Gets the length of the queue
     */
    int getQueueLen() const;

    /**
     * @brief This class is not copyable
     */
    SequentialFile(const SequentialFile&) = delete;

    /**
     * @brief This class is not copyable
     */
    SequentialFile& operator=(const SequentialFile&) = delete;

    /**
     * @brief Creates the directory path if it does not exist
     * 
     * Note: This only creates the last element of the path if it does not
     * exist. If you use directories nested more than one level deep,
     * you'll need to call this separately to create parent directories!
     */
    static bool createDirIfNecessary(const char *path);

    /**
     * @brief Combines name and ext into a filename with an optional filename extension
     * 
     * @param name Filename (required)
     * 
     * @param ext Filename extension (optional). If you pass is NULL or an 
     * empty string, the dot and extension are not appended.
     * 
     * Only adds the . when ext is non-null and non-empty.
     */
    static String getNameWithOptionalExt(const char *name, const char *ext);

protected:
    /**
     * @brief Allows a subclass to choose whether to queue a file or not during scanDir.
     * 
     * @param name A potential filename in the queue directory
     * 
     * @return true to add the file to the queue or false to not queue the file.
     * 
     * This would allow a subclass to do some validation of the file before adding
     * it to the queue when reading the files from disk after reboot.
     */
    virtual bool preScanAddHook(const char *name) { return true; };

    /**
     * @brief Lock the mutex used to protect the queue
     */
    void queueMutexLock() const;

    /**
     * @brief Unlock the mutex used to protect the queue
     */
    void queueMutexUnlock() const;

protected:
    /**
     * @brief The path to the queue directory. Must be configured, using the top level directory is not allowed
     */
    String dirPath;

    /**
     * @brief Filename pattern, used with snprintf and sscanf to parse and generate filenames.
     */
    String pattern = "%08d";

    /**
     * @brief Filename extension, without the dot. May be an empty string for no extension.
     */
    String filenameExtension = "";

    /**
     * @brief Set to true after scanDir() is called
     */
    bool scanDirCompleted = false;

    /**
     * @brief Last file number used.
     * 
     * Set during scanDir() and incremented by reserveFile(). May be updated by addFileToQueue() if
     * you didn't reserveFile() first.
     */
    int lastFileNum = 0;

    /**
     * @brief Mutex used to protect queue
     */
    mutable os_mutex_t queueMutex = 0;

    /**
     * @brief Queue of files
     * 
     * Items are added using scanDir() and addFileToQueue(). Removed using getFileFromQueue().
     */
    std::deque<int> queue;
};

#endif // __SEQUENTIALFILERK_H
