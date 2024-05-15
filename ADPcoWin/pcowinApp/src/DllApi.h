/*
 * Api.h
 *
 * Revamped PCO area detector driver.
 *
 * Virtual base class for the PCO API library
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */

#ifndef DLLAPI_H_
#define DLLAPI_H_

#include <string>
#include "asynPortDriver.h"
#include "PcoException.h"
#include "PcoCameraDevice.h"
class Pco;
class TraceStream;

class DllApi
{
// Construction
public:
    DllApi(Pco* pco, TraceStream* trace);
    virtual ~DllApi();

// Constants
public:
    enum {errorNone=0, errorAny=-1};
    enum {cameraTypeDimaxStd=0x1000, cameraTypeDimaxTv=0x1010, cameraTypeDimaxAutomotive=0x1020,
        cameraType1200Hs=0x0100, cameraType1300=0x0200, cameraType1600=0x0220,
        cameraType2000=0x0240, cameraType4000=0x0260, cameraTypeEdge=0x1300,
        cameraTypeEdgeGl=0x1310, cameraTypeEdgeCLHS=0x1340};
    enum {triggerAuto=0x0000, triggerSoftware=0x0001, triggerExternal=0x0002,
        triggerExternalExposure=0x0003, triggerSourceHdsdi=0x0102,
        triggerExternalSynchronised=0x0004, triggerExternalOnly=0x0005};
    enum {storageModeRecorder=0, storageModeFifoBuffer=1};
    enum {recorderSubmodeSequence=0, recorderSubmodeRingBuffer=1};
    enum {timestampModeOff=0, timestampModeBinary=1,
        timestampModeBinaryAndAscii=2, timestampModeAscii=3};
    enum {acquireModeAuto=0, acquireModeExternal=1,
        acquireModeExternalFrameTrigger=2, acquireModeLiveView=3,
        acquireModeImageSequence=4};
    enum {timebaseNanoseconds=0, timebaseMicroseconds=1, timebaseMilliseconds=2,
        numTimebases=3};
    enum {adcModeSingle=1, adcModeDual=2};
    enum {generalCapsTimestampAsciiOnly=0x00000010, generalCapsNoTimestamp=0x00000100};
    enum {edgeSetupRollingShutter=0x00000001, edgeSetupGlobalShutter=0x00000002, 
        edgeSetupGlobalReset=0x00000004};
    enum {camlinkDataFormatMask=0x0f, camlinkDataFormat1x16=0x01, camlinkDataFormat2x12=0x02,
        camlinkDataFormat3x8=0x03, camlinkDataFormat4x16=0x04, camlinkDataFormat5x16=0x05,
        camlinkDataFormat5x12=0x07, camlinkDataFormat10x8=0x08, camlinkDataFormat5x12L=0x09,
        camlinkDataFormat5x12R=0x0a};
    enum {camlinkLutNone=0, camLinkLutSqrt=0x1612};
    enum {sccmosFormatMask=0xff00, sccmosFormatTopBottom=0x0000,
        sccmosFormatTopCenterBottomCenter=0x0100, sccmosFormatCenterTopCenterBottom=0x0200,
        sccmosFormatCenterTopBottomCenter=0x0300, sccmosFormatTopCenterCenterBottom=0x0400};
    enum {recorderStateOff=0, recorderStateOn=1};
    enum {statusDllBufferAllocated=0x80000000, statusDllEventCreated=0x40000000,
        statusDllExternalBuffer=0x20000000, statusDllEventSet=0x00008000};
    static const double ccdTemperatureScaleFactor;
    enum {descriptionNumPixelRates=4};
    enum {cameraSetupDataSize=10, cameraSetupRollingShutter=1, cameraSetupGlobalShutter=2};
    static const double timebaseScaleFactor[numTimebases];
    enum {maxNumBuffers=8};
    enum {bufferUnallocated=-1};
    enum {binSteppingLinearBinary=0, binSteppingLinear=1};
    enum {bitAlignmentMsb=0, bitAlignmentLsb=1};
    enum {transferTransmitEnable=1, transferTransmitLongGap=2};
    enum {storageNumSegments=4};
    enum {fireWire=1, cameraLink=2, USBGen2=3, GigE=4, Serial=5, USBGen3=6, cameraLinkHS=7};

// Types
public:
    typedef void* Handle;
    struct Description
    {
        unsigned short maxHorzRes;      // Maximum horz. resolution in std.mode
        unsigned short maxVertRes;      // Maximum vert. resolution in std.mode // 10
        unsigned short dynResolution;   // Dynamic resolution of ADC in bit
        unsigned short maxBinHorz;      // Maximum horz. binning
        unsigned short maxBinVert;      // Maximum vert. binning
        unsigned short binHorzStepping; // Horz. bin. stepping (0:bin, 1:lin)    // 20
        unsigned short binVertStepping; // Vert. bin. stepping (0:bin, 1:lin)
        unsigned short roiHorSteps;     // Minimum granularity of ROI in pixels
        unsigned short roiVertSteps;    // Minimum granularity of ROI in pixels
        unsigned long pixelRate[descriptionNumPixelRates];    // Possible pixel rates in Hz  // 48
        unsigned short convFact;        // Possible conversion factor in e/cnt   // 136
        unsigned long generalCaps;      // General capabilities
        unsigned long minDelayNs;       // Minimum delay time in ns
        unsigned long maxDelayMs;       // Maximum delay time in ms
        unsigned long minDelayStepNs;   // Minimum stepping of delay time in ns  // 192
        unsigned long minExposureNs;    // Minimum exposure time in ns
        unsigned long maxExposureMs;    // Maximum exposure time in ms           // 200
        unsigned long minExposureStepNs;// Minimum stepping of exposure time in ns
        short minCoolingSetpoint;       // Minimum cooling set point
        short maxCoolingSetpoint;       // Maximum cooling set point
        short defaultCoolingSetpoint;   // Default cooling set point
    };
    struct Transfer
    {
        unsigned long baudRate;         // serial baud rate: 9600, 19200, 38400, 56400, 115200
        unsigned long clockFrequency;   // Pixel clock in Hz: 40000000,66000000,80000000
        unsigned long camlinkLines;     // Usage of CameraLink CC1-CC4 lines, use value returned by Get
        unsigned long dataFormat;       // camlinkDataFormat | sccmosFormat (enums above)
        unsigned long transmit;         // single or continuous transmitting images, 0-single, 1-continuous
    };
    struct Sizes
    {
        unsigned short xResActual;
        unsigned short yResActual;
        unsigned short xResMaximum;
        unsigned short yResMaximum;
    };
    struct CameraType
    {
        unsigned short camType;         // camera type from cameraType enum
        unsigned long serialNumber;     // serial number of the device
        unsigned long hardwareVersion;  // hardware version
        unsigned long firmwareVersion;  // firmware version
        unsigned short interfaceType;   // interface
    };
    struct Storage
    {
        unsigned long ramSizePages;     // Size of camera ram in pages
        unsigned short pageSizePixels;  // Size of a page in pixels
        unsigned long segmentSizePages[storageNumSegments];  // Segment sizes in pages
        unsigned short activeSegment;   // The active segment number
    };

// API for derived classes to implement
protected:
    virtual int doOpenCamera(Handle* handle, unsigned short camNum) = 0;
    virtual int doCloseCamera(Handle handle) = 0;
    virtual int doRebootCamera(Handle handle) = 0;
    virtual int doGetGeneral(Handle handle) = 0;
    virtual int doGetCameraType(Handle handle, CameraType* cameraType) = 0;
    virtual int doGetFirmwareInfo(Handle handle, std::vector<PcoCameraDevice> &devices) = 0;
    virtual int doGetSensorStruct(Handle handle) = 0;
    virtual int doGetTimingStruct(Handle handle) = 0;
    virtual int doGetCameraDescription(Handle handle, Description* description) = 0;
    virtual int doGetStorageStruct(Handle handle, Storage* storage) = 0;
    virtual int doGetRecordingStruct(Handle handle) = 0;
    virtual int doResetSettingsToDefault(Handle handle) = 0;
    virtual int doGetTransferParameters(Handle handle, Transfer* transfer) = 0;
    virtual int doSetTransferParameters(Handle handle, Transfer* transfer) = 0;
    virtual int doGetSizes(Handle handle, Sizes* sizes) = 0;
    virtual int doSetDateTime(Handle handle, struct tm* currentTime) = 0;
    virtual int doGetTemperature(Handle handle, short* ccd, short* camera, short* psu) = 0;
    virtual int doSetCoolingSetpoint(Handle handle, short setPoint) = 0;
    virtual int doGetCoolingSetpoint(Handle handle, short* setPoint) = 0;
    virtual int doSetPixelRate(Handle handle, unsigned long pixRate) = 0;
    virtual int doGetPixelRate(Handle handle, unsigned long* pixRate) = 0;
    virtual int doGetBitAlignment(Handle handle, unsigned short* bitAlignment) = 0;
    virtual int doSetBitAlignment(Handle handle, unsigned short bitAlignment) = 0;
    virtual int doGetCameraSetup(Handle handle, unsigned short* setupType,
            unsigned long* setupData, unsigned short* setupDataLen) = 0;
    virtual int doSetCameraSetup(Handle handle, unsigned short setupType,
            unsigned long* setupData, unsigned short setupDataLen) = 0;
    virtual int doSetBinning(Handle handle, unsigned short binHorz, unsigned short binVert) = 0;
    virtual int doGetBinning(Handle handle, unsigned short* binHorz, unsigned short* binVert) = 0;
    virtual int doSetRoi(Handle handle, unsigned short x0, unsigned short y0,
            unsigned short x1, unsigned short y1) = 0;
    virtual int doGetRoi(Handle handle, unsigned short* x0, unsigned short* y0,
            unsigned short* x1, unsigned short* y1) = 0;
    virtual int doSetTriggerMode(Handle handle, unsigned short mode) = 0;
    virtual int doGetTriggerMode(Handle handle, unsigned short* mode) = 0;
    virtual int doSetStorageMode(Handle handle, unsigned short mode) = 0;
    virtual int doGetStorageMode(Handle handle, unsigned short* mode) = 0;
    virtual int doSetTimestampMode(Handle handle, unsigned short mode) = 0;
    virtual int doGetTimestampMode(Handle handle, unsigned short* mode) = 0;
    virtual int doSetAcquireMode(Handle handle, unsigned short mode) = 0;
    virtual int doGetAcquireMode(Handle handle, unsigned short* mode) = 0;
    virtual int doSetDelayExposureTime(Handle handle, unsigned long delay,
            unsigned long exposure, unsigned short timeBaseDelay,
            unsigned short timeBaseExposure) = 0;
    virtual int doGetDelayExposureTime(Handle handle, unsigned long* delay,
            unsigned long* exposure, unsigned short* timeBaseDelay,
            unsigned short* timeBaseExposure) = 0;
    virtual int doSetConversionFactor(Handle handle, unsigned short factor) = 0;
    virtual int doSetAdcOperation(Handle handle, unsigned short mode) = 0;
    virtual int doGetAdcOperation(Handle handle, unsigned short* mode) = 0;
    virtual int doGetRecordingState(Handle handle, unsigned short* state) = 0;
    virtual int doSetRecordingState(Handle handle, unsigned short state) = 0;
    virtual int doGetRecorderSubmode(Handle handle, unsigned short* mode) = 0;
    virtual int doSetRecorderSubmode(Handle handle, unsigned short mode) = 0;
    virtual int doAllocateBuffer(Handle handle, short* bufferNumber, unsigned long size,
            unsigned short** buffer, Handle* event) = 0;
    virtual int doCancelImages(Handle handle) = 0;
    virtual int doCamlinkSetImageParameters(Handle handle, unsigned short xRes, unsigned short yRes) = 0;
    virtual int doArm(Handle handle) = 0;
    virtual int doAddBufferEx(Handle handle, unsigned long firstImage, unsigned long lastImage, 
        short bufferNumber, unsigned short xRes, unsigned short yRes, unsigned short bitRes) = 0;
    virtual int doGetImageEx(Handle handle, unsigned short segment, unsigned long firstImage,
        unsigned long lastImage, short bufferNumber, unsigned short xRes, 
        unsigned short yRes, unsigned short bitRes) = 0;
    virtual int doGetBufferStatus(Handle handle, short bufferNumber, unsigned long* statusDll, 
        unsigned long* statusDrv) = 0;
    virtual int doForceTrigger(Handle handle, unsigned short* triggered) = 0;
    virtual int doFreeBuffer(Handle handle, short bufferNumber) = 0;
    virtual int doGetActiveRamSegment(Handle handle, unsigned short* segment) = 0;
    virtual int doSetActiveRamSegment(Handle handle, unsigned short segment) = 0;
    virtual int doGetNumberOfImagesInSegment(Handle handle, unsigned short segment,
            unsigned long* validImageCount, unsigned long* maxImageCount) = 0;
    virtual int doSetActiveLookupTable(Handle handle, unsigned short identifier) = 0;
    virtual int doSetTimeouts(Handle handle, unsigned int commandTimeout,
            unsigned int imageTimeout, unsigned int transferTimeout) = 0;
    virtual int doClearRamSegment(Handle handle) = 0;
    virtual int doGetCameraRamSize(Handle handle, unsigned long* numPages, unsigned short* pageSize) = 0;
    virtual int doGetCameraHealthStatus(Handle handle, unsigned long* warnings, unsigned long* errors,
            unsigned long* status) = 0;
    virtual int doGetCameraBusyStatus(Handle handle, unsigned short* status) = 0;
    virtual int doGetExpTrigSignalStatus(Handle handle, unsigned short* status) = 0;
    virtual int doGetAcqEnblSignalStatus(Handle handle, unsigned short* status) = 0;
    virtual int doSetSensorFormat(Handle handle, unsigned short format) = 0;
    virtual int doSetDoubleImageMode(Handle handle, unsigned short mode) = 0;
    virtual int doSetOffsetMode(Handle handle, unsigned short mode) = 0;
    virtual int doSetNoiseFilterMode(Handle handle, unsigned short mode) = 0;
    virtual int doSetCameraRamSegmentSize(Handle handle, unsigned long seg1,
        unsigned long seg2, unsigned long seg3, unsigned long seg4) = 0;
    virtual void doStartFrameCapture(bool useGetImage) = 0;
    virtual void doStopFrameCapture() = 0;

// API for Pco class
public:
    void openCamera(Handle* handle, unsigned short camNum) throw(PcoException);
    void closeCamera(Handle handle) throw(PcoException);
    void rebootCamera(Handle handle) throw(PcoException);
    void getGeneral(Handle handle) throw(PcoException);
    void getCameraType(Handle handle, CameraType* cameraType) throw(PcoException);
    void getFirmwareInfo(Handle handle, std::vector<PcoCameraDevice> &devices) throw(PcoException);
    void getSensorStruct(Handle handle) throw(PcoException);
    void getTimingStruct(Handle handle) throw(PcoException);
    void getCameraDescription(Handle handle, Description* description) throw(PcoException);
    void getStorageStruct(Handle handle, Storage* storage) throw(PcoException);
    void getRecordingStruct(Handle handle) throw(PcoException);
    void resetSettingsToDefault(Handle handle) throw(PcoException);
    void getTransferParameters(Handle handle, Transfer* transfer) throw(PcoException);
    void setTransferParameters(Handle handle, Transfer* transfer) throw(PcoException);
    void getSizes(Handle handle, Sizes* sizes) throw(PcoException);
    void setDateTime(Handle handle, struct tm* currentTime) throw(PcoException);
    void getTemperature(Handle handle, short* ccd, short* camera, short* psu) throw(PcoException);
    void setCoolingSetpoint(Handle handle, short setPoint) throw(PcoException);
    void getCoolingSetpoint(Handle handle, short* setPoint) throw(PcoException);
    void setPixelRate(Handle handle, unsigned long pixRate) throw(PcoException);
    void getPixelRate(Handle handle, unsigned long* pixRate) throw(PcoException);
    void getBitAlignment(Handle handle, unsigned short* bitAlignment) throw(PcoException);
    void setBitAlignment(Handle handle, unsigned short bitAlignment) throw(PcoException);
    void getCameraSetup(Handle handle, unsigned short* setupType,
            unsigned long* setupData, unsigned short* setupDataLen) throw(PcoException);
    void setCameraSetup(Handle handle, unsigned short setupType,
            unsigned long* setupData, unsigned short setupDataLen) throw(PcoException);
    void setBinning(Handle handle, unsigned short binHorz, unsigned short binVert) throw(PcoException);
    void getBinning(Handle handle, unsigned short* binHorz, unsigned short* binVert) throw(PcoException);
    void setRoi(Handle handle, unsigned short x0, unsigned short y0,
            unsigned short x1, unsigned short y1) throw(PcoException);
    void getRoi(Handle handle, unsigned short* x0, unsigned short* y0,
            unsigned short* x1, unsigned short* y1) throw(PcoException);
    void setTriggerMode(Handle handle, unsigned short mode) throw(PcoException);
    void getTriggerMode(Handle handle, unsigned short* mode) throw(PcoException);
    void setStorageMode(Handle handle, unsigned short mode) throw(PcoException);
    void getStorageMode(Handle handle, unsigned short* mode) throw(PcoException);
    void setTimestampMode(Handle handle, unsigned short mode) throw(PcoException);
    void getTimestampMode(Handle handle, unsigned short* mode) throw(PcoException);
    void setAcquireMode(Handle handle, unsigned short mode) throw(PcoException);
    void getAcquireMode(Handle handle, unsigned short* mode) throw(PcoException);
    void setDelayExposureTime(Handle handle, unsigned long delay,
            unsigned long exposure, unsigned short timeBaseDelay,
            unsigned short timeBaseExposure) throw(PcoException);
    void getDelayExposureTime(Handle handle, unsigned long* delay,
            unsigned long* exposure, unsigned short* timeBaseDelay,
            unsigned short* timeBaseExposure) throw(PcoException);
    void setConversionFactor(Handle handle, unsigned short factor) throw(PcoException);
    void setAdcOperation(Handle handle, unsigned short mode) throw(PcoException);
    void getAdcOperation(Handle handle, unsigned short* mode) throw(PcoException);
    void getRecordingState(Handle handle, unsigned short* state) throw(PcoException);
    void setRecordingState(Handle handle, unsigned short state) throw(PcoException);
    void getRecorderSubmode(Handle handle, unsigned short* mode) throw(PcoException);
    void setRecorderSubmode(Handle handle, unsigned short mode) throw(PcoException);
    void allocateBuffer(Handle handle, short* bufferNumber, unsigned long size,
            unsigned short** buffer, Handle* event) throw(PcoException);
    void cancelImages(Handle handle) throw(PcoException);
    void camlinkSetImageParameters(Handle handle, unsigned short xRes, unsigned short yRes) throw(PcoException);
    void arm(Handle handle) throw(PcoException);
    void addBufferEx(Handle handle, unsigned long firstImage, unsigned long lastImage, 
        short bufferNumber, unsigned short xRes, unsigned short yRes, 
        unsigned short bitRes) throw(PcoException);
    void getImageEx(Handle handle, unsigned short segment, unsigned long firstImage,
        unsigned long lastImage, short bufferNumber, unsigned short xRes, 
        unsigned short yRes, unsigned short bitRes) throw(PcoException);
    void getBufferStatus(Handle handle, short bufferNumber, unsigned long* statusDll, 
        unsigned long* statusDrv) throw(PcoException);
    void forceTrigger(Handle handle, unsigned short* triggered) throw(PcoException);
    void freeBuffer(Handle handle, short bufferNumber) throw(PcoException);
    void getActiveRamSegment(Handle handle, unsigned short* segment) throw(PcoException);
    void setActiveRamSegment(Handle handle, unsigned short segment) throw(PcoException);
    void getNumberOfImagesInSegment(Handle handle, unsigned short segment,
            unsigned long* validImageCount, unsigned long* maxImageCount) throw(PcoException);
    void setActiveLookupTable(Handle handle, unsigned short identifier) throw(PcoException);
    void setTimeouts(Handle handle, unsigned int commandTimeout,
            unsigned int imageTimeout, unsigned int transferTimeout);
    void clearRamSegment(Handle handle);
    void getCameraRamSize(Handle handle, unsigned long* numPages, unsigned short* pageSize);
    void getCameraHealthStatus(Handle handle, unsigned long* warnings, unsigned long* errors,
            unsigned long* status);
    void getCameraBusyStatus(Handle handle, unsigned short* status);
    void getExpTrigSignalStatus(Handle handle, unsigned short* status);
    void getAcqEnblSignalStatus(Handle handle, unsigned short* status);
    void setSensorFormat(Handle handle, unsigned short format) throw(PcoException);
    void setDoubleImageMode(Handle handle, unsigned short mode) throw(PcoException);
    void setOffsetMode(Handle handle, unsigned short mode) throw(PcoException);
    void setNoiseFilterMode(Handle handle, unsigned short mode) throw(PcoException);
    void setCameraRamSegmentSize(Handle handle, unsigned long seg1, unsigned long seg2,
        unsigned long seg3, unsigned long seg4) throw(PcoException);
    void startFrameCapture(bool useGetImage);
    void stopFrameCapture();
    bool isStopped();

// Members
protected:
    Pco* pco;
    TraceStream* trace;
    bool stopped;
};

#endif /* DLLAPI_H_ */
