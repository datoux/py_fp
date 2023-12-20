/*
Copyright (c) 2023 Daniel Turecek <daniel@turecek.de>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef FILELOG_H
#define FILELOG_H
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>

#ifdef TESTING
#define TESTING_VIRTUAL virtual
#else
#define TESTING_VIRTUAL
#endif

#ifndef WIN32
#include <sys/time.h>
#endif

#define FILELOG_DEFLEN 512

using namespace std::chrono;

enum LogLevel { LOG_FATAL_ERR = 0, LOG_ERR, LOG_MSG, LOG_DBG};

static const char* LOG_PREFIX[] = {"FAIL", "!ERR", " MSG", "DBG"};

class FileLog {
  public:
    FileLog(const char* logFileName = "log.log", bool logToFile = true, bool logToStdout = true, LogLevel logLevel = LOG_ERR)
        : mLogFile(NULL)
        , mLogFileName(logFileName)
        , mLastLogMsg("")
        , mLogToFile(logToFile)
        , mLogToStdout(logToStdout)
        , mLogLevel(logLevel)
        , mMaxLogBufferSize(250)
    {
        if (mLogToFile) {
            openFile(!fileExists(mLogFileName.c_str()));
            if (mLogFile)
                fprintf(mLogFile,
                        "########################################### LOG OPENED (%s) "
                        "########################################### \n",
                        currentTime().c_str());
            closeFile();
        }
    }

    virtual ~FileLog()
    {
        if (mLogFile)
            fclose(mLogFile);
    }

    int openFile(bool create = false)
    {
#ifdef _MSC_VER
        if (fopen_s(&mLogFile, mLogFileName.c_str(), create ? "w" : "a"))
            mLogFile = NULL;
#else
        mLogFile = fopen(mLogFileName.c_str(), create ? "w" : "a");
#endif
        return mLogFile == NULL;
    }

    void closeFile()
    {
        if (mLogFile)
            fclose(mLogFile);
        mLogFile = NULL;
    }

    int log(int err, LogLevel logLevel, const char* text, ...)
    {
        va_list args;
        va_start(args, text);
        log(logLevel, text, args);
        va_end(args);
        return err;
    }

    int log(LogLevel logLevel, const char* text, ...)
    {
        va_list args;
        va_start(args, text);
        int rc = log(logLevel, text, args);
        va_end(args);
        return rc;
    }

    int log(LogLevel logLevel, const char* text, va_list args)
    {
        int size = FILELOG_DEFLEN;
        std::string str;
        while (1) {
            str.resize(size);
#ifdef _MSC_VER
            int n = _vsnprintf_s((char*)str.c_str(), size, size - 1, text, args);
#else
            int n = vsnprintf((char*)str.c_str(), size, text, args);
#endif
            if (n > -1 && n < size)
                break;
            size = n > -1 ? n + 1 : 2 * size;
        }
        return log(0, logLevel, str);
    }

    int log(int err, LogLevel logLevel, std::string text)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (logLevel > mLogLevel)
            return 0;

        if (mLogToStdout) {
            printf("(%s) [%s]: %s\n", currentTime().c_str(), LOG_PREFIX[logLevel], text.c_str());
            fflush(stdout);
        }

        if (mLogToFile && !openFile()) {
            fprintf(mLogFile, "(%s) [%s]: %s\n", currentTime().c_str(), LOG_PREFIX[logLevel], text.c_str());
            fflush(mLogFile);
            closeFile();
        }
        mLastLogMsg = text;
        return err;
    }

    int logTextBuffer(int err, LogLevel logLevel, const char* text, const char* prefix = NULL)
    {
        log(err, logLevel, prefix ? prefix : "Buffer:");
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (logLevel > mLogLevel)
                return 0;

            if (mLogToStdout) {
                printf("(%s) [%s]: %s\n", currentTime().c_str(), LOG_PREFIX[logLevel], text);
                fflush(stdout);
            }

            if (mLogToFile && !openFile()) {
                fprintf(mLogFile, "(%s) [%s]: %s\n", currentTime().c_str(), LOG_PREFIX[logLevel], text);
                fflush(mLogFile);
                closeFile();
            }
        }
        return err;
    }

    int logBuffer(LogLevel logLevel, char* buffer, size_t size, const char* prefix = NULL,
                  bool showAsciiTranscript = true)
    {
        return logBuffer(0, logLevel, buffer, size, prefix, showAsciiTranscript);
    }

    int logBuffer(int err, LogLevel logLevel, char* buffer, size_t size, const char* prefix = NULL,
                  bool showAsciiTranscript = true)
    {
        log(err, logLevel, prefix ? prefix : "Buffer:");
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (logLevel > mLogLevel)
                return 0;

            if (mLogToStdout) {
                logBuffer(stdout, (unsigned char*)buffer, size, mMaxLogBufferSize, showAsciiTranscript);
            }

            if (mLogToFile && !openFile()) {
                logBuffer(mLogFile, (unsigned char*)buffer, size, mMaxLogBufferSize, showAsciiTranscript);
                closeFile();
            }
        }
        return err;
    }

    void logNoTime(LogLevel logLevel, const char* text)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (logLevel > mLogLevel)
            return;

        if (mLogToStdout) {
            printf("%s", text);
            fflush(stdout);
        }

        if (mLogToFile && !openFile()) {
            fprintf(mLogFile, "%s", text);
            fflush(mLogFile);
            closeFile();
        }
    }

    int rotateLog(const char* newFileName)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        int rc = std::rename(mLogFileName.c_str(), newFileName);
        if (rc == 0) {
            openFile(!fileExists(mLogFileName.c_str()));
            if (mLogFile)
                fprintf(mLogFile, "################# LOG OPENED (%s) ###################### \n", currentTime().c_str());
            closeFile();
        }
        return rc;
    }

  public:
    void setLogToFile(bool logToFile) { mLogToFile = logToFile; }
    void setLogToStdout(bool logToStdout) { mLogToStdout = logToStdout; }
    void setLogLevel(int logLevel) { mLogLevel = logLevel; }
    void setMaxLogBufferSize(size_t size) { mMaxLogBufferSize = size; }
    bool isLoggingToFile() { return mLogToFile; }
    bool isLoggingToStdout() { return mLogToStdout; }
    std::string getLogFileName() { return mLogFileName; }
    std::string getLastMessage() { return mLastLogMsg; }

  protected:
    TESTING_VIRTUAL std::chrono::system_clock::time_point getTimeNow()
    {
        return system_clock::now();
    }

    std::string currentTime()
    {
        auto timeNow = getTimeNow();
        std::time_t timet = system_clock::to_time_t(timeNow);
        auto millis = duration_cast<milliseconds>(timeNow.time_since_epoch()).count() - timet * 1000;
        const size_t size = 30;
        char str[size] = {0};
        std::strftime(str, size, "%d-%m-%y %H:%M:%S.", std::localtime(&timet));
        return std::string(str) + std::to_string(millis);
    }

    void logBuffer(FILE* file, unsigned char* data, size_t size, size_t showSize = 250, bool asciiTrans = true)
    {
        const size_t asciiOffset = 100;
        const size_t lineSize = 256;
        char line[lineSize] = {0};
        bool skipped = false;

        if (size > showSize)
            showSize /= 2;

        for (size_t i = 0; i < size; i += 32) {

            if (i >= showSize && !skipped) {
                skipped = true;
                fprintf(file, "                                          ----- DATA SKIPPED -----\n");
                if (size - showSize > i)
                    i = size - showSize;
            }

            memset(line, ' ', asciiOffset);

            // HEX bytes first 16
            for (size_t j = 0; (j < 16) && (j + i < size); j++)
                snprintf(&line[j * 3], 4, "%02x ", data[i + j]);
            line[strlen(line)] = ' ';

            // HEX bytes next 16
            for (size_t j = 16; (j < 32) && (j + i < size); j++)
                snprintf(&line[j * 3 + 3], 4, "%02X ", data[i + j]);
            line[strlen(line)] = ' ';

            if (asciiTrans) {
                memset(line + asciiOffset, 0, lineSize - asciiOffset);
                line[strlen(line)] = '|';
                line[strlen(line)] = ' ';

                // first 16 chars
                for (size_t j = 0; (j < 16) && (j + i < size); j++) {
                    unsigned char b = data[i + j];
                    line[asciiOffset + 2 + j] = (b > 0x20 && b < 0x80) ? b : '.';
                }

                line[strlen(line)] = ' ';
                line[strlen(line)] = ' ';

                // next 16 chars
                for (size_t j = 16; (j < 32) && (j + i < size); j++) {
                    unsigned char b = data[i + j];
                    line[asciiOffset + 2 + j + 1] = (b > 0x20 && b < 0x80) ? b : '.';
                }
            }
            else {
                line[asciiOffset] = '\0';
            }

            fprintf(file, "   %s \n", line);
        }

        if (size > 64)
            fprintf(file, "   Bytes: %u\n", (unsigned)size);

        fprintf(file, "\n");
    }

    bool fileExists(std::string path)
    {
        std::error_code err;
        return std::filesystem::exists(path, err);
    }

  protected:
    std::mutex mMutex;
    FILE* mLogFile;
    std::string mLogFileName;
    std::string mLastLogMsg;
    bool mLogToFile;
    bool mLogToStdout;
    int mLogLevel;
    size_t mMaxLogBufferSize;
};

#endif // FILELOG_H

