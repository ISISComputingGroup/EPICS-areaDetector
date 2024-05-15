/*
 * Api.cpp
 *
 * Revamped pixium area detector driver.
 *
 * Virtual base class for the PU API library
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */

#include "DllApi.h"
#include <ctime>
#include "TraceStream.h"
#include "Pco.h"

/* Constants */
const double DllApi::ccdTemperatureScaleFactor = 10.0;
const double DllApi::timebaseScaleFactor[DllApi::numTimebases] =
    {1000000000.0, 1000000.0, 1000.0};

/**
 * Constructor
 */
DllApi::DllApi(Pco* pco, TraceStream* trace)
: pco(pco)
, trace(trace)
, stopped(true)
{
    this->pco->registerDllApi(this);
}

/**
 * Destructor
 */
DllApi::~DllApi()
{
}

/**
 * Connect to the camera
 */
void DllApi::openCamera(Handle* handle, unsigned short camNum) throw(PcoException)
{
    int result = doOpenCamera(handle, camNum);
    this->trace->printf("DllApi->OpenCamera(%p, %hu) = 0x%x\n",
            handle, camNum, result);
    if(result != DllApi::errorNone)
    {
        throw PcoException("openCamera", result);
    }
}

/**
 * Disconnect from the camera
 */
void DllApi::closeCamera(Handle handle) throw(PcoException)
{
    int result = doCloseCamera(handle);
    *this->trace << "DllApi->CloseCamera(" << handle << ") = " <<
            result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("closeCamera", result);
    }
}

/**
 * Reboot the camera
 */
void DllApi::rebootCamera(Handle handle) throw(PcoException)
{
    int result = doRebootCamera(handle);
    *this->trace << "DllApi->RebootCamera(" << handle << ") = " <<
            result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("rebootCamera", result);
    }
}

/**
 * Get general information from the camera
 */
void DllApi::getGeneral(Handle handle) throw(PcoException)
{
    int result = doGetGeneral(handle);
    this->trace->printf("DllApi->GetGeneral(%p) = 0x%x\n",
            handle, result);
    if(result != DllApi::errorNone)
    {
        throw PcoException("getGeneral", result);
    }
}

/**
 * Get the camera type information
 */
void DllApi::getCameraType(Handle handle, CameraType* cameraType) throw(PcoException)
{
    int result = doGetCameraType(handle, cameraType);
    *this->trace << "DllApi->GetCameraType(" << handle << ", " <<
            cameraType->camType << ", " << cameraType->serialNumber << ", " << 
            cameraType->hardwareVersion << ", " << cameraType->firmwareVersion << 
            ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getCameraType", result);
    }
}

/**
 * Get the firmware versions of all devices in the camera
 */
void DllApi::getFirmwareInfo(Handle handle, std::vector<PcoCameraDevice> &devices) throw(PcoException)
{
    int result = doGetFirmwareInfo(handle, devices);
    if(result != DllApi::errorNone)
    {
        throw PcoException("getFirmwareInfo", result);
    }
}

/**
 * Get sensor information
 */
void DllApi::getSensorStruct(Handle handle) throw(PcoException)
{
    int result = doGetSensorStruct(handle);
    *this->trace << "DllApi->GetSensorStruct(" << handle << ") = " <<
            result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getSensorStruct", result);
    }
}

/**
 * Get timing information
 */
void DllApi::getTimingStruct(Handle handle) throw(PcoException)
{
    int result = doGetTimingStruct(handle);
    *this->trace << "DllApi->GetTimingStruct(" << handle << ") = " <<
            result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getTimingStruct", result);
    }
}

/**
 * Get camera description
 */
