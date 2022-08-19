
# class SequentialFile 

Class for maintaining a directory of files as a queue with unique filenames.

You select a directory on the Gen 3 flash file system for use as the queue directory. This directory is filled with numbered files, for example: 00000001.jpg. It's possible to store meta information in additional files. For example, 00000001.sha1 to hold a SHA-1 hash of the file. The file are numbered sequentially.

You typically instantiate this class as a global variable.

In global setup(), configure it using the withXXX() methods, then call the scanDir() method to read the queue from disk. In particular, you must use withDirPath() to set the queue directory!

Files left on disk are added to the queue during scanDir() and an in-memory queue is initialized with the files to process.

Code can reserve a space in the queue using reserveFile(). This returns the next available file number but does not create the file.

Once you have fully written the file, call addFileToQueue().

The code that processes files in the queue calls getFileFromQueue().

## Members

---

###  SequentialFile::SequentialFile() 

Default constructor.

```
 SequentialFile()
```

This object is often created as a global variable.

In global setup(), configure it using the withXXX() methods, then call the scanDir() method to read the queue from disk.

---

###  SequentialFile::~SequentialFile() 

Destructor.

```
virtual  ~SequentialFile()
```

This object is often created as a global variables and never deleted.

---

### SequentialFile & SequentialFile::withDirPath(const char * dirPath) 

Sets the directory to use as the queue directory. This is required!

```
SequentialFile & withDirPath(const char * dirPath)
```

#### Parameters
* `dirPath` the pathname, Unix-style with / as the directory separator.

Typically you create your queue either at the top level ("/myqueue") or in /usr ("/usr/myqueue"). The directory will be created if necessary, however only one level of directory will be created. The parent must already exist.

The dirPath can end with a slash or not, but if you include it, it will be removed.

You must call this as you cannot use the root directory as a queue!

---

### const char * SequentialFile::getDirPath() const 

Gets the directory path set using withDirPath()

```
const char * getDirPath() const
```

The returned path will not end with a slash.

---

### SequentialFile & SequentialFile::withPattern(const char * pattern) 

Sets the filename pattern. Default is %08d (8-digit decimal number with leading zeros)

```
SequentialFile & withPattern(const char * pattern)
```

#### Parameters
* `pattern` The sprintf/sscanf pattern to use

This string is used when scanning the queue directory to find files. Do not include the filename extension in this pattern!

---

### const char * SequentialFile::getPattern() const 

Gets the filename pattern.

```
const char * getPattern() const
```

---

### SequentialFile & SequentialFile::withFilenameExtension(const char * ext) 

Filename extension for queue files. (Default: no extension)

```
SequentialFile & withFilenameExtension(const char * ext)
```

#### Parameters
* `ext` The filename extension (just the extension, no preceding dot)

For example, for JPEG files it might be "jpg".

---

### const char * SequentialFile::getFilenameExtension() const 

Gets the filename extension.

```
const char * getFilenameExtension() const
```

Returned string does not contain the dot. For example, for JPEG files it might be "jpg".

---

### bool SequentialFile::scanDir(void) 

Scans the queue directory for files. Typically called during setup().

```
bool scanDir(void)
```

---

### int SequentialFile::reserveFile(void) 

Reserve a file number you will use to write data to.

```
int reserveFile(void)
```

Use getPathForFileNum() to get the pathname to the file. Reservations are in-RAM only so if the device reboots before you write the file, the reservation will be lost.

---

### void SequentialFile::addFileToQueue(int fileNum) 

Adds a previously reserved file to the queue.

```
void addFileToQueue(int fileNum)
```

Use reserveFile() to get the next file number, addFileToQueue() to add it to the queue and getFileFromQueue() to get an item from the queue.

It's safe to call reserveFile(), addFileToQueue(), and getFileFromQueue() from different threads. Locking is handled internally.

---

### int SequentialFile::getFileFromQueue(void) 

Gets a file from the queue.

```
int getFileFromQueue(void)
```

#### Returns
0 if there are no items in the queue, or a fileNum for an item in the queue.

Use getPathForFileNum() to convert the number into a pathname for use with open().

The queue is stored in RAM, so if the device reboots before you delete the file it will reappear in the queue when scanDir is called. This method does not need to access the filesystem. You can call getFileFromQueue() on every loop if you want to to check if there are files to process without affecting performance.

It's safe to call reserveFile(), addFileToQueue(), and getFileFromQueue() from different threads. Locking is handled internally.

---

### String SequentialFile::getNameForFileNum(int fileNum, const char * overrideExt) 

Uses pattern to create a filename given a fileNum.

```
String getNameForFileNum(int fileNum, const char * overrideExt)
```

#### Parameters
* `fileNum` A file number, typically from reserveFile() or getFileFromQueue()

* `overrideExt` If non-null, use this extension instead of the configured filename extension.

The overrideExt is used when you create multiple files per queue entry fileNum, for example a data file and a .sha1 hash for the file. Or other metadata.

---

### String SequentialFile::getPathForFileNum(int fileNum, const char * overrideExt) 

Gets a full pathname based on dirName and getNameForFileNum.

```
String getPathForFileNum(int fileNum, const char * overrideExt)
```

#### Parameters
* `fileNum` A file number, typically from reserveFile() or getFileFromQueue()

* `overrideExt` If non-null, use this extension instead of the configured filename extension.

The overrideExt is used when you create multiple files per queue entry fileNum, for example a data file and a .sha1 hash for the file. Or other metadata.

---

### void SequentialFile::removeFileNum(int fileNum, bool allExtensions) 

Remove fileNum from the flash file system.

```
void removeFileNum(int fileNum, bool allExtensions)
```

#### Parameters
* `fileNum` A file number, typically from reserveFile() or getFileFromQueue()

* `allExtensions` If true, all files with that number regardless of extension are removed.

---

### void SequentialFile::removeAll(bool removeDir) 

Removes all of the files in the queue directory.

```
void removeAll(bool removeDir)
```

removeDir true to remove the queue directory itself, false to just remove the contents.

Note this is all files, not just the filenames with the matching extension.

---

### int SequentialFile::getQueueLen() const 

Gets the length of the queue.

```
int getQueueLen() const
```

---

###  SequentialFile::SequentialFile(const SequentialFile &) 

This class is not copyable.

```
 SequentialFile(const SequentialFile &) = delete
```

---

### SequentialFile & SequentialFile::operator=(const SequentialFile &) 

This class is not copyable.

```
SequentialFile & operator=(const SequentialFile &) = delete
```

Generated by [Moxygen](https://sourcey.com/moxygen)