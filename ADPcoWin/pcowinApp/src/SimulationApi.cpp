/*
 * SimulationApi.cpp
 *
 * Revamped PCO area detector driver.
 *
 * A simulation of the PCO API library and hardware
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */

#include "SimulationApi.h"
#include "TraceStream.h"
#include "Pco.h"
#include "epicsExport.h"
#include "iocsh.h"

/**
 * Constants
 */
const int SimulationApi::edgeSetupDataLength = 1;
const int SimulationApi::edgeSetupDataType = 1;

/**
 * Constructor
 */
SimulationApi::SimulationApi(Pco* pco, TraceStream* trace)
: DllApi(pco, trace)
, paramConnected(pco, "SimConnected", true, new AsynParam::Notify<SimulationApi>(this, &SimulationApi::onConnected))
, paramOpen(pco, "SimOpen", false)
, paramCameraType(pco, "SimCameraType", DllApi::cameraTypeEdge)
, paramMaxHorzRes(pco, "SimMaxHorzRes", 640)
, paramMaxVertRes(pco, "SimMaxVertRes", 480)
, paramDynResolution(pco, "SimDynResolution", 14)
, paramMaxBinHorz(pco, "SimMaxBinHorz", 4)
, paramMaxBinVert(pco, "SimMaxBinVert", 4)
, paramBinHorzStepping(pco, "SimBinHorzStepping", 1)
, paramBinVertStepping(pco, "SimBinVertStepping", 1)
, paramRoiHorSteps(pco, "SimRoiHorSteps", 1)
, paramRoiVertSteps(pco, "SimRoiVertSteps", 1)
, paramPixelRate(pco, "SimPixelRate", 4000000)
, paramConvFact(pco, "SimConvFact", 100)
, paramGeneralCaps(pco, "SimGeneralCaps", 0)
, paramRamSize(pco, "SimRamSize", 0)
, paramPageSize(pco, "SimPageSize", 0)
, paramBaudRate(pco, "SimBaudRate", 0)
, paramClockFrequency(pco, "SimClockFrequency", 0)
, paramCamlinkLines(pco, "SimCamlinkLines", 0)
, paramDataFormat(pco, "SimDataFormat", 0)
, paramTransmit(pco, "SimTransmit", 0)
, paramActualHorzRes(pco, "SimActualHorzRes", 640)
, paramActualVertRes(pco, "SimActualVertRes", 480)
, paramTimeYear(pco, "SimTimeYear", 0)
, paramTimeMonth(pco, "SimTimeMonth", 0)
, paramTimeDay(pco, "SimTimeDay", 0)
, paramTimeHour(pco, "SimTimeHour", 0)
, paramTimeMinute(pco, "SimTimeMinute", 0)
, paramTimeSecond(pco, "SimTimeSecond", 0)
, paramTempCcd(pco, "SimTempCcd", 21)
, paramTempCamera(pco, "SimTempCamera", 22)
, paramTempPsu(pco, "SimTempPsu", 23)
, paramBitAlignment(pco, "SimBitAlignment", 0)
, paramEdgeGlobalShutter(pco, "SimEdgeGlobalShutter", 0)
, paramActualHorzBin(pco, "SimActualHorzBin", 1)
, paramActualVertBin(pco, "SimActualVertBin", 1)
, paramActualRoiX0(pco, "SimActualRoiX0", 0)
, paramActualRoiY0(pco, "SimActualRoiY0", 0)
, paramActualRoiX1(pco, "SimActualRoiX1", 640)
, paramActualRoiY1(pco, "SimActualRoiY1", 480)
, paramTriggerMode(pco, "SimTriggerMode", DllApi::triggerAuto)
, paramStorageMode(pco, "SimStorageMode", DllApi::storageModeRecorder)
, paramTimestampMode(pco, "SimTimestampMode", DllApi::timestampModeOff)
, paramAcquireMode(pco, "SimAcquireMode", DllApi::acquireModeAuto)
, paramDelayTime(pco, "SimDelayTime", 100)
, paramDelayTimebase(pco, "SimDelayTimebase", DllApi::timebaseMilliseconds)
, paramExposureTime(pco, "SimExposureTime", 100)
, paramExposureTimebase(pco, "SimExposureTimebase", DllApi::timebaseMilliseconds)
, paramActualConvFact(pco, "SimActualConvFact", 100)
, paramAdcOperation(pco, "SimAdcOperation", DllApi::adcModeSingle)
, paramRecordingState(pco, "SimRecordingState", DllApi::recorderStateOff)
, paramRecorderSubmode(pco, "SimRecorderSubmode", 0)
, paramCamlinkHorzRes(pco, "SimCamlinkHorzRes", 640)
, paramCamlinkVertRes(pco, "SimCamlinkVertRes", 480)
, paramArmed(pco, "SimArmed", false)
, paramClearStateRecord(pco, "SimClearStateRecord", 0)
, paramExternalTrigger(pco, "SimExternalTrigger", 0, new AsynParam::Notify<SimulationApi>(this, &SimulationApi::onExternalTrigger))
, paramStateRecord(pco, "SimStateRecord", "")
, bufferQueue(DllApi::maxNumBuffers, sizeof(int))
, stateMachine(NULL)
, frameNumber(0)
{
    // Initialise the buffers
    for(int i=0; i<DllApi::maxNumBuffers; i++)
    {
        this->buffers[i].status = 0;
        this->buffers[i].buffer = NULL;
    }
    // Create the state machine
    this->stateMachine = new StateMachine("SimulationApi", this->pco,
            &paramStateRecord, trace);
    // Events
    requestConnectionUp = stateMachine->event("ConnectionUp");
    requestConnectionDown = stateMachine->event("ConnectionDown");
    requestOpen = stateMachine->event("Open");
    requestClose = stateMachine->event("Close");
    requestStartRecording = stateMachine->event("StartRecording");
    requestStopRecording = stateMachine->event("StopRecording");
    requestTrigger = stateMachine->event("Trigger");
    requestArm = stateMachine->event("Arm");
    requestCancelImages = stateMachine->event("CancelImages");
    // States
    stateConnected = stateMachine->state("Connected");
    stateOpen = stateMachine->state("Open");
    stateDisconnected = stateMachine->state("Disconnected");
    stateArmed = stateMachine->state("Armed");
    stateRecording = stateMachine->state("Recording");
    // Transitions
    stateMachine->transition(stateConnected, requestConnectionDown, NULL, stateDisconnected);
    stateMachine->transition(stateConnected, requestOpen, NULL, stateOpen);
    stateMachine->transition(stateOpen, requestConnectionDown, NULL, stateDisconnected);
    stateMachine->transition(stateOpen, requestClose, NULL, stateConnected);
    stateMachine->transition(stateOpen, requestArm, NULL, stateArmed);
    stateMachine->transition(stateDisconnected, requestConnectionUp, NULL, stateConnected);
    stateMachine->transition(stateArmed, requestConnectionDown, NULL, stateDisconnected);
    stateMachine->transition(stateArmed, requestClose, NULL, stateConnected);
    stateMachine->transition(stateArmed, requestStartRecording, new StateMachine::Act<SimulationApi>(this, &SimulationApi::smStartRecording), stateRecording);
    stateMachine->transition(stateArmed, requestCancelImages, NULL, stateOpen);
    stateMachine->transition(stateRecording, requestConnectionDown, new StateMachine::Act<SimulationApi>(this, &SimulationApi::smStopTriggerTimer), stateDisconnected);
    stateMachine->transition(stateRecording, requestClose, new StateMachine::Act<SimulationApi>(this, &SimulationApi::smStopTriggerTimer), stateConnected);
    stateMachine->transition(stateRecording, requestStopRecording, new StateMachine::Act<SimulationApi>(this, &SimulationApi::smStopTriggerTimer), stateArmed);
    stateMachine->transition(stateRecording, requestTrigger, new StateMachine::Act<SimulationApi>(this, &SimulationApi::smCreateFrame), stateRecording);
    // Starting state
    stateMachine->initialState(stateConnected);
}

/**
 * Destructor
 */
SimulationApi::~SimulationApi()
{
    delete this->stateMachine;
}

/**
 * Post a request
 */
void SimulationApi::post(const StateMachine::Event* req)
{
    this->stateMachine->post(req);
}


/**
 * Start recording
 */
StateMachine::StateSelector SimulationApi::smStartRecording()
{
    startTriggerTimer();
    frameNumber = 0;
    return StateMachine::firstState;
}

/**
 * Stop the trigger timer
 */
StateMachine::StateSelector SimulationApi::smStopTriggerTimer()
{
    stateMachine->stopTimer();
    return StateMachine::firstState;
}

/**
 * Create a frame
 */
StateMachine::StateSelector SimulationApi::smCreateFrame()
{
    generateFrame();
    startTriggerTimer();
    return StateMachine::firstState;
}

/**
 * Generate a simulated frame
 */
void SimulationApi::generateFrame()
{
    // Advance the frame number
    this->frameNumber++;
    if(this->frameNumber >= 100000000)
    {
        this->frameNumber = 0;
    }
    // Is there a buffer available in the queue
    if(this->bufferQueue.pending() > 0)
    {
        int bufferNumber;
        this->bufferQueue.tryReceive(&bufferNumber, sizeof(int));
        // Fill the frame with a pattern
        for(int x=0; x<paramActualHorzRes; x++)
        {
            for(int y=0; y<paramActualVertRes; y++)
            {
                bool dark = true;
                if(((x / 16) & 1) != 0)
                {
                    dark = !dark;
                }
                if(((y / 16) & 1) != 0)
                {
                    dark = !dark;
                }
                this->buffers[bufferNumber].buffer[y*paramActualHorzRes+x] = (dark ? 15 : 255);
            }
        }
        // Plant the BCD time stamp if enabled
        if(paramTimestampMode == DllApi::timestampModeBinary ||
                paramTimestampMode == DllApi::timestampModeBinaryAndAscii)
        {
            // The frame number
            int shiftLowBcd = 0;
            if(paramBitAlignment == DllApi::bitAlignmentMsb)
            {
                shiftLowBcd = Pco::bitsPerShortWord - paramDynResolution;
            }
            int shiftHighBcd = shiftLowBcd + Pco::bitsPerNybble;
            unsigned long n = this->frameNumber;
            unsigned long divisor = Pco::bcdDigitValue * Pco::bcdDigitValue *
                    Pco::bcdDigitValue * Pco::bcdDigitValue * Pco::bcdDigitValue *
                    Pco::bcdDigitValue * Pco::bcdDigitValue;
            for(int i=0; i<Pco::bcdPixelLength; i++)
            {
                unsigned long n0 = n / divisor;
                n -= n0 * divisor;
                divisor /= Pco::bcdDigitValue;
                unsigned long n1 = n / divisor;
                n -= n1 * divisor;
                divisor /= Pco::bcdDigitValue;
                unsigned short pixel = (unsigned short)((n1 << shiftLowBcd) | (n0 << shiftHighBcd));
                this->buffers[bufferNumber].buffer[i] = pixel;
            }
            // TODO: The time
        }
        // Give the buffer back to the driver
        this->buffers[bufferNumber].status |= DllApi::statusDllEventSet;
        this->pco->frameReceived(bufferNumber);
    }
}

/**
 * Start the trigger timer if the trigger mode is automatic
 */
void SimulationApi::startTriggerTimer()
{
    if(paramTriggerMode == DllApi::triggerAuto)
    {
        // In auto trigger mode, start the trigger timer
        double period = (double)paramDelayTime / DllApi::timebaseScaleFactor[paramDelayTimebase] +
                (double)paramExposureTime / DllApi::timebaseScaleFactor[paramExposureTimebase];
        this->stateMachine->startTimer(period, SimulationApi::requestTrigger);
    }
}

/**
 * Handles changes to the Connected parameter
 */
void SimulationApi::onConnected(TakeLock& takeLock)
{
    if(paramConnected)
    {
        this->post(SimulationApi::requestConnectionUp);
    }
    else
    {
        this->post(SimulationApi::requestConnectionDown);
    }
}

/**
 * Handle changes to the ExternalTrigger parameter
 */
void SimulationApi::onExternalTrigger(TakeLock& takeLock)
{
    if(paramTriggerMode == DllApi::triggerExternal ||
            paramTriggerMode == triggerExternalExposure)
    {
        this->post(SimulationApi::requestTrigger);
    }
}

/**
 * Connect to the camera
 * Camera number is currently ignored.
 */
int SimulationApi::doOpenCamera(Handle* handle, unsigned short camNum)
{
    int result = DllApi::errorAny;
    if(paramConnected && !paramOpen)
    {
        paramOpen = true;
        this->post(SimulationApi::requestOpen);
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Disconnect from the camera
 */
int SimulationApi::doCloseCamera(Handle handle)
{
    paramOpen = false;
    this->post(SimulationApi::requestClose);
    return DllApi::errorNone;
}

/**
 * Reboot the camera
 */
int SimulationApi::doRebootCamera(Handle handle)
{
    return DllApi::errorNone;
}

/**
 * Get general information from the camera
 */
int SimulationApi::doGetGeneral(Handle handle)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get the camera type information
 */
int SimulationApi::doGetCameraType(Handle handle, CameraType* cameraType)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        cameraType->camType = paramCameraType;
        cameraType->serialNumber = 0x00000001;
        cameraType->hardwareVersion = 0x00000002;
        cameraType->firmwareVersion = 0x00000003;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get the firmware versions of all devices in the camera
 */
int SimulationApi::doGetFirmwareInfo(Handle handle, std::vector<PcoCameraDevice> &devices)
{

    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        // TODO: implement function for simulator
        result = DllApi::errorNone;
    }
    return result;

}

/**
 * Get sensor information
 */
int SimulationApi::doGetSensorStruct(Handle handle)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get timing information
 */
int SimulationApi::doGetTimingStruct(Handle handle)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get camera description
 */
int SimulationApi::doGetCameraDescription(Handle handle, Description* description)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        description->maxHorzRes = (unsigned short)paramMaxHorzRes;
        description->maxVertRes = (unsigned short)paramMaxVertRes;
        description->dynResolution = (unsigned short)paramDynResolution;
        description->maxBinHorz = (unsigned short)paramMaxBinHorz;
        description->maxBinVert = (unsigned short)paramMaxBinVert;
        description->binHorzStepping = (unsigned short)paramBinHorzStepping;
        description->binVertStepping = (unsigned short)paramBinVertStepping;
        description->roiHorSteps = (unsigned short)paramRoiHorSteps;
        description->roiVertSteps = (unsigned short)paramRoiVertSteps;
        description->pixelRate[0] = (unsigned long)paramPixelRate;
        description->pixelRate[1] = 32000000;
        description->pixelRate[2] = 0;
        description->pixelRate[3] = 0;
        description->convFact = (unsigned short)paramConvFact;
        description->generalCaps = (unsigned long)paramGeneralCaps;
        description->minCoolingSetpoint = 0;
        description->maxCoolingSetpoint = 0;
        description->defaultCoolingSetpoint = 0;
        description->minDelayNs = 0;
        description->maxDelayMs = 10000;
        description->minDelayStepNs = 1;
        description->minExposureNs = 1000;
        description->maxExposureMs= 10000;
        description->minExposureStepNs = 1;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get camera storage information
 */
int SimulationApi::doGetStorageStruct(Handle handle, Storage* storage)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        storage->ramSizePages = (unsigned long)paramRamSize;
        storage->pageSizePixels= (unsigned short)paramPageSize;
        for(int i=0; i<DllApi::storageNumSegments; i++)
        {
            storage->segmentSizePages[storageNumSegments] = 0;
        }
        storage->activeSegment = 0;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get camera recording information
 */
int SimulationApi::doGetRecordingStruct(Handle handle)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Reset the camera's settings
 */
int SimulationApi::doResetSettingsToDefault(Handle handle)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        // TODO: Actually reset the settings
        this->post(SimulationApi::requestStopRecording);
        paramRecordingState = DllApi::recorderStateOff;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get the camera's transfer parameters
 */
int SimulationApi::doGetTransferParameters(Handle handle, Transfer* transfer)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        transfer->baudRate = (unsigned long)paramBaudRate;
        transfer->clockFrequency = (unsigned long)paramClockFrequency;
        transfer->camlinkLines = (unsigned long)paramCamlinkLines;
        transfer->dataFormat = (unsigned long)paramDataFormat;
        transfer->transmit = (unsigned long)paramTransmit;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Set the camera's transfer parameters
 */
int SimulationApi::doSetTransferParameters(Handle handle, Transfer* transfer)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        paramBaudRate = (int)transfer->baudRate;
        paramClockFrequency = (int)transfer->clockFrequency;
        paramCamlinkLines = (int)transfer->camlinkLines;
        paramDataFormat = (int)transfer->dataFormat;
        paramTransmit = (int)transfer->transmit;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * The the camera's current and maximum resolutions
 */
int SimulationApi::doGetSizes(Handle handle, Sizes* sizes)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        sizes->xResActual = (unsigned short)paramActualHorzRes;
        sizes->yResActual = (unsigned short)paramActualVertRes;
        sizes->xResMaximum = (unsigned short)paramMaxHorzRes;
        sizes->yResMaximum = (unsigned short)paramMaxVertRes;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Set the camera's date and time
 */
int SimulationApi::doSetDateTime(Handle handle, struct tm* currentTime)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        paramTimeYear = (int)currentTime->tm_year;
        paramTimeMonth = (int)currentTime->tm_mon;
        paramTimeDay = (int)currentTime->tm_mday;
        paramTimeHour = (int)currentTime->tm_hour;
        paramTimeMinute = (int)currentTime->tm_min;
        paramTimeSecond = (int)currentTime->tm_sec;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get camera temperatures
 */
int SimulationApi::doGetTemperature(Handle handle, short* ccd,
        short* camera, short* psu)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        *ccd = (short)paramTempCcd;
        *camera = (short)paramTempCamera;
        *psu = (short)paramTempPsu;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Set the camera's cooling setpoint
 */
int SimulationApi::doSetCoolingSetpoint(Handle handle, short setPoint)
{
    return DllApi::errorNone;
}

/**
 * Get the camera's cooling setpoint.
 */
int SimulationApi::doGetCoolingSetpoint(Handle handle, short* setPoint)
{
    return DllApi::errorNone;
}

/**
 * Set the camera's operating pixel rate
 */
int SimulationApi::doSetPixelRate(Handle handle, unsigned long pixRate)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        paramPixelRate = (int)pixRate;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get the camera's operating pixel rate
 */
int SimulationApi::doGetPixelRate(Handle handle, unsigned long* pixRate)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        *pixRate = (unsigned long)paramPixelRate;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get the operating bit alignment
 */
int SimulationApi::doGetBitAlignment(Handle handle, unsigned short* bitAlignment)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        *bitAlignment = (unsigned short)paramBitAlignment;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Set the operating bit alignment
 */
int SimulationApi::doSetBitAlignment(Handle handle, unsigned short bitAlignment)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        paramBitAlignment = (int)bitAlignment;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Return an Edge's camera setup information
 */
int SimulationApi::doGetCameraSetup(Handle handle, unsigned short* setupType,
        unsigned long* setupData, unsigned short* setupDataLen)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        *setupType = (unsigned short)SimulationApi::edgeSetupDataType;
        *setupData = (unsigned long)paramEdgeGlobalShutter;
        *setupDataLen = (unsigned short)SimulationApi::edgeSetupDataLength;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Set an Edge's camera setup information
 */
int SimulationApi::doSetCameraSetup(Handle handle, unsigned short setupType,
        unsigned long* setupData, unsigned short setupDataLen)
{
    return DllApi::errorNone;
}

/**
 * Set the binning parameters.
 */
int SimulationApi::doSetBinning(Handle handle, unsigned short binHorz, unsigned short binVert)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        paramActualHorzBin = (int)binHorz;
        paramActualVertBin = (int)binVert;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get the binning parameters.
 */
int SimulationApi::doGetBinning(Handle handle, unsigned short* binHorz, unsigned short* binVert)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        *binHorz = (unsigned short)paramActualHorzBin;
        *binVert = (unsigned short)paramActualVertBin;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Set the region of interest parameters
 */
int SimulationApi::doSetRoi(Handle handle, unsigned short x0, unsigned short y0,
        unsigned short x1, unsigned short y1)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        paramActualRoiX0 = (int)x0;
        paramActualRoiY0 = (int)y0;
        paramActualRoiX1 = (int)x1;
        paramActualRoiY1 = (int)y1;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get the region of interest parameters
 */
int SimulationApi::doGetRoi(Handle handle, unsigned short* x0, unsigned short* y0,
        unsigned short* x1, unsigned short* y1)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        *x0 = (unsigned short)paramActualRoiX0;
        *y0 = (unsigned short)paramActualRoiY0;
        *x1 = (unsigned short)paramActualRoiX1;
        *y1 = (unsigned short)paramActualRoiY1;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Set the trigger mode
 */
int SimulationApi::doSetTriggerMode(Handle handle, unsigned short mode)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        paramTriggerMode = (int)mode;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get the trigger mode
 */
int SimulationApi::doGetTriggerMode(Handle handle, unsigned short* mode)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        *mode = (unsigned short)paramTriggerMode;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Set the storage mode
 */
int SimulationApi::doSetStorageMode(Handle handle, unsigned short mode)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        paramStorageMode = (int)mode;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get the storage mode
 */
int SimulationApi::doGetStorageMode(Handle handle, unsigned short* mode)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        *mode = (unsigned short)paramStorageMode;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Set the time stamp mode
 */
int SimulationApi::doSetTimestampMode(Handle handle, unsigned short mode)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        paramTimestampMode = (int)mode;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get the time stamp mode
 */
int SimulationApi::doGetTimestampMode(Handle handle, unsigned short* mode)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        *mode = (unsigned short)paramTimestampMode;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Set the acquire mode
 */
int SimulationApi::doSetAcquireMode(Handle handle, unsigned short mode)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        paramAcquireMode = (int)mode;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get the acquire mode
 */
int SimulationApi::doGetAcquireMode(Handle handle, unsigned short* mode)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        *mode = (unsigned short)paramAcquireMode;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Set the delay and exposure times
 */
int SimulationApi::doSetDelayExposureTime(Handle handle, unsigned long delay,
        unsigned long exposure, unsigned short timeBaseDelay,
        unsigned short timeBaseExposure)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        paramDelayTime = (int)delay;
        paramDelayTimebase = (int)timeBaseDelay;
        paramExposureTime = (int)exposure;
        paramExposureTimebase = (int)timeBaseExposure;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get the delay and exposure times
 */
int SimulationApi::doGetDelayExposureTime(Handle handle, unsigned long* delay,
        unsigned long* exposure, unsigned short* timeBaseDelay,
        unsigned short* timeBaseExposure)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        *delay = (unsigned long)paramDelayTime;
        *timeBaseDelay = (unsigned short)paramDelayTimebase;
        *exposure = (unsigned long)paramExposureTime;
        *timeBaseExposure = (unsigned short)paramExposureTimebase;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Set the sensor gain
 */
int SimulationApi::doSetConversionFactor(Handle handle, unsigned short factor)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        paramActualConvFact = (int)factor;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get the ADC operating mode
 */
int SimulationApi::doGetAdcOperation(Handle handle, unsigned short* mode)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        *mode = (unsigned short)paramAdcOperation;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Set the ADC operating mode
 */
int SimulationApi::doSetAdcOperation(Handle handle, unsigned short mode)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        paramAdcOperation = (int)mode;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get the camera's recording state
 */
int SimulationApi::doGetRecordingState(Handle handle, unsigned short* state)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        *state = (unsigned short)paramRecordingState;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Set the camera's recording state
 */
int SimulationApi::doSetRecordingState(Handle handle, unsigned short state)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        if(state == DllApi::recorderStateOn)
        {
            this->post(SimulationApi::requestStartRecording);
        }
        else
        {
            this->post(SimulationApi::requestStopRecording);
        }
        paramRecordingState = (int)state;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get the recorder submode
 */
int SimulationApi::doGetRecorderSubmode(Handle handle, unsigned short* mode)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        *mode = (unsigned short)paramRecorderSubmode;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Set the recorder submode
 */
int SimulationApi::doSetRecorderSubmode(Handle handle, unsigned short mode)
{
    return DllApi::errorNone;
}

/**
 * Allocate a buffer
 */
int SimulationApi::doAllocateBuffer(Handle handle, short* bufferNumber, unsigned long size,
        unsigned short** buffer, Handle* eventHandle)
{
    int result = DllApi::errorAny;
    bool found = false;
    if(paramConnected && paramOpen)
    {
        // Identify a buffer number
        found = *bufferNumber != DllApi::bufferUnallocated;
        for(int i=0; i<DllApi::maxNumBuffers && !found; i++)
        {
            if((this->buffers[i].status & DllApi::statusDllBufferAllocated) == 0)
            {
                found = true;
                *bufferNumber = (short)i;
                this->buffers[i].status = DllApi::statusDllBufferAllocated;
            }
        }
        if(found)
        {
            // Allocate the memory
            if(*buffer == NULL)
            {
                this->buffers[*bufferNumber].buffer = new unsigned short[size/sizeof(unsigned short)];
                *buffer = this->buffers[*bufferNumber].buffer;
            }
            else
            {
                this->buffers[*bufferNumber].buffer = *buffer;
                this->buffers[*bufferNumber].status |= DllApi::statusDllExternalBuffer;
            }
            // Create the event (the simulation doesn't use this, so just indicate created)
            if(*eventHandle == NULL)
            {
                this->buffers[*bufferNumber].status |= DllApi::statusDllEventCreated;
            }
            // Return result
            result = DllApi::errorNone;
        }
    }
    return result;
}

/**
 * Cancel all image buffers
 */
int SimulationApi::doCancelImages(Handle handle)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        // Empty the buffer queue
        while(this->bufferQueue.pending() > 0)
        {
            int bufferNumber;
            this->bufferQueue.tryReceive(&bufferNumber, sizeof(int));
        }
        this->post(SimulationApi::requestCancelImages);
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Set the image parameters for the image buffer transfer inside the CamLink and GigE interface.
 * While using CamLink or GigE this function must be called, before the user tries to get images
 * from the camera and the sizes have changed. With all other interfaces this is a dummy call.
 */
int SimulationApi::doCamlinkSetImageParameters(Handle handle, unsigned short xRes, unsigned short yRes)
{
    paramCamlinkHorzRes = (int)xRes;
    paramCamlinkHorzRes = (int)yRes;
    return DllApi::errorNone;
}

/**
 * Arm the camera ready for taking images.
 */
int SimulationApi::doArm(Handle handle)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        paramArmed = (int)true;
        this->post(SimulationApi::requestArm);
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Add a buffer to the receive queue.
 */
int SimulationApi::doAddBufferEx(Handle handle, unsigned long firstImage, unsigned long lastImage,
        short bufferNumber, unsigned short xRes, unsigned short yRes,
        unsigned short bitRes)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        // Are the parameters correct?
        if((int)xRes == paramActualHorzRes && (int)yRes == paramActualVertRes &&
                firstImage==0 && lastImage==0)
        {
            // Put the buffer on the queue
            int v = bufferNumber;
            this->bufferQueue.trySend(&v, sizeof(int));
        }
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Get an image from memory.
 */
int SimulationApi::doGetImageEx(Handle handle, unsigned short segment, unsigned long firstImage,
        unsigned long lastImage, short bufferNumber, unsigned short xRes, 
        unsigned short yRes, unsigned short bitRes)
{
    return DllApi::errorAny;
}

/**
 * Get the status of a buffer
 */
int SimulationApi::doGetBufferStatus(Handle handle, short bufferNumber,
        unsigned long* statusDll, unsigned long* statusDrv)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        *statusDll = this->buffers[bufferNumber].status;
        *statusDrv = 0;
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Force a trigger.
 */
int SimulationApi::doForceTrigger(Handle handle, unsigned short* triggered)
{
    int result = DllApi::errorAny;
    if(paramConnected && paramOpen)
    {
        if(paramTriggerMode == DllApi::triggerSoftware ||
                paramTriggerMode == DllApi::triggerExternal)
        {
            this->post(SimulationApi::requestTrigger);
        }
        // TODO: Fill in the triggered return parameter
        result = DllApi::errorNone;
    }
    return result;
}

/**
 * Free a buffer.
 */
int SimulationApi::doFreeBuffer(Handle handle, short bufferNumber)
{
    if((this->buffers[bufferNumber].status & DllApi::statusDllExternalBuffer) == 0 &&
        this->buffers[bufferNumber].buffer != NULL)
    {
        delete[] this->buffers[bufferNumber].buffer;
        this->buffers[bufferNumber].buffer = NULL;
    }
    this->buffers[bufferNumber].status = 0;
    return DllApi::errorNone;
}

/**
 * Get the active RAM segment
 */
int SimulationApi::doGetActiveRamSegment(Handle handle, unsigned short* segment)
{
    *segment = 1;
    return DllApi::errorNone;
}

/**
 * Set the active RAM segment
 */
int SimulationApi::doSetActiveRamSegment(Handle handle, unsigned short segment)
{
    return DllApi::errorNone;
}

/**
 * Get the number of images in a segment
 */
int SimulationApi::doGetNumberOfImagesInSegment(Handle handle, unsigned short segment,
        unsigned long* validImageCount, unsigned long* maxImageCount)
{
    *validImageCount = 0;
    *maxImageCount = 0;
    return DllApi::errorNone;
}

/**
 * Set the active lookup table
 */
int SimulationApi::doSetActiveLookupTable(Handle handle, unsigned short identifier)
{
    return DllApi::errorNone;
}

/**
 * Set driver timeouts
 */
int SimulationApi::doSetTimeouts(Handle handle, unsigned int commandTimeout,
        unsigned int imageTimeout, unsigned int transferTimeout)
{
    return DllApi::errorNone;
}

/**
 * Get the camera RAM size
 */
int SimulationApi::doGetCameraRamSize(Handle handle, unsigned long* numPages, unsigned short* pageSize)
{
    *numPages = 0;
    *pageSize = 0;
    return DllApi::errorNone;
}

/**
 * Clear active RAM segment
 */
int SimulationApi::doClearRamSegment(Handle handle)
{
    return DllApi::errorNone;
}

/**
 * Get the camera health status
 */
int SimulationApi::doGetCameraHealthStatus(Handle handle, unsigned long* warnings, unsigned long* errors,
            unsigned long* status)
{
    *warnings = 0;
    *errors = 0;
    *status = 0;
    return DllApi::errorNone;
}

/**
 * Get the camera busy status
 */
int SimulationApi::doGetCameraBusyStatus(Handle handle, unsigned short* status)
{
    *status = 0;
    return DllApi::errorNone;
}

/**
 * Get the exposure trigger status
 */
int SimulationApi::doGetExpTrigSignalStatus(Handle handle, unsigned short* status)
{
    *status = 0;
    return DllApi::errorNone;
}

/**
 * Get the acquisition enable status
 */
int SimulationApi::doGetAcqEnblSignalStatus(Handle handle, unsigned short* status)
{
    *status = 0;
    return DllApi::errorNone;
}

/**
 * Set the sensor format
 */
int SimulationApi::doSetSensorFormat(Handle handle, unsigned short format)
{
    return DllApi::errorNone;
}

/**
 * Set double image mode
 */
int SimulationApi::doSetDoubleImageMode(Handle handle, unsigned short mode)
{
    return DllApi::errorNone;
}

/**
 * Set offset mode
 */
int SimulationApi::doSetOffsetMode(Handle handle, unsigned short mode)
{
    return DllApi::errorNone;
}

/**
 * Set noise filter mode
 */
int SimulationApi::doSetNoiseFilterMode(Handle handle, unsigned short mode)
{
    return DllApi::errorNone;
}

/**
 * Set the camera RAM segment size
 */
int SimulationApi::doSetCameraRamSegmentSize(Handle handle, unsigned long seg1,
    unsigned long seg2, unsigned long seg3, unsigned long seg4)
{
    return DllApi::errorNone;
}

/*
 * Start the frame acquisition thread
 */
void SimulationApi::doStartFrameCapture(bool useGetImage)
{
}

/*
 * Stop the frame acquisition thread
 */
void SimulationApi::doStopFrameCapture()
{
}

// C entry point for iocinit
extern "C" int simulationApiConfig(const char* portName)
{
    Pco* pco = Pco::getPco(portName);
    if(pco != NULL)
    {
        new SimulationApi(pco, &pco->apiTrace);
    }
    else
    {
        printf("simulationApiConfig: Pco \"%s\" not found\n", portName);
    }
    return asynSuccess;
}
static const iocshArg simulationApiConfigArg0 = {"Port Name", iocshArgString};
static const iocshArg* const simulationApiConfigArgs[] =
    {&simulationApiConfigArg0};
static const iocshFuncDef configSimulationApi =
    {"simulationApiConfig", 1, simulationApiConfigArgs};
static void configSimulationApiCallFunc(const iocshArgBuf *args)
{
    simulationApiConfig(args[0].sval);
}

/** Register the functions */
static void simulationApiRegister(void)
{
    iocshRegister(&configSimulationApi, configSimulationApiCallFunc);
}

extern "C" { epicsExportRegistrar(simulationApiRegister); }


