/*
 * PcoApi.cpp
 *
 * Revamped PCO area detector driver.
 *
 * An interface to the PCO API library and hardware
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */

#include "PcoApi.h"
#include "TraceStream.h"
#include "Pco.h"
#include "TakeLock.h"
#include "epicsExport.h"
#include "iocsh.h"
#include "sc2_SDKStructures.h"
#include "SC2_SDKAddendum.h"
#include "sc2_defs.h"
#include "PCO_err.h"
#define PCO_ERRT_H_CREATE_OBJECT
#include "PCO_errt.h"
#include "load.h"
#include "SC2_CamExport.h"

/**
 * Constructor
 */
PcoApi::PcoApi(Pco* pco, TraceStream* trace)
: DllApi(pco, trace)
, thread(*this, "PcoApi", epicsThreadGetStackSize(epicsThreadStackMedium))
, buffersValid(false)
, useGetImage(false)
{
    // Initialise the buffer information
    for(int i=0; i<DllApi::maxNumBuffers; i++)
    {
        this->buffers[i].addedToDll = false;
        this->buffers[i].eventHandle = NULL;
    }
    // Create the start/stop events
    this->startEvent = ::CreateEvent(NULL, TRUE, FALSE, "StartEvent");
    this->stopEvent = ::CreateEvent(NULL, TRUE, FALSE, "StopEvent");
    // Start the thread
    this->thread.start();
}

/**
 * Destructor
 */
PcoApi::~PcoApi()
{
    ::CloseHandle(this->startEvent);
    ::CloseHandle(this->stopEvent);
}

/**
 * The thread that handles buffer events
 */
void PcoApi::run()
{
    while(true)
    {
        ::ResetEvent(this->startEvent);
        // Wait for the start event
        HANDLE waitEvents[PcoApi::numberOfWaitingEvents] = {this->startEvent};
        DWORD result = ::WaitForMultipleObjects(PcoApi::numberOfWaitingEvents,
            waitEvents, FALSE, INFINITE);
        ::ResetEvent(this->startEvent);
        if(result == WAIT_OBJECT_0)
        {
            *trace << "#### Entering run event loop" << std::endl;
            ::ResetEvent(this->stopEvent);
            bool running = true;
            while(running)
            {
                if(this->useGetImage)
                {
                    // Check for stopped
                    if(::WaitForSingleObject(this->stopEvent, 100) == WAIT_OBJECT_0)
                    {
                        // Stop event received
                        ::ResetEvent(this->stopEvent);
                        running = false;
                        *trace << "#### Exiting run event loop" << std::endl;
                    }
                    else
                    {
                        // Try to get a frame
                        this->pco->getFrames();
                    }
                }
                else
                {
                    // Wait for an image or the stop event
                    HANDLE runEvents[PcoApi::numberOfRunningEvents];
                    runEvents[PcoApi::stopEventIndex] = this->stopEvent;
                    for(int i=0; i<DllApi::maxNumBuffers; i++)
                    {
                        if(this->buffers[i].eventHandle != NULL)
                        {
                            runEvents[PcoApi::firstBufferEventIndex+i] = this->buffers[i].eventHandle;
                        }
                    }
                    result = ::WaitForMultipleObjects(PcoApi::numberOfRunningEvents,
                        runEvents, FALSE, PcoApi::waitTimeoutMs);
                    {
                        // What should we do?
                        if(result == WAIT_TIMEOUT)
                        {
                            // Do a poll
                            this->pco->pollForFrames();
                        }
                        else if(::WaitForSingleObject(this->stopEvent, 0) == WAIT_OBJECT_0)
                        {
                            // Stop event received
                            ::ResetEvent(this->stopEvent);
                            running = false;
                            *trace << "#### Exiting run event loop" << std::endl;
                        }
                        else if(result >= WAIT_OBJECT_0+PcoApi::firstBufferEventIndex &&
                            result < WAIT_OBJECT_0+PcoApi::firstBufferEventIndex+DllApi::maxNumBuffers)
                        {
                            // Handle a buffer ready
                            int eventBufferNumber = result - WAIT_OBJECT_0 - PcoApi::firstBufferEventIndex;
                            this->pco->frameReceived(eventBufferNumber);
                        }
                        else
                        {
                            // Faulty exit reason
                            *trace << "#### Unhandled wait result " << result << std::endl;
                            this->pco->frameWaitFault();
                        }
                    }
                }
            }
        }
    }
}

