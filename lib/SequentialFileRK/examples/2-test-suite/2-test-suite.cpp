#include "SequentialFileRK.h"

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>


SerialLogHandler logHandler(LOG_LEVEL_TRACE);

SYSTEM_THREAD(ENABLED);

SequentialFile sequentialFile;
bool doReset = false;

int testHandler(String cmd);

void setup() {
    Particle.function("test", testHandler);

    // Wait for a USB serial connection for up to 10 seconds
    waitFor(Serial.isConnected, 10000);
    delay(1000);

    sequentialFile 
        .withDirPath("/usr/seqtest1")
        .withFilenameExtension("jpg")
        .scanDir();
}

void loop() {
    if (doReset) {
        Log.info("resetting...");
        delay(1000);
        System.reset();
    }
}


int testHandler(String param) {
    String cmd;
    String arg;

    int spaceIndex = param.indexOf(' ');
    if (spaceIndex > 0) {
        cmd = param.substring(0, spaceIndex).trim();
        arg = param.substring(spaceIndex + 1).trim();
    }
    else {
        cmd = param.trim();
    }

    cmd = cmd.toLowerCase();
    if (cmd.equals("list")) {
        // List files in queue directory
        DIR *dir = opendir(sequentialFile.getDirPath());
        if (dir) {
            while(true) {
                struct dirent* ent = readdir(dir); 
                if (!ent) {
                    break;
                }
                
                Log.info("%s", ent->d_name);
            }
            closedir(dir);
        }    
        else {
            Log.info("directory does not exist %s", sequentialFile.getDirPath());
        }
    }
    else
    if (cmd.equals("scan")) {
        sequentialFile.scanDir();
    }
    else
    if (cmd.equals("cd")) {
        if (arg.length() > 0) {
            sequentialFile.withDirPath(arg).scanDir();
        }
        else {
            Log.info("arg required (path to use as queue directory");
        }
    }
    else
    if (cmd.equals("ext")) {
        sequentialFile.withFilenameExtension(arg).scanDir();
    }
    else 
    if (cmd.equals("reserve")) {
        int fileNum = sequentialFile.reserveFile();
        Log.info("reserveFile returned %d", fileNum);
    }
    else 
    if (cmd.equals("create")) {
        int fileNum = arg.toInt();
        if (fileNum != 0) {
            int fd = open(sequentialFile.getPathForFileNum(fileNum), O_RDWR | O_CREAT);
            if (fd) {
                close(fd);
            }
        }
        else {
            Log.info("arg required (fileNum)");
        }
    }
    else 
    if (cmd.equals("create2")) {
        int fileNum = arg.toInt();
        if (fileNum != 0) {
            int fd = open(sequentialFile.getPathForFileNum(fileNum), O_RDWR | O_CREAT);
            if (fd) {
                close(fd);
            }
            fd = open(sequentialFile.getPathForFileNum(fileNum, "sha1"), O_RDWR | O_CREAT);
            if (fd) {
                close(fd);
            }
        }
        else {
            Log.info("arg required (fileNum)");
        }
    }
    else
    if (cmd.equals("add")) {
        int fileNum = arg.toInt();
        if (fileNum != 0) {
            sequentialFile.addFileToQueue(fileNum);
        }
        else {
            Log.info("arg required (fileNum)");
        }
    }
    else
    if (cmd.equals("queue")) {
        int fileNum = sequentialFile.getFileFromQueue();
        Log.info("getFileFromQueue returned %d", fileNum);
    }
    else 
    if (cmd.equals("rm")) {
        int fileNum = arg.toInt();
        if (fileNum != 0) {
            sequentialFile.removeFileNum(fileNum, false);
        }
        else {
            Log.info("arg required (fileNum)");
        }
    }
    else 
    if (cmd.equals("rm2")) {
        int fileNum = arg.toInt();
        if (fileNum != 0) {
            sequentialFile.removeFileNum(fileNum, true);
        }
        else {
            Log.info("arg required (fileNum)");
        }
    }
    else 
    if (cmd.equals("rmdir")) {
        sequentialFile.removeAll(true);
    }
    else 
    if (cmd.equals("reset")) {
        doReset = true;
    }
    else {
        Log.info("unknown command");
    }


    return 0;
}