void DllApi::getCameraDescription(Handle handle, Description* description) throw(PcoException)
{
    int result = doGetCameraDescription(handle, description);
    *this->trace << "DllApi->GetCameraDescription(" << handle << ", {" <<
        description->maxHorzRes << "," << description->maxVertRes << ", " <<
        description->maxBinHorz << "," << description->maxBinVert << ", " <<
        description->binHorzStepping << "," << description->binVertStepping << ", " <<
        description->roiHorSteps << "," << description->roiVertSteps << ", {" <<
        description->pixelRate[0] << "," << description->pixelRate[1] << "," <<
        description->pixelRate[2] << "," << description->pixelRate[3] << "}, " <<
        description->convFact << "," << description->minCoolingSetpoint << "," <<
        description->maxCoolingSetpoint << "," << description->defaultCoolingSetpoint <<
        ", " << std::hex << description->generalCaps << std::dec <<
        "}) = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getCameraDescription", result);
    }
}

/**
 * Get camera storage information
 */
void DllApi::getStorageStruct(Handle handle, Storage* storage) throw(PcoException)
{
    int result = doGetStorageStruct(handle, storage);
    *this->trace << "DllApi->GetStorageStruct(" << handle << ", " <<
        storage->ramSizePages << ", " << storage->pageSizePixels << ", {" <<
        storage->segmentSizePages[0] << ", " << storage->segmentSizePages[1] << ", " <<
        storage->segmentSizePages[2] << ", " << storage->segmentSizePages[3] << "}, " <<
        storage->activeSegment << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getStorageStruct", result);
    }
}

/**
 * Get camera recording information
 */
void DllApi::getRecordingStruct(Handle handle) throw(PcoException)
{
    int result = doGetRecordingStruct(handle);
    *this->trace << "DllApi->GetRecordingStruct(" << handle << ") = " <<
            result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getRecordingStruct", result);
    }
}

/**
 * Reset the camera's settings
 */
void DllApi::resetSettingsToDefault(Handle handle) throw(PcoException)
{
    int result = doResetSettingsToDefault(handle);
    *this->trace << "DllApi->ResetSettingsToDefault(" << handle << ") = " <<
            result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("resetSettingsToDefault", result);
    }
}

/**
 * Get the camera's transfer parameters
 */
void DllApi::getTransferParameters(Handle handle, Transfer* transfer) throw(PcoException)
{
    int result = doGetTransferParameters(handle, transfer);
    *this->trace << "DllApi->GetTransferParameters(" << handle << ", " <<
        transfer->baudRate << ", " << transfer->clockFrequency << ", " <<
        transfer->camlinkLines << ", " << transfer->dataFormat << ", " <<
        transfer->transmit << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getTransferParameters", result);
    }
}

/**
 * Set the camera's transfer parameters
 */
void DllApi::setTransferParameters(Handle handle, Transfer* transfer) throw(PcoException)
{
    int result = doSetTransferParameters(handle, transfer);
    *this->trace << "DllApi->SetTransferParameters(" << handle << ", " <<
        transfer->baudRate << ", " << transfer->clockFrequency << ", " <<
        transfer->camlinkLines << ", " << transfer->dataFormat << ", " <<
        transfer->transmit << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setTransferParameters", result);
    }
}

/**
 * The the camera's current and maximum resolutions
 */
void DllApi::getSizes(Handle handle, Sizes* sizes) throw(PcoException)
{
    int result = doGetSizes(handle, sizes);
    *this->trace << "DllApi->GetSizes(" << handle << ", " <<
        sizes->xResActual << ", " << sizes->yResActual << ", " <<
        sizes->xResMaximum << ", " << sizes->yResMaximum << ") = " <<
        result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getSizes", result);
    }
}

/**
 * Set the camera's date and time
 */
void DllApi::setDateTime(Handle handle, struct tm* currentTime) throw(PcoException)
{
    int result = doSetDateTime(handle, currentTime);
    *this->trace << "DllApi->SetDateTime(" << handle << ", " <<
        currentTime->tm_mday << ", " << currentTime->tm_mon << ", " <<
        currentTime->tm_year << ", " << currentTime->tm_hour << ", " <<
        currentTime->tm_min << ", " << currentTime->tm_sec << ") = " <<
        result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setDateTime", result);
    }
}