/**
 * Connect to the camera
 * Camera number is currently ignored.
 */
int PcoApi::doOpenCamera(Handle* handle, unsigned short camNum)
{
    int result = PCO_OpenCamera(handle, camNum);
    this->handle = *handle;
    return result;
}

/**
 * Disconnect from the camera
 */
int PcoApi::doCloseCamera(Handle handle)
{
    return PCO_CloseCamera(handle);
}

/**
 * Reboot the camera
 */
int PcoApi::doRebootCamera(Handle handle)
{
    return PCO_RebootCamera(handle);
}

/**
 * Get general information from the camera
 */
int PcoApi::doGetGeneral(Handle handle)
{
    PCO_General info;
    info.wSize = sizeof(info);
    info.strCamType.wSize = sizeof(info.strCamType);
    return PCO_GetGeneral(handle, &info);
}

/**
 * Get the camera type information
 */
int PcoApi::doGetCameraType(Handle handle, CameraType* cameraType)
{
    PCO_CameraType info;
    info.wSize = sizeof(info);
    int result = PCO_GetCameraType(handle, &info);
    cameraType->camType = info.wCamType;
    cameraType->serialNumber = info.dwSerialNumber;
    cameraType->hardwareVersion = info.dwHWVersion;
    cameraType->firmwareVersion = info.dwFWVersion;
    cameraType->interfaceType = info.wInterfaceType;
    return result;
}

/**
 * Get the firmware versions of all devices
 */
int PcoApi::doGetFirmwareInfo(Handle handle, std::vector<PcoCameraDevice> &devices)
{

    PCO_FW_Vers firmwareVersion;
    int maxDevicesPerBlock = 10;
    int currentBlock = 0;

    // Get the first block of devices
    int result = PCO_GetFirmwareInfo(handle, currentBlock, &firmwareVersion);
    int deviceNum = firmwareVersion.DeviceNum;

    // Total number of device blocks
    int numBlocks = deviceNum/maxDevicesPerBlock + 1;

    // Iterate over all device blocks
    int numDevicesInBlock;
    while (currentBlock < numBlocks) {

        // Get number of devices in the selected block
        if (currentBlock+1 == numBlocks) {
            numDevicesInBlock = deviceNum%maxDevicesPerBlock;
        }
        else {
            numDevicesInBlock = maxDevicesPerBlock;
        }

        // Parse devices in current block
        for (int i=0; i<numDevicesInBlock; i++)
        {
            // Get the information
            std::string deviceName(firmwareVersion.Device[i].szName);
            int variant = (int)firmwareVersion.Device[i].wVariant;
            int majorVersion = (int)firmwareVersion.Device[i].bMajorRev;
            int minorVersion = (int)firmwareVersion.Device[i].bMinorRev;

            // Add the device to our list
            devices.push_back(PcoCameraDevice(deviceName, majorVersion, minorVersion, variant));
        }

        // Increment block counter
        currentBlock += 1;

        // Check for the next block
        if (currentBlock < numBlocks) {
            result = PCO_GetFirmwareInfo(handle, currentBlock, &firmwareVersion);
        }

    }

    return result;
}

/**
 * Get sensor information
 */
int PcoApi::doGetSensorStruct(Handle handle)
{
    PCO_Sensor info;
    info.wSize = sizeof(info);
    info.strDescription.wSize = sizeof(info.strDescription);
    return PCO_GetSensorStruct(handle, &info);
}

/**
 * Get timing information
 */
