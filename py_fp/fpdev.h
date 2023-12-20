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
#ifndef FPDEV_H
#define FPDEV_H
#include <string>
#include <vector>
#include "common.h"

#define FPERR_LIBRARY_NOT_FOUND  -100
#define FPERR_ALREADY_OPENED     -101
#define FPERR_CANNOT_OPEN        -102
#define FPERR_FPG_CFG_FAILED     -103
#define FPERR_FP_NOT_ENABLED     -104
#define FPERR_NOT_CONNECTED      -105

namespace OpalKellyLegacy{
class okCFrontPanel;
}

struct FPDevInfo {
    std::string devSerial;
    std::string deviceID;
};

class FPDev
{
public:
    FPDev();
    virtual ~FPDev();
    static int loadFrontPanelLibrary(const char* path);
    static std::vector<std::string> listDevices();
    static std::vector<FPDevInfo> listDevicesInfo();
    static std::string deviceID(const char* serial);

public:
    int open(const char* serial, const char* firmwareFile);
    int close();
    bool isOpen() const;
    std::string getDeviceID() const;
    void setDeviceID(const char deviceID[32]);
    void setCloseOnFailure(bool closeOnFailure) { mCloseOnFailure = closeOnFailure; }
    int resetDevice();
    int setWireIn(u32 address, u32 value, bool sendNow=true);
    i64 getWireOut(u32 address, bool refreshWireOuts=true);
    int writeRegister(u32 address, u32 value);
    i64 readRegister(u32 address);
    int writePipe(u32 address, byte* data, size_t size, size_t blockSize=1024);
    i64 readPipe(u32 address, byte* data, size_t size, size_t blockSize=1024);
    int setTimeout(u32 timeout);

public:
    static std::string libraryDate() { return mLibDate; }
    std::string serial() const { return mSerial; }
    std::string deviceID() const { return mDeviceID; }
    std::string fpFirmwareVersion() const { return mFpFirmwareVersion; }
    bool isUSB3Speed() const { return mIsUSB3Speed; }
    std::string lastError() const { return mLastError; }

private:
    static std::string mLibDate;
    OpalKellyLegacy::okCFrontPanel* mFp;
    std::string mFpFirmwareVersion;
    std::string mSerial;
    std::string mDeviceID;
    bool mIsUSB3Speed;
    bool mCloseOnFailure;
    mutable std::string mLastError;
};


#endif /* !FPDEV_H */