/**
 * Get camera temperatures
 */
void DllApi::getTemperature(Handle handle, short* ccd,
        short* camera, short* psu) throw(PcoException)
{
    int result = doGetTemperature(handle, ccd, camera, psu);
    *this->trace << "DllApi->GetTemperature(" << handle << ", " <<
        *ccd << ", " << *camera << ", " << *psu << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getTemperature", result);
    }
}

/**
 * Set the camera sensor cooling setpoint.
 */
void DllApi::setCoolingSetpoint(Handle handle, short setPoint) throw(PcoException)
{
    int result = doSetCoolingSetpoint(handle, setPoint);
    *this->trace << "DllApi->SetCoolingSetpoint(" << handle << ", " <<
        setPoint << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setCoolingSetpoint", result);
    }
}

/**
 * Get the camera sensor cooling setpoint.
 */
void DllApi::getCoolingSetpoint(Handle handle, short* setPoint) throw(PcoException)
{
    int result = doGetCoolingSetpoint(handle, setPoint);
    *this->trace << "DllApi->GetCoolingSetpoint(" << handle << ", " <<
        *setPoint << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getCoolingSetpoint", result);
    }
}

/**
 * Set the camera's operating pixel rate
 */
void DllApi::setPixelRate(Handle handle, unsigned long pixRate) throw(PcoException)
{
    int result = doSetPixelRate(handle, pixRate);
    *this->trace << "DllApi->SetPixelRate(" << handle << ", " <<
        pixRate << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setPixelRate", result);
    }
}

/**
 * Get the camera's operating pixel rate
 */
void DllApi::getPixelRate(Handle handle, unsigned long* pixRate) throw(PcoException)
{
    int result = doGetPixelRate(handle, pixRate);
    *this->trace << "DllApi->GetPixelRate(" << handle << ", " <<
        *pixRate << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getPixelRate", result);
    }
}

/**
 * Set the operating bit alignment
 */
void DllApi::setBitAlignment(Handle handle, unsigned short bitAlignment) throw(PcoException)
{
    int result = doSetBitAlignment(handle, bitAlignment);
    *this->trace << "DllApi->SetBitAlignment(" << handle << ", " <<
        bitAlignment << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setBitAlignment", result);
    }
}

/**
 * Get the operating bit alignment
 */
void DllApi::getBitAlignment(Handle handle, unsigned short* bitAlignment) throw(PcoException)
{
    int result = doGetBitAlignment(handle, bitAlignment);
    *this->trace << "DllApi->GetBitAlignment(" << handle << ", " <<
        *bitAlignment << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getBitAlignment", result);
    }
}

/**
 * Return an Edge's camera setup information
 */
void DllApi::getCameraSetup(Handle handle, unsigned short* setupType,
        unsigned long* setupData, unsigned short* setupDataLen) throw(PcoException)
{
    int result = doGetCameraSetup(handle, setupType, setupData, setupDataLen);
    *this->trace << "DllApi->GetCameraSetup(" << handle << ", " <<
        *setupType << ", " << setupData[0] << ", " << *setupDataLen << ") = " <<
        result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getCameraSetup", result);
    }
}

/**
 * Set an Edge's camera setup information
 */
void DllApi::setCameraSetup(Handle handle, unsigned short setupType,
        unsigned long* setupData, unsigned short setupDataLen) throw(PcoException)
{
    int result = doSetCameraSetup(handle, setupType, setupData, setupDataLen);
    *this->trace << "DllApi->SetCameraSetup(" << handle << ", " <<
        setupType << ", " << setupData[0] << ", " << setupDataLen << ") = " <<
        result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setCameraSetup", result);
    }
}

/**
 * Set the binning parameters.
 */
void DllApi::setBinning(Handle handle, unsigned short binHorz, unsigned short binVert) throw(PcoException)
{
    int result = doSetBinning(handle, binHorz, binVert);
    *this->trace << "DllApi->SetBinning(" << handle << ", " <<
        binHorz << ", " << binVert << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setBinning", result);
    }
}