int PcoApi::doGetTimingStruct(Handle handle)
{
    PCO_Timing info;
    info.wSize = sizeof(info);
    for(int i=0; i<NUM_MAX_SIGNALS; i++)
    {
        info.strSignal[i].wSize = sizeof(info.strSignal[i]);
    }
    return PCO_GetTimingStruct(handle, &info);
}

/**
 * Get camera description
 */
int PcoApi::doGetCameraDescription(Handle handle, Description* description)
{
    PCO_Description info;
    info.wSize = sizeof(info);
    int result = PCO_GetCameraDescription(handle, &info);
    if(result == DllApi::errorNone)
    {
        description->maxHorzRes = info.wMaxHorzResStdDESC;
        description->maxVertRes = info.wMaxVertResStdDESC;
        description->dynResolution = info.wDynResDESC;
        description->maxBinHorz = info.wMaxBinHorzDESC;
        description->maxBinVert = info.wMaxBinVertDESC;
        description->binHorzStepping = info.wBinHorzSteppingDESC;
        description->binVertStepping = info.wBinVertSteppingDESC;
        description->roiHorSteps = info.wRoiHorStepsDESC;
        description->roiVertSteps = info.wRoiVertStepsDESC;
        description->pixelRate[0] = info.dwPixelRateDESC[0];
        description->pixelRate[1] = info.dwPixelRateDESC[1];
        description->pixelRate[2] = info.dwPixelRateDESC[2];
        description->pixelRate[3] = info.dwPixelRateDESC[3];
        description->convFact = info.wConvFactDESC[0];
        description->generalCaps = info.dwGeneralCapsDESC1;
        description->minCoolingSetpoint = info.sMinCoolSetDESC;
        description->maxCoolingSetpoint = info.sMaxCoolSetDESC;
        description->defaultCoolingSetpoint = info.sDefaultCoolSetDESC;
        description->minDelayNs = info.dwMinDelayDESC;
        description->maxDelayMs = info.dwMaxDelayDESC;
        description->minDelayStepNs = info.dwMinDelayStepDESC;
        description->minExposureNs = info.dwMinExposureDESC;
        description->maxExposureMs = info.dwMaxExposureDESC;
        description->minExposureStepNs = info.dwMinExposureStepDESC;
    }
    return result;
}

/**
 * Get camera storage information
 */
int PcoApi::doGetStorageStruct(Handle handle, Storage* storage)
{
    PCO_Storage info;
    info.wSize =  sizeof(info);
    int result = PCO_GetStorageStruct(handle, &info);
    if(result == DllApi::errorNone)
    {
        storage->ramSizePages = info.dwRamSize;
        storage->pageSizePixels = info.wPageSize;
        for(int i=0; i<DllApi::storageNumSegments; i++)
        {
            storage->segmentSizePages[i] = info.dwRamSegSize[i];
        }
        storage->activeSegment = info.wActSeg;
    }
    return result;
}

/**
 * Get camera recording information
 */
int PcoApi::doGetRecordingStruct(Handle handle)
{
    PCO_Recording info;
    info.wSize = sizeof(info);
    return PCO_GetRecordingStruct(handle, &info);
}

/**
 * Reset the camera's settings
 */
int PcoApi::doResetSettingsToDefault(Handle handle)
{
    return PCO_ResetSettingsToDefault(handle);
}

/**
 * Get the camera's transfer parameters
 */
int PcoApi::doGetTransferParameters(Handle handle, Transfer* transfer)
{
    _PCO_SC2_CL_TRANSFER_PARAMS info;
    int result = PCO_GetTransferParameter(handle, &info, sizeof(info));
    if(result == DllApi::errorNone)
    {
        transfer->baudRate = info.baudrate;
        transfer->clockFrequency = info.ClockFrequency;
        transfer->camlinkLines = info.CCline;
        transfer->dataFormat = info.DataFormat;
        transfer->transmit = info.Transmit;
    }
    return result;
}

/**
 * Set the camera's transfer parameters
 */
