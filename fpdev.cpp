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
#define NOMINMAX
#include "fpdev.h"
#include <cmath>
#include <algorithm>

#include "buffer.h"
#include "okFrontPanelDLL.h"
#include "strutils.h"


#define CHECK_CONNECTED \
    if (!mFp){ \
        mLastError = "Device not connected.";\
        return FPERR_NOT_CONNECTED; \
    }

std::string FPDev::mLibDate;

FPDev::FPDev()
    : mFp(NULL)
    , mIsUSB3Speed(false)
    , mCloseOnFailure(false)
{

}

FPDev::~FPDev()
{

}

int FPDev::loadFrontPanelLibrary(const char* path)
{
    (void)path;
    if (!okFrontPanelDLL_LoadLib(NULL))
        return FPERR_LIBRARY_NOT_FOUND;
    char dllDate[32], dllTime[32];
    okFrontPanelDLL_GetVersion(dllDate, dllTime);
    mLibDate = str::format("%s %s", dllDate, dllTime);
    return 0;
}

std::vector<std::string> FPDev::listDevices()
{
    std::vector<std::string> devs;
    okCFrontPanel* fp = new okCFrontPanel();
    int deviceCount = fp->GetDeviceCount();
    for (int i = 0; i < deviceCount; i++)
        devs.push_back(fp->GetDeviceListSerial(i));
    delete fp;
    return devs;
}

std::vector<FPDevInfo> FPDev::listDevicesInfo()
{
    std::vector<FPDevInfo> devs;
    okCFrontPanel* fp = new okCFrontPanel();
    int deviceCount = fp->GetDeviceCount();
    for (int i = 0; i < deviceCount; i++){
        FPDevInfo info;
        info.devSerial = fp->GetDeviceListSerial(i);

        if (fp->OpenBySerial(info.devSerial) != okCFrontPanel::NoError)
            continue;

        okTDeviceInfo devInfo;
        fp->GetDeviceInfo(&devInfo);
        fp->Close();

        info.deviceID = std::string(devInfo.deviceID);
        devs.push_back(info);
    }
    delete fp;
    return devs;
}

std::string FPDev::deviceID(const char* serial)
{
    OpalKellyLegacy::okCFrontPanel* fp = new okCFrontPanel();
    if (fp->OpenBySerial(std::string(serial)) != okCFrontPanel::NoError) {
        delete fp;
        return "";
    }

    okTDeviceInfo devInfo;
    fp->GetDeviceInfo(&devInfo);
    fp->Close();
    delete fp;
    return devInfo.deviceID;
}

int FPDev::open(const char* serial, const char* firmwareFile)
{
    if (mFp){
        mLastError = "Cannot open: Device alraedy opened.";
        return FPERR_ALREADY_OPENED;
    }

    mFp = new okCFrontPanel();
    if (mFp->OpenBySerial(std::string(serial)) != okCFrontPanel::NoError) {
        delete mFp;
        mFp = NULL;
        mLastError = "Device could not be opened.";
        return FPERR_CANNOT_OPEN;
    }

    okTDeviceInfo devInfo;
    mFp->GetDeviceInfo(&devInfo);
    mFpFirmwareVersion = str::format("Firmware %d.%d", devInfo.deviceMajorVersion, devInfo.deviceMinorVersion);
    mDeviceID = devInfo.deviceID;
    mSerial = devInfo.serialNumber;
    mIsUSB3Speed = devInfo.usbSpeed == OK_USBSPEED_SUPER;

    mFp->LoadDefaultPLLConfiguration();

    if (firmwareFile){
        if (mFp->ConfigureFPGA(firmwareFile) != okCFrontPanel::NoError) {
            mFp->Close();
            delete mFp;
            mFp = NULL;
            mLastError = "FPG configuration failed.";
            return FPERR_FPG_CFG_FAILED;
        }
    }

    if (!mFp->IsFrontPanelEnabled()){
        mFp->Close();
        delete mFp;
        mFp = NULL;
        mLastError = "FrontPanel support is not enabled.";
        return FPERR_FP_NOT_ENABLED;
    }

    return 0;
}