/**
 * Get the binning parameters.
 */
void DllApi::getBinning(Handle handle, unsigned short* binHorz, unsigned short* binVert) throw(PcoException)
{
    int result = doGetBinning(handle, binHorz, binVert);
    *this->trace << "DllApi->GetBinning(" << handle << ", " <<
        *binHorz << ", " << *binVert << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getBinning", result);
    }
}

/**
 * Set the region of interest parameters
 */
void DllApi::setRoi(Handle handle, unsigned short x0, unsigned short y0,
        unsigned short x1, unsigned short y1) throw(PcoException)
{
    int result = doSetRoi(handle, x0, y0, x1, y1);
    this->trace->printf("DllApi->SetRoi(%p, %hu, %hu, %hu, %hu) = 0x%x\n",
            handle, x0, y0, x1, y1, result);
    if(result != DllApi::errorNone)
    {
        throw PcoException("setRoi", result);
    }
}

/**
 * Get the region of interest parameters
 */
void DllApi::getRoi(Handle handle, unsigned short* x0, unsigned short* y0,
        unsigned short* x1, unsigned short* y1) throw(PcoException)
{
    int result = doGetRoi(handle, x0, y0, x1, y1);
    *this->trace << "DllApi->GetRoi(" << handle << ", " <<
        *x0 << "," << *y0 << ", " << *x1 << "," << *y1 << ") = " <<
        result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getRoi", result);
    }
}

/**
 * Set the trigger mode
 */
void DllApi::setTriggerMode(Handle handle, unsigned short mode) throw(PcoException)
{
    int result = doSetTriggerMode(handle, mode);
    *this->trace << "DllApi->SetTriggerMode(" << handle << ", " <<
        mode << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setTriggerMode", result);
    }
}

/**
 * Get the trigger mode
 */
void DllApi::getTriggerMode(Handle handle, unsigned short* mode) throw(PcoException)
{
    int result = doGetTriggerMode(handle, mode);
    *this->trace << "DllApi->GetTriggerMode(" << handle << ", " <<
        *mode << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getTriggerMode", result);
    }
}

/**
 * Set the storage mode
 */
void DllApi::setStorageMode(Handle handle, unsigned short mode) throw(PcoException)
{
    int result = doSetStorageMode(handle, mode);
    *this->trace << "DllApi->SetStorageMode(" << handle << ", " <<
        mode << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setStorageMode", result);
    }
}

/**
 * Get the storage mode
 */
void DllApi::getStorageMode(Handle handle, unsigned short* mode) throw(PcoException)
{
    int result = doGetStorageMode(handle, mode);
    *this->trace << "DllApi->GetStorageMode(" << handle << ", " <<
        *mode << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getStorageMode", result);
    }
}

/**
 * Set the time stamp mode
 */
void DllApi::setTimestampMode(Handle handle, unsigned short mode) throw(PcoException)
{
    int result = doSetTimestampMode(handle, mode);
    *this->trace << "DllApi->SetTimestampMode(" << handle << ", " <<
        mode << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setTimestampMode", result);
    }
}

/**
 * Get the time stamp mode
 */
void DllApi::getTimestampMode(Handle handle, unsigned short* mode) throw(PcoException)
{
    int result = doGetTimestampMode(handle, mode);
    *this->trace << "DllApi->GetTimestampMode(" << handle << ", " <<
        *mode << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getTimestampMode", result);
    }
}

/**
 * Set the acquire mode
 */
void DllApi::setAcquireMode(Handle handle, unsigned short mode) throw(PcoException)
{
    int result = doSetAcquireMode(handle, mode);
    *this->trace << "DllApi->SetAcquireMode(" << handle << ", " <<
        mode << ", " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setAcquireMode", result);
    }
}

/**
 * Get the acquire mode
 */