int PcoApi::doSetTransferParameters(Handle handle, Transfer* transfer)
{
    _PCO_SC2_CL_TRANSFER_PARAMS info;
    info.baudrate = transfer->baudRate;
    info.ClockFrequency = transfer->clockFrequency;
    info.CCline = transfer->camlinkLines;
    info.DataFormat = transfer->dataFormat;
    info.Transmit = transfer->transmit;
    return PCO_SetTransferParameter(handle, &info, sizeof(info));
}

/**
 * The the camera's current and maximum resolutions
 */
int PcoApi::doGetSizes(Handle handle, Sizes* sizes)
{
    return PCO_GetSizes(handle, &sizes->xResActual, &sizes->yResActual,
            &sizes->xResMaximum, &sizes->yResMaximum);
}

/**
 * Set the camera's date and time
 */
int PcoApi::doSetDateTime(Handle handle, struct tm* currentTime)
{
    return PCO_SetDateTime(handle, (BYTE)currentTime->tm_mday,
            (BYTE)(currentTime->tm_mon+1), (WORD)(currentTime->tm_year+1900),
            (WORD)currentTime->tm_hour, (BYTE)currentTime->tm_min,
            (BYTE)currentTime->tm_sec);
}

/**
 * Get camera temperatures
 */
int PcoApi::doGetTemperature(Handle handle, short* ccd,
        short* camera, short* psu)
{
    return PCO_GetTemperature(handle, ccd, camera, psu);
}

/**
 * Set the camera's cooling setpoint
 */
int PcoApi::doSetCoolingSetpoint(Handle handle, short setPoint)
{
    return PCO_SetCoolingSetpointTemperature(handle, setPoint);
}

/**
 * Get the camera's cooling setpoint.
 */
int PcoApi::doGetCoolingSetpoint(Handle handle, short* setPoint)
{
    return PCO_GetCoolingSetpointTemperature(handle, setPoint);
}

/**
 * Set the camera's operating pixel rate
 */
int PcoApi::doSetPixelRate(Handle handle, unsigned long pixRate)
{
    return PCO_SetPixelRate(handle, pixRate);
}

/**
 * Get the camera's operating pixel rate
 */
int PcoApi::doGetPixelRate(Handle handle, unsigned long* pixRate)
{
    return PCO_GetPixelRate(handle, pixRate);
}

/**
 * Get the operating bit alignment
 */
int PcoApi::doGetBitAlignment(Handle handle, unsigned short* bitAlignment)
{
    return PCO_GetBitAlignment(handle, bitAlignment);
}

/**
 * Set the operating bit alignment
 */
int PcoApi::doSetBitAlignment(Handle handle, unsigned short bitAlignment)
{
    return PCO_SetBitAlignment(handle, bitAlignment);
}

/**
 * Return an Edge's camera setup information
 */
int PcoApi::doGetCameraSetup(Handle handle, unsigned short* setupType,
        unsigned long* setupData, unsigned short* setupDataLen)
{
    return PCO_GetCameraSetup(handle, setupType, setupData, setupDataLen);
}

/**
 * Set an Edge's camera setup information
 */
int PcoApi::doSetCameraSetup(Handle handle, unsigned short setupType,
        unsigned long* setupData, unsigned short setupDataLen)
{
    return PCO_SetCameraSetup(handle, setupType, setupData, setupDataLen);
}

/**
 * Set the binning parameters.
 */
int PcoApi::doSetBinning(Handle handle, unsigned short binHorz, unsigned short binVert)
{
    return PCO_SetBinning(handle, binHorz, binVert);
}

/**
 * Get the binning parameters.
 */
int PcoApi::doGetBinning(Handle handle, unsigned short* binHorz, unsigned short* binVert)
{
    return PCO_GetBinning(handle, binHorz, binVert);
}

/**
 * Set the region of interest parameters
 */
int PcoApi::doSetRoi(Handle handle, unsigned short x0, unsigned short y0,
        unsigned short x1, unsigned short y1)
{
    return PCO_SetROI(handle, x0, y0, x1, y1);
}

/**
 * Get the region of interest parameters
 */