int FPDev::setTimeout(u32 timeout)
{
    mFp->SetTimeout((u32)timeout);
    return 0;
}

int FPDev::close()
{
    if (mFp){
        mFp->Close();
        delete mFp;
        mFp = NULL;
    }
    return 0;
}

bool FPDev::isOpen() const
{
    if (!mFp)
        return false;
    return mFp->IsOpen();
}

int FPDev::resetDevice()
{
    CHECK_CONNECTED;
    return mFp->ResetFPGA();
}

std::string FPDev::getDeviceID() const
{
    if (!mFp){
        mLastError = "Device not connected";
        return "";
    }
    return mFp->GetDeviceID();
}

void FPDev::setDeviceID(const char deviceID[32])
{
    if (!mFp)
        mLastError = "Device not connected";
    mFp->SetDeviceID(deviceID);
}

int FPDev::setWireIn(u32 address, u32 value, bool sendNow)
{
    CHECK_CONNECTED;
    int rc = mFp->SetWireInValue(address, value);
    if (rc == okCFrontPanel::Failed && mCloseOnFailure){
        close();
        return rc;
    }
    if (sendNow)
        mFp->UpdateWireIns();
    return rc;
}

i64 FPDev::getWireOut(u32 address, bool refreshWireOuts)
{
    CHECK_CONNECTED;
    if (refreshWireOuts)
        mFp->UpdateWireOuts();
    u32 value = static_cast<u32>(mFp->GetWireOutValue(address));
    return value;
}

int FPDev::writeRegister(u32 address, u32 value)
{
    CHECK_CONNECTED;
    int rc = mFp->WriteRegister(address, value);
    if (rc == okCFrontPanel::Failed && mCloseOnFailure)
        close();
    return rc;
}

i64 FPDev::readRegister(u32 address)
{
    CHECK_CONNECTED;
    u32 value = 0;
    int rc = mFp->ReadRegister(address, &value);
    if (rc == okCFrontPanel::Failed && mCloseOnFailure)
        close();
    return rc ? static_cast<i64>(rc) : static_cast<i64>(value);
}

int FPDev::writePipe(u32 address, byte* data, size_t size, size_t blockSize)
{
    CHECK_CONNECTED;
    int rc = 0;
    if (size % blockSize != 0){
        u32 writeSize = (u32)(ceil(size / (double)blockSize) * blockSize);
        writeSize = std::max((u32)blockSize, writeSize);
        Buffer<byte> buff(writeSize);
        memcpy(buff.data(), data, size);
        rc = static_cast<int>(mFp->WriteToBlockPipeIn(address, static_cast<u32>(blockSize), writeSize, buff.data()));
    }else
        rc = static_cast<int>(mFp->WriteToBlockPipeIn(address, static_cast<u32>(blockSize), (long)size, data));

    if (rc == okCFrontPanel::Failed && mCloseOnFailure)
        close();
    return rc;
}

i64 FPDev::readPipe(u32 address, byte* data, size_t size, size_t blockSize)
{
    CHECK_CONNECTED;
    i64 rc = 0;
    if (size % blockSize != 0){
        size_t readSize = (size_t)(ceil(size / (double)blockSize) * blockSize);
        readSize = std::max((size_t)blockSize, readSize);
        Buffer<byte> buff(readSize);
        rc = static_cast<i64>(mFp->ReadFromBlockPipeOut(address, static_cast<int>(blockSize), static_cast<long>(readSize), buff.data()));
        if (rc == static_cast<i64>(readSize)){
            memcpy(data, buff.data(), size);
            rc = size;
        }
    }else
        rc = static_cast<i64>(mFp->ReadFromBlockPipeOut(address, static_cast<int>(blockSize), (long)size, data));

    if (rc == (i64)okCFrontPanel::Failed && mCloseOnFailure)
        close();
    return rc;
}