void DllApi::getAcquireMode(Handle handle, unsigned short* mode) throw(PcoException)
{
    int result = doGetAcquireMode(handle, mode);
    *this->trace << "DllApi->GetAcquireMode(" << handle << ", " <<
        *mode << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getAcquireMode", result);
    }
}

/**
 * Set the delay and exposure times
 */
void DllApi::setDelayExposureTime(Handle handle, unsigned long delay,
        unsigned long exposure, unsigned short timeBaseDelay,
        unsigned short timeBaseExposure) throw(PcoException)
{
    int result = doSetDelayExposureTime(handle, delay, exposure,
            timeBaseDelay, timeBaseExposure);
    *this->trace << "DllApi->SetDelayExposureTime(" << handle << ", " <<
        delay << ", " << exposure << ", " << timeBaseDelay << ", " <<
        timeBaseExposure << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setDelayExposureTime", result);
    }
}

/**
 * Get the delay and exposure times
 */
void DllApi::getDelayExposureTime(Handle handle, unsigned long* delay,
        unsigned long* exposure, unsigned short* timeBaseDelay,
        unsigned short* timeBaseExposure) throw(PcoException)
{
    int result = doGetDelayExposureTime(handle, delay, exposure,
            timeBaseDelay, timeBaseExposure);
    *this->trace << "DllApi->GetDelayExposureTime(" << handle << ", " <<
        *delay << ", " << *exposure << ", " << *timeBaseDelay << ", " <<
        *timeBaseExposure << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getDelayExposureTime", result);
    }
}

/**
 * Set the sensor gain
 */
void DllApi::setConversionFactor(Handle handle, unsigned short factor) throw(PcoException)
{
    int result = doSetConversionFactor(handle, factor);
    *this->trace << "DllApi->SetConversionFactor(" << handle << ", " <<
        factor << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setConversionFactor", result);
    }
}

/**
 * Get the ADC operating mode
 */
void DllApi::getAdcOperation(Handle handle, unsigned short* mode) throw(PcoException)
{
    int result = doGetAdcOperation(handle, mode);
    *this->trace << "DllApi->SetAdcOperation(" << handle << ", " <<
        *mode << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getAdcOperation", result);
    }
}

/**
 * Set the ADC operating mode
 */
void DllApi::setAdcOperation(Handle handle, unsigned short mode) throw(PcoException)
{
    int result = doSetAdcOperation(handle, mode);
    *this->trace << "DllApi->SetAdcOperation(" << handle << ", " <<
        mode << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setAdcOperation", result);
    }
}

/**
 * Get the camera's recording state
 */
void DllApi::getRecordingState(Handle handle, unsigned short* state) throw(PcoException)
{
    int result = doGetRecordingState(handle, state);
    *this->trace << "DllApi->GetRecordingState(" << handle << ", " <<
        *state << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getRecordingState", result);
    }
}

/**
 * Set the camera's recording state
 */
void DllApi::setRecordingState(Handle handle, unsigned short state) throw(PcoException)
{
    int result = doSetRecordingState(handle, state);
    this->trace->printf("DllApi->SetRecordingState(%p, %hu) = 0x%x\n",
            handle, state, result);
    if(result != DllApi::errorNone)
    {
        throw PcoException("setRecordingState", result);
    }
}

/**
 * Get the recorder submode
 */
void DllApi::getRecorderSubmode(Handle handle, unsigned short* mode) throw(PcoException)
{
    int result = doGetRecorderSubmode(handle, mode);
    *this->trace << "DllApi->GetRecorderSubmode(" << handle << ", " <<
        *mode << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getRecorderSubmode", result);
    }
}

/**
 * Set the recorder submode.
 */
void DllApi::setRecorderSubmode(Handle handle, unsigned short mode) throw(PcoException)
{
    int result = doSetRecorderSubmode(handle, mode);
    *this->trace << "DllApi->SetRecorderSubmode(" << handle <<
        ", " << mode << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setRecorderSubmode", result);
    }
}