int PcoApi::doGetRoi(Handle handle, unsigned short* x0, unsigned short* y0,
        unsigned short* x1, unsigned short* y1)
{
    return PCO_GetROI(handle, x0, y0, x1, y1);
}

/**
 * Set the trigger mode
 */
int PcoApi::doSetTriggerMode(Handle handle, unsigned short mode)
{
    return PCO_SetTriggerMode(handle, mode);
}

/**
 * Get the trigger mode
 */
int PcoApi::doGetTriggerMode(Handle handle, unsigned short* mode)
{
    return PCO_GetTriggerMode(handle, mode);
}

/**
 * Set the storage mode
 */
int PcoApi::doSetStorageMode(Handle handle, unsigned short mode)
{
    return PCO_SetStorageMode(handle, mode);
}

/**
 * Get the storage mode
 */
int PcoApi::doGetStorageMode(Handle handle, unsigned short* mode)
{
    return PCO_GetStorageMode(handle, mode);
}

/**
 * Set the time stamp mode
 */
int PcoApi::doSetTimestampMode(Handle handle, unsigned short mode)
{
    return PCO_SetTimestampMode(handle, mode);
}

/**
 * Get the time stamp mode
 */
int PcoApi::doGetTimestampMode(Handle handle, unsigned short* mode)
{
    return PCO_GetTimestampMode(handle, mode);
}

/**
 * Set the acquire mode
 */
int PcoApi::doSetAcquireMode(Handle handle, unsigned short mode)
{
    return PCO_SetAcquireMode(handle, mode);
}

/**
 * Get the acquire mode
 */
int PcoApi::doGetAcquireMode(Handle handle, unsigned short* mode)
{
    return PCO_GetAcquireMode(handle, mode);
}

/**
 * Set the delay and exposure times
 */
int PcoApi::doSetDelayExposureTime(Handle handle, unsigned long delay,
        unsigned long exposure, unsigned short timeBaseDelay,
        unsigned short timeBaseExposure)
{
    return PCO_SetDelayExposureTime(handle, delay, exposure,
            timeBaseDelay, timeBaseExposure);
}

/**
 * Get the delay and exposure times
 */
int PcoApi::doGetDelayExposureTime(Handle handle, unsigned long* delay,
        unsigned long* exposure, unsigned short* timeBaseDelay,
        unsigned short* timeBaseExposure)
{
    return PCO_GetDelayExposureTime(handle, delay, exposure,
            timeBaseDelay, timeBaseExposure);
}

/**
 * Set the sensor gain
 */
int PcoApi::doSetConversionFactor(Handle handle, unsigned short factor)
{
    return PCO_SetConversionFactor(handle, factor);
}

/**
 * Get the ADC operating mode
 */
int PcoApi::doGetAdcOperation(Handle handle, unsigned short* mode)
{
    return PCO_GetADCOperation(handle, mode);
}

/**
 * Set the ADC operating mode
 */
int PcoApi::doSetAdcOperation(Handle handle, unsigned short mode)
{
    return PCO_SetADCOperation(handle, mode);
}

/**
 * Get the camera's recording state
 */
int PcoApi::doGetRecordingState(Handle handle, unsigned short* state)
{
    return PCO_GetRecordingState(handle, state);
}

/**
 * Set the camera's recording state
 */
int PcoApi::doSetRecordingState(Handle handle, unsigned short state)
{
    return PCO_SetRecordingState(handle, state);
}

/**
 * Get the recorder submode
 */
int PcoApi::doGetRecorderSubmode(Handle handle, unsigned short* mode)
{
    return PCO_GetRecorderSubmode(handle, mode);
}

/**
 * Set the recorder submode
 */
int PcoApi::doSetRecorderSubmode(Handle handle, unsigned short mode)
{
    return PCO_SetRecorderSubmode(handle, mode);
}

/**
 * Allocate a buffer
 */
int PcoApi::doAllocateBuffer(Handle handle, short* bufferNumber, unsigned long size,
        unsigned short** buffer, Handle* eventHandle)
{
    this->buffersValid = true;
    int result = PCO_AllocateBuffer(handle, bufferNumber, size, buffer, eventHandle);
    this->buffers[*bufferNumber].eventHandle = *eventHandle;
    return result;
}