/**
 * Allocate a buffer
 */
void DllApi::allocateBuffer(Handle handle, short* bufferNumber, unsigned long size,
        unsigned short** buffer, Handle* event) throw(PcoException)
{
    int result = doAllocateBuffer(handle, bufferNumber, size, buffer, event);
    *this->trace << "DllApi->AllocateBuffer(" << handle << ", " <<
        *bufferNumber << ", " << size << ", " << *buffer << ", " << event << ") = " <<
        result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("allocateBuffer", result);
    }
}

/**
 * Cancel all image buffers
 */
void DllApi::cancelImages(Handle handle) throw(PcoException)
{
    int result = doCancelImages(handle);
    *this->trace << "DllApi->CancelImages(" << handle <<
        ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("cancelImages", result);
    }
}

/**
 * Set the image parameters for the image buffer transfer inside the CamLink and GigE interface.
 * While using CamLink or GigE this function must be called, before the user tries to get images
 * from the camera and the sizes have changed. With all other interfaces this is a dummy call.
 */
void DllApi::camlinkSetImageParameters(Handle handle, unsigned short xRes, unsigned short yRes)
    throw(PcoException)
{
    int result = doCamlinkSetImageParameters(handle, xRes, yRes);
    *this->trace << "DllApi->CamlinkSetImageParameters(" << handle <<
        ", " << xRes << ", " << yRes << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("camlinkSetImageParameters", result);
    }
}

/**
 * Arm the camera ready for taking images.
 */
void DllApi::arm(Handle handle) throw(PcoException)
{
    int result = doArm(handle);
    this->trace->printf("DllApi->Arm(%p) = 0x%x\n",
            handle, result);
    if(result != DllApi::errorNone)
    {
        throw PcoException("arm", result);
    }
}

/**
 * Add a buffer to the receive queue.
 */
void DllApi::addBufferEx(Handle handle, unsigned long firstImage, unsigned long lastImage, 
    short bufferNumber, unsigned short xRes, unsigned short yRes, 
    unsigned short bitRes) throw(PcoException)
{
    int result = doAddBufferEx(handle, firstImage, lastImage, bufferNumber, xRes, yRes, bitRes);
    this->trace->printf("DllApi->AddBufferEx(%p, %lu, %lu, %hd, %hu, %hu, %hu) = 0x%x\n",
            handle, firstImage, lastImage, bufferNumber, xRes, yRes, bitRes, result);
    if(result != DllApi::errorNone)
    {
        throw PcoException("addBufferEx", result);
    }
}

/**
 * Get an image from memory
 */
void DllApi::getImageEx(Handle handle, unsigned short segment, unsigned long firstImage,
        unsigned long lastImage, short bufferNumber, unsigned short xRes, 
        unsigned short yRes, unsigned short bitRes) throw(PcoException)
{
    int result = doGetImageEx(handle, segment, firstImage, lastImage, 
            bufferNumber, xRes, yRes, bitRes);
    this->trace->printf("DllApi->GetImageEx(%p, %hu, %lu, %lu, %hd, %hu, %hu, %hu) = 0x%x\n",
            handle, segment, firstImage, lastImage, bufferNumber, xRes, yRes, bitRes, result);
    if(result != DllApi::errorNone)
    {
        throw PcoException("getImageEx", result);
    }
}

/**
 * Get the status of a buffer.
 */
void DllApi::getBufferStatus(Handle handle, short bufferNumber, unsigned long* statusDll, 
    unsigned long* statusDrv) throw(PcoException)
{
    int result = doGetBufferStatus(handle, bufferNumber, statusDll, statusDrv);
    this->trace->printf("DllApi->GetBufferStatus(%p, %hd, %lx, %lx) = 0x%x\n",
            handle, bufferNumber, *statusDll, *statusDrv, result);
    if(result != DllApi::errorNone)
    {
        throw PcoException("getBufferStatus", result);
    }
}