/**
 * Cancel all image buffers
 */
int PcoApi::doCancelImages(Handle handle)
{
    this->buffersValid = false;
    int result = PCO_CancelImages(handle);
    for(int i=0; i<DllApi::maxNumBuffers; i++)
    {
        ::ResetEvent(this->buffers[i].eventHandle);
        this->buffers[i].addedToDll = false;
    }
    return result;
}

/**
 * Set the image parameters for the image buffer transfer inside the CamLink and GigE interface.
 * While using CamLink or GigE this function must be called, before the user tries to get images
 * from the camera and the sizes have changed. With all other interfaces this is a dummy call.
 */
int PcoApi::doCamlinkSetImageParameters(Handle handle, unsigned short xRes,
        unsigned short yRes)
{
    return PCO_CamLinkSetImageParameters(handle, xRes, yRes);
}

/**
 * Arm the camera ready for taking images.
 */
int PcoApi::doArm(Handle handle)
{
    return PCO_ArmCamera(handle);
}

/**
 * Add a buffer to the receive queue.
 */
int PcoApi::doAddBufferEx(Handle handle, unsigned long firstImage, unsigned long lastImage,
        short bufferNumber, unsigned short xRes, unsigned short yRes,
        unsigned short bitRes)
{
    if(this->buffersValid)
    {
        ::ResetEvent(this->buffers[bufferNumber].eventHandle);
        return PCO_AddBufferEx(handle, firstImage, lastImage, bufferNumber,
                xRes, yRes, bitRes);
    }
    else
    {
        return 0;
    }
}

/**
 * Get an image from memory.
 */
int PcoApi::doGetImageEx(Handle handle, unsigned short segment, unsigned long firstImage,
        unsigned long lastImage, short bufferNumber, unsigned short xRes, 
        unsigned short yRes, unsigned short bitRes)
{
    return PCO_GetImageEx(handle, segment, firstImage, lastImage, bufferNumber,
            xRes, yRes, bitRes);
}

/**
 * Get the status of a buffer
 */
int PcoApi::doGetBufferStatus(Handle handle, short bufferNumber,
        unsigned long* statusDll, unsigned long* statusDrv)
{
    return PCO_GetBufferStatus(handle, bufferNumber, statusDll, statusDrv);
}

/**
 * Force a trigger.
 */
int PcoApi::doForceTrigger(Handle handle, unsigned short* triggered)
{
    return PCO_ForceTrigger(handle, triggered);
}

/**
 * Free a buffer.
 */
int PcoApi::doFreeBuffer(Handle handle, short bufferNumber)
{
    return PCO_FreeBuffer(handle, bufferNumber);
}

/**
 * Get the active RAM segment
 */
int PcoApi::doGetActiveRamSegment(Handle handle, unsigned short* segment)
{
    return PCO_GetActiveRamSegment(handle, segment);
}

/**
 * Set the active RAM segment
 */
int PcoApi::doSetActiveRamSegment(Handle handle, unsigned short segment)
{
    return PCO_SetActiveRamSegment(handle, segment);
}

/**
 * Get the number of images in a segment
 */
int PcoApi::doGetNumberOfImagesInSegment(Handle handle, unsigned short segment,
        unsigned long* validImageCount, unsigned long* maxImageCount)
{
    return PCO_GetNumberOfImagesInSegment(handle, segment, validImageCount,
            maxImageCount);
}

/**
 * Set driver timeouts
 */
int PcoApi::doSetTimeouts(Handle handle, unsigned int commandTimeout,
        unsigned int imageTimeout, unsigned int transferTimeout)
{
    unsigned int buff[3];
    buff[0] = commandTimeout;
    buff[1] = imageTimeout;
    buff[2] = transferTimeout;
    return PCO_SetTimeouts(handle, &buff, sizeof(buff));
}

/**
 * Set the active lookup table
 */
int PcoApi::doSetActiveLookupTable(Handle handle, unsigned short identifier)
{
    unsigned short p1 = identifier;
    unsigned short p2 = 0;
    return PCO_SetActiveLookupTable(handle, &p1, &p2);
}

/**
 * Clear active RAM segment
 */
int PcoApi::doClearRamSegment(Handle handle)
{
    return PCO_ClearRamSegment(handle);
}

/**
 * Get the camera RAM size
 */
int PcoApi::doGetCameraRamSize(Handle handle, unsigned long* numPages, unsigned short* pageSize)
{
    return PCO_GetCameraRamSize(handle, numPages, pageSize);
}

/**
 * Get the camera health status
 */
int PcoApi::doGetCameraHealthStatus(Handle handle, unsigned long* warnings, unsigned long* errors,
            unsigned long* status)
{
    return PCO_GetCameraHealthStatus(handle, warnings, errors, status);
}

/**
 * Get the camera busy status
 */
int PcoApi::doGetCameraBusyStatus(Handle handle, unsigned short* status)
{
    return PCO_GetCameraBusyStatus(handle, status);
}

/**
 * Get the exposure trigger status
 */
int PcoApi::doGetExpTrigSignalStatus(Handle handle, unsigned short* status)
{
    return PCO_GetExpTrigSignalStatus(handle, status);
}

/**
 * Get the acquisition enable status
 */
int PcoApi::doGetAcqEnblSignalStatus(Handle handle, unsigned short* status)
{
    return PCO_GetAcqEnblSignalStatus(handle, status);
}

/**
 * Set the sensor format
 */
int PcoApi::doSetSensorFormat(Handle handle, unsigned short format)
{
    return PCO_SetSensorFormat(handle, format);
}

/**
 * Set the double image mode
 */
int PcoApi::doSetDoubleImageMode(Handle handle, unsigned short mode)
{
    return PCO_SetDoubleImageMode(handle, mode);
}

/**
 * Set the offset mode
 */
int PcoApi::doSetOffsetMode(Handle handle, unsigned short mode)
{
    return PCO_SetOffsetMode(handle, mode);
}

/**
 * Set the noise filter mode
 */
int PcoApi::doSetNoiseFilterMode(Handle handle, unsigned short mode)
{
    return PCO_SetNoiseFilterMode(handle, mode);
}

/**
 * Set the camera RAM segment size
 */
int PcoApi::doSetCameraRamSegmentSize(Handle handle, unsigned long seg1,
    unsigned long seg2, unsigned long seg3, unsigned long seg4)
{
    unsigned long segs[4] = {seg1, seg2, seg3, seg4};
    return PCO_SetCameraRamSegmentSize(handle, segs);
}

/*
 * Start the frame acquisition thread
 */
void PcoApi::doStartFrameCapture(bool useGetImage)
{
    this->useGetImage = useGetImage;
    ::SetEvent(this->startEvent);
}

/*
 * Stop the frame acquisition thread
 */
void PcoApi::doStopFrameCapture()
{
    ::SetEvent(this->stopEvent);
}

// C entry point for iocinit
extern "C" int pcoApiConfig(const char* portName)
{
    Pco* pco = Pco::getPco(portName);
    if(pco != NULL)
    {
        new PcoApi(pco, &pco->apiTrace);
    }
    else
    {
        printf("pcoApiConfig: Pco \"%s\" not found\n", portName);
    }
    return asynSuccess;
}
static const iocshArg pcoApiConfigArg0 = {"Port Name", iocshArgString};
static const iocshArg* const pcoApiConfigArgs[] =
    {&pcoApiConfigArg0};
static const iocshFuncDef configPcoApi =
    {"pcoApiConfig", 1, pcoApiConfigArgs};
static void configPcoApiCallFunc(const iocshArgBuf *args)
{
    pcoApiConfig(args[0].sval);
}

/** Register the functions */
static void pcoApiRegister(void)
{
    iocshRegister(&configPcoApi, configPcoApiCallFunc);
}

extern "C" { epicsExportRegistrar(pcoApiRegister); }