/**
 * Force a trigger if one not already running
 */
void DllApi::forceTrigger(Handle handle, unsigned short* triggered) throw(PcoException)
{
    int result = doForceTrigger(handle, triggered);
    *this->trace << "DllApi->ForceTrigger(" << handle <<
        ", " << *triggered << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("forceTrigger", result);
    }
}

/**
 * Free a buffer.  Need to do this before closing the camera.
 */
void DllApi::freeBuffer(Handle handle, short bufferNumber) throw(PcoException)
{
    int result = doFreeBuffer(handle, bufferNumber);
    *this->trace << "DllApi->FreeBuffer(" << handle <<
        ", " << bufferNumber << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("freeBuffer", result);
    }
}

/**
 * Get the active RAM segment.
 */
void DllApi::getActiveRamSegment(Handle handle, unsigned short* segment) throw(PcoException)
{
    int result = doGetActiveRamSegment(handle, segment);
    *this->trace << "DllApi->GetActiveRamSegment(" << handle <<
        ", " << *segment << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getActiveRamSegment", result);
    }
}

/**
 * Set the active RAM segment.
 */
void DllApi::setActiveRamSegment(Handle handle, unsigned short segment) throw(PcoException)
{
    int result = doSetActiveRamSegment(handle, segment);
    *this->trace << "DllApi->SetActiveRamSegment(" << handle <<
        ", " << segment << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setActiveRamSegment", result);
    }
}

/**
 * Get the number of images in a segment.
 */
void DllApi::getNumberOfImagesInSegment(Handle handle, unsigned short segment,
        unsigned long* validImageCount, unsigned long* maxImageCount) throw(PcoException)
{
    int result = doGetNumberOfImagesInSegment(handle, segment, validImageCount,
            maxImageCount);
    *this->trace << "DllApi->GetNumberOfImagesInSegment(" << handle <<
        ", " << segment << ", " << *validImageCount << ", " << *maxImageCount <<
        ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getNumberOfImagesInSegment", result);
    }
}

/**
 * Set the active lookup table
 */
void DllApi::setActiveLookupTable(Handle handle, unsigned short identifier) throw(PcoException)
{
    int result = doSetActiveLookupTable(handle, identifier);
    *this->trace << "DllApi->SetActiveLookupTable(" << handle << ", " <<
        identifier << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setActiveLookupTable", result);
    }
}

/**
 * Set the driver timeouts
 */
void DllApi::setTimeouts(Handle handle, unsigned int commandTimeout,
        unsigned int imageTimeout, unsigned int transferTimeout)
{
    int result = doSetTimeouts(handle, commandTimeout, imageTimeout, transferTimeout);
    this->trace->printf("DllApi->SetTimeouts(%p, %u, %u, %u) = 0x%x\n",
            handle, commandTimeout, imageTimeout, transferTimeout, result);
    if(result != DllApi::errorNone)
    {
        throw PcoException("setTimeouts", result);
    }
}

/**
 * Clear active RAM segment
 */
void DllApi::clearRamSegment(Handle handle)
{
    int result = doClearRamSegment(handle);
    this->trace->printf("DllApi->ClearRamSegment(%p) = 0x%x\n",
            handle, result);
    if(result != DllApi::errorNone)
    {
        throw PcoException("clearRamSegment", result);
    }
}

/**
 * Get the RAM size
 */
void DllApi::getCameraRamSize(Handle handle, unsigned long* numPages, unsigned short* pageSize)
{
    int result = doGetCameraRamSize(handle, numPages, pageSize);
    *this->trace << "DllApi->GetCameraRamSize(" << handle << ", " <<
        *numPages << ", " << *pageSize << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getCameraRamSize", result);
    }
}

/**
 * Get camera health status
 */
void DllApi::getCameraHealthStatus(Handle handle, unsigned long* warnings, unsigned long* errors,
        unsigned long* status)
{
    int result = doGetCameraHealthStatus(handle, warnings, errors, status);
    *this->trace << "DllApi->GetCameraHealthStatus(" << handle << ", " <<
        *warnings << ", " << *errors << ", " << *status << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getCameraHealthStatus", result);
    }
}

/**
 * Get camera busy status
 */
void DllApi::getCameraBusyStatus(Handle handle, unsigned short* status)
{
    int result = doGetCameraBusyStatus(handle, status);
    *this->trace << "DllApi->GetCameraBusyStatus(" << handle << ", " <<
        *status << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getCameraBusyStatus", result);
    }
}

/**
 * Get exposure trigger status
 */
void DllApi::getExpTrigSignalStatus(Handle handle, unsigned short* status)
{
    int result = doGetExpTrigSignalStatus(handle, status);
    *this->trace << "DllApi->GetExpTrigSignalStatus(" << handle << ", " <<
        *status << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getExpTrigSignalStatus", result);
    }
}

/**
 * Get acquisition enable status
 */
void DllApi::getAcqEnblSignalStatus(Handle handle, unsigned short* status)
{
    int result = doGetAcqEnblSignalStatus(handle, status);
    *this->trace << "DllApi->GetAcqEnblSignalStatus(" << handle << ", " <<
        *status << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("getAcqEnblSignalStatus", result);
    }
}

/**
 * Set the sensor format.
 */
void DllApi::setSensorFormat(Handle handle, unsigned short format) throw(PcoException)
{
    int result = doSetSensorFormat(handle, format);
    *this->trace << "DllApi->SetSensorFormat(" << handle <<
        ", " << format << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setSensorFormat", result);
    }
}

/**
 * Set double image mode.
 */
void DllApi::setDoubleImageMode(Handle handle, unsigned short mode) throw(PcoException)
{
    int result = doSetDoubleImageMode(handle, mode);
    *this->trace << "DllApi->SetDoubleImageMode(" << handle <<
        ", " << mode << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setDoubleImageMode", result);
    }
}

/**
 * Set offset mode.
 */
void DllApi::setOffsetMode(Handle handle, unsigned short mode) throw(PcoException)
{
    int result = doSetOffsetMode(handle, mode);
    *this->trace << "DllApi->SetOffsetMode(" << handle <<
        ", " << mode << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setOffsetMode", result);
    }
}

/**
 * Set noise filter mode.
 */
void DllApi::setNoiseFilterMode(Handle handle, unsigned short mode) throw(PcoException)
{
    int result = doSetNoiseFilterMode(handle, mode);
    *this->trace << "DllApi->SetNoiseFilterMode(" << handle <<
        ", " << mode << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setNoiseFilterMode", result);
    }
}

/**
 * Set the camera RAM segment size.
 */
void DllApi::setCameraRamSegmentSize(Handle handle, unsigned long seg1,
    unsigned long seg2, unsigned long seg3, unsigned long seg4) throw(PcoException)
{
    int result = doSetCameraRamSegmentSize(handle, seg1, seg2, seg3, seg4);
    *this->trace << "DllApi->SetCameraRamSegmentSize(" << handle <<
        ", " << seg1 << ", " << seg2 << ", " << seg3 << ", " << seg4 << ") = " << result << std::endl;
    if(result != DllApi::errorNone)
    {
        throw PcoException("setCameraRamSegmentSize", result);
    }
}

/**
 * Start the frame capturing loop
 */
void DllApi::startFrameCapture(bool useGetImage)
{
    doStartFrameCapture(useGetImage);
    *this->trace << "DllApi->startFrameCapture(" << useGetImage << ")" << std::endl;
    this->stopped = false;
}

/**
 * Stop the frame capturing loop
 */
void DllApi::stopFrameCapture()
{
    this->stopped = true;
    doStopFrameCapture();
    *this->trace << "DllApi->stopFrameCapture()" << std::endl;
}

/**
 * Is the frame capture stopped?
 */
bool DllApi::isStopped()
{
    return this->stopped;
}

