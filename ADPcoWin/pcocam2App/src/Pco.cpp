/* Pco.h
 *
 * Revamped PCO area detector driver.
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */
#include "Pco.h"
#include "DllApi.h"
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <algorithm>
#include "epicsExport.h"
#include "epicsThread.h"
#include "iocsh.h"
#include "db_access.h"
#include <iostream>
#include "GangServer.h"
#include "GangConnection.h"
#include "PerformanceMonitor.h"
#include "TakeLock.h"
#include "FreeLock.h"
#include "initHooks.h"

// Set this symbol to 1 if you want to be able to set
// an arbitary ROI and binning that uses the hardware
// as much as it can and finishes it off in software.
// Set this symbol to 0 if you want to only do hardware
// ROI and binning, the achieved values being reported
// back.
#define DO_SOFTWARE_ROI 0

/** Constants
 */
const int Pco::traceFlagsDllApi = 0x0100;
const int Pco::traceFlagsGang = 0x0400;
const int Pco::traceFlagsPerformance = 0x0800;
const int Pco::traceFlagsPcoState = 0x0200;
const int Pco::requestQueueCapacity = 10;
const int Pco::numHandles = 300;
const double Pco::reconnectPeriod = 4.0;
const double Pco::rebootPeriod = 10.0;
const double Pco::connectPeriod = 20.0;
const double Pco::statusPollPeriod = 2.0;
const double Pco::acquisitionStatusPollPeriod = 5.0;
const double Pco::armIgnoreImagesPeriod = 0.1;
const double Pco::initialisationPeriod = 1.0;
const int Pco::bitsPerShortWord = 16;
const int Pco::bitsPerNybble = 4;
const long Pco::nybbleMask = 0x0f;
const long Pco::bcdDigitValue = 10;
const int Pco::bcdPixelLength = 4;
const int Pco::binaryHeaderLength = 14;
const int Pco::defaultHorzBin = 1;
const int Pco::defaultVertBin = 1;
const int Pco::defaultRoiMinX = 1;
const int Pco::defaultRoiMinY = 1;
const int Pco::defaultExposureTime = 50;
const int Pco::defaultDelayTime = 0;
const int Pco::edgeXSizeNeedsReducedCamlink = 1920;
const int Pco::edgePixRateNeedsReducedCamlink = 286000000;
const int Pco::edgeBaudRate = 115200;
const double Pco::timebaseNanosecondsThreshold = 0.001;
const double Pco::timebaseMicrosecondsThreshold = 1.0;
const double Pco::oneNanosecond = 1e-9;
const double Pco::oneMillisecond = 1e-3;
const double Pco::triggerRetryPeriod = 0.01;
const int Pco::statusMessageSize = 256;

/** Aligned allocation for DLL buffers. */
#ifdef _WIN32
#else
#define _aligned_malloc(siz, align) malloc(siz)
#define _aligned_free(buff) free(buff)
#endif

/** The PCO object map
 */
std::map<std::string, Pco*> Pco::thePcos;

/** EPICS init hook
 */
void pcoInitHookFunction(initHookState state)
{
	std::map<std::string, Pco*>::iterator pos;
	switch(state)
	{
	case initHookAfterIocRunning:
		for(pos=Pco::thePcos.begin(); pos!=Pco::thePcos.end(); ++pos)
		{
			pos->second->initialiseOnceRunning();
		}
		break;
	default:
		break;
	}
}

/**
 * Constructor
 * \param[in] portName ASYN Port name
 * \param[in] maxSizeX frame size width
 * \param[in] maxSizeY frame size height
 * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
 *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
 * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
 *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
 */
Pco::Pco(const char* portName, int maxBuffers, size_t maxMemory)
: ADDriverEx(portName, 1, maxBuffers, maxMemory)
, paramPixRate(this, "PCO_PIX_RATE", 0)
, paramAdcMode(this, "PCO_ADC_MODE", DllApi::adcModeDual,
		new AsynParam::Notify<Pco>(this, &Pco::onAdcMode))
, paramCamRamUse(this, "PCO_CAM_RAM_USE", 0)
, paramElectronicsTemp(this, "PCO_ELECTRONICS_TEMP", 0.0)
, paramPowerTemp(this, "PCO_POWER_TEMP", 0.0)
, paramStorageMode(this, "PCO_STORAGE_MODE", DllApi::storageModeFifoBuffer)
, paramRecorderSubmode(this, "PCO_RECORDER_SUBMODE", DllApi::recorderSubmodeRingBuffer)
, paramTimestampMode(this, "PCO_TIMESTAMP_MODE", DllApi::timestampModeBinaryAndAscii)
, paramAcquireMode(this, "PCO_ACQUIRE_MODE", DllApi::acquireModeAuto)
, paramDelayTime(this, "PCO_DELAY_TIME", 0.0)
, paramArmMode(this, "PCO_ARM_MODE", 0, new AsynParam::Notify<Pco>(this, &Pco::onArmMode))
, paramImageNumber(this, "PCO_IMAGE_NUMBER", 0)
, paramCameraSetup(this, "PCO_CAMERA_SETUP", DllApi::edgeSetupRollingShutter, 
		new AsynParam::Notify<Pco>(this, &Pco::onReboot))
, paramBitAlignment(this, "PCO_BIT_ALIGNMENT", DllApi::bitAlignmentLsb)
, paramStateRecord(this, "PCO_STATERECORD", "")
, paramClearStateRecord(this, "PCO_CLEARSTATERECORD", 0, 
		new AsynParam::Notify<Pco>(this, &Pco::onClearStateRecord))
, paramHwBinX(this, "PCO_HWBINX", 0)
, paramHwBinY(this, "PCO_HWBINY", 0)
, paramHwRoiX1(this, "PCO_HWROIX1", 0)
, paramHwRoiY1(this, "PCO_HWROIY1", 0)
, paramHwRoiX2(this, "PCO_HWROIX2", 0)
, paramHwRoiY2(this, "PCO_HWROIY2", 0)
, paramXCamSize(this, "PCO_XCAMSIZE", 1280)
, paramYCamSize(this, "PCO_YCAMSIZE", 1024)
, paramCamlinkClock(this, "PCO_CAMLINKCLOCK", 0)
, paramMinCoolingSetpoint(this, "PCO_MINCOOLINGSETPOINT", 0)
, paramMaxCoolingSetpoint(this, "PCO_MAXCOOLINGSETPOINT", 0)
, paramDefaultCoolingSetpoint(this, "PCO_DEFAULTCOOLINGSETPOINT", 0)
, paramCoolingSetpoint(this, "PCO_COOLINGSETPOINT", 0, 
		new AsynParam::Notify<Pco>(this, &Pco::onCoolingSetpoint))
, paramDelayTimeMin(this, "PCO_DELAYTIMEMIN", 0.0)
, paramDelayTimeMax(this, "PCO_DELAYTIMEMAX", 0.0)
, paramDelayTimeStep(this, "PCO_DELAYTIMESTEP", 0.0)
, paramExpTimeMin(this, "PCO_EXPTIMEMIN", 0.0)
, paramExpTimeMax(this, "PCO_EXPTIMEMAX", 0.0)
, paramExpTimeStep(this, "PCO_EXPTIMESTEP", 0.0)
, paramMaxBinHorz(this, "PCO_MAXBINHORZ", 0)
, paramMaxBinVert(this, "PCO_MAXBINVERT", 0)
, paramBinHorzStepping(this, "PCO_BINHORZSTEPPING", 0)
, paramBinVertStepping(this, "PCO_BINVERTSTEPPING", 0)
, paramRoiHorzSteps(this, "PCO_ROIHORZSTEPS", 0)
, paramRoiVertSteps(this, "PCO_ROIVERTSTEPS", 0)
, paramReboot(this, "PCO_REBOOT", 1, new AsynParam::Notify<Pco>(this, &Pco::onReboot))
, paramCamlinkLongGap(this, "PCO_CAMLINKLONGGAP", 1)
, paramGangMode(this, "PCO_GANGMODE", gangModeNone)
, paramADAcquire(ADDriverEx::paramADAcquire, new AsynParam::Notify<Pco>(this, &Pco::onAcquire))
, paramADTemperature(ADDriverEx::paramADTemperature, 
		new AsynParam::Notify<Pco>(this, &Pco::onADTemperature))
, paramCameraRam(this, "PCO_CAM_RAM", 0)
, paramCameraBusy(this, "PCO_CAM_BUSY", 0)
, paramExpTrigger(this, "PCO_EXP_TRIGGER", 0)
, paramAcqEnable(this, "PCO_ACQ_ENABLE", 0)
, paramSerialNumber(this, "PCO_SERIAL_NUMBER", 0)
, paramHardwareVersion(this, "PCO_HARDWARE_VERSION", 0)
, paramFirmwareVersion(this, "PCO_FIRMWARE_VERSION", 0)
, paramCamRamUseFrames(this, "PCO_CAM_RAM_USE_FRAMES", 0)
, paramArmComplete(this, "PCO_ARM_COMPLETE", 0)
, paramConnected(this, "PCO_CONNECTED", 0)
, paramBuffersReady(this, "PCO_BUFFERS_READY", 0)
, paramIsEdge(this, "PCO_IS_EDGE", 0)
, paramGetImage(this, "PCO_GET_IMAGE", 0,
		new AsynParam::Notify<Pco>(this, &Pco::onGetImage))
, paramBuffersInUse(this, "PCO_BUFFERS_IN_USE", 0)
, paramDataFormat(this, "PCO_DATAFORMAT", 0)
, paramConfirmedStop(this, "PCO_CONFIRMEDSTOP", 0,
		new AsynParam::Notify<Pco>(this, &Pco::onConfirmedStop))
, paramApplyBinningAndRoi(this, "PCO_APPLY_BIN_ROI", 0,
		new AsynParam::Notify<Pco>(this, &Pco::onApplyBinningAndRoi))
, paramRoiPercentX(this, "PCO_ROIPCX", 100,
		new AsynParam::Notify<Pco>(this, &Pco::onRequestPercentageRoi))
, paramRoiPercentY(this, "PCO_ROIPCY", 100,
		new AsynParam::Notify<Pco>(this, &Pco::onRequestPercentageRoi))
, paramFriendlyRoiSetting(this, "PCO_ROI_FRIENDLY", 0)
, paramRoiSymmetryX(this, "PCO_ROI_SYMMETRY_X", 0)
, paramRoiSymmetryY(this, "PCO_ROI_SYMMETRY_Y", 0)
, paramuCName(this, "PCO_UC_NAME", "")
, paramuCFWVersion(this, "PCO_UC_FW_VERSION", "")
, paramFPGAName(this, "PCO_FPGA_NAME", "")
, paramFPGAFWVersion(this, "PCO_FPGA_FW_VERSION", "")
, paramzFPGAName(this, "PCO_zFPGA_NAME", "")
, paramzFPGAFWVersion(this, "PCO_zFPGA_FW_VERSION", "")
, paramXMLName(this, "PCO_XML_NAME", "")
, paramXMLFWVersion(this, "PCO_XML_FW_VERSION", "")
, paramphyuCName(this, "PCO_PHY_UC_NAME", "")
, paramphyuCFWVersion(this, "PCO_PHY_UC_FW_VERSION", "")
, paramInterfaceType(this, "PCO_INTERFACE", 0)
, paramInterfaceIsCameraLink(this, "PCO_USES_CAMERALINK", 0)
, stateMachine(NULL)
, triggerTimer(NULL)
, api(NULL)
, errorTrace(getAsynUser(), ASYN_TRACE_ERROR)
, apiTrace(getAsynUser(), Pco::traceFlagsDllApi)
, gangTrace(getAsynUser(), Pco::traceFlagsGang)
, performanceTrace(getAsynUser(), Pco::traceFlagsPerformance)
, stateTrace(getAsynUser(), Pco::traceFlagsPcoState)
, receivedImageQueue(1000, sizeof(NDArray*))
, gangServer(NULL)
, gangConnection(NULL)
, performanceMonitor(NULL)
{
    // Put in global map
    Pco::thePcos[portName] = this;
    // Hook EPICS initialisation
    initHookRegister(pcoInitHookFunction);
    // Initialise some base class parameters
    paramNDDataType = NDUInt16;
    paramADNumExposures = 1;
    paramADManufacturer = "PCO";
    paramADModel = "Unknown";
    paramADMaxSizeX = 0;
    paramADMaxSizeY = 0;
    paramNDArraySize = 0;
	paramADStatusMessage = "Disconnected";
	// The performance monitoring system
	performanceMonitor = new PerformanceMonitor(this, &performanceTrace);
    // We are not connected to a camera
    camera = NULL;
    // Initialise the buffers
    for(int i=0; i<Pco::numApiBuffers; i++)
    {
        buffers[i].bufferNumber = DllApi::bufferUnallocated;
        buffers[i].buffer = NULL;
        buffers[i].eventHandle = NULL;
        buffers[i].ready = false;
    }
    // Initialise the enum strings
    for(int i=0; i<DllApi::descriptionNumPixelRates; i++)
    {
        pixRateEnumValues[i] = 0;
        pixRateEnumStrings[i] = (char *)calloc(MAX_ENUM_STRING_SIZE, sizeof(char));
        pixRateEnumSeverities[i] = 0;
    }
    // Create the state machine
    stateMachine = new StateMachine("Pco", this,
            &paramStateRecord, &stateTrace, Pco::requestQueueCapacity);
    // States
	stateUninitialised = stateMachine->state("Uninitialised");
	stateUnconnected = stateMachine->state("Unconnected");
	stateIdle = stateMachine->state("Idle");
    stateArmed = stateMachine->state("Armed");
    stateAcquiring = stateMachine->state("Acquiring");
	stateUnarmedAcquiring = stateMachine->state("UnarmedAcquiring");
	stateExternalAcquiring = stateMachine->state("ExternalAcquiring");
	stateUnarmedDraining = stateMachine->state("UnarmedDraining");
	stateExternalDraining = stateMachine->state("ExternalDraining");
	stateDraining = stateMachine->state("Draining");
	// Events
    requestInitialise = stateMachine->event("Initialise");
	requestTimerExpiry = stateMachine->event("TimerExpiry");
	requestAcquire = stateMachine->event("Acquire");
	requestStop = stateMachine->event("Stop");
	requestArm = stateMachine->event("Arm");
	requestImageReceived = stateMachine->event("ImageReceived");
	requestDisarm = stateMachine->event("Disarm");
	requestTrigger = stateMachine->event("Trigger");
	requestReboot = stateMachine->event("Reboot");
	requestMakeImages = stateMachine->event("MakeImages");
	requestApplyBinningAndRoi = stateMachine->event("ApplyBinningAndRoi");
	// Transitions
	stateMachine->transition(stateUninitialised, requestInitialise, new StateMachine::Act<Pco>(this, &Pco::smInitialiseWait), stateUnconnected);
	stateMachine->transition(stateUninitialised, requestStop, new StateMachine::Act<Pco>(this, &Pco::smAlreadyStopped), stateUninitialised);
	stateMachine->transition(stateUnconnected, requestTimerExpiry, new StateMachine::Act<Pco>(this, &Pco::smConnectToCamera), stateIdle, stateUnconnected);
	stateMachine->transition(stateUnconnected, requestStop, new StateMachine::Act<Pco>(this, &Pco::smAlreadyStopped), stateUnconnected);
	stateMachine->transition(stateIdle, requestTimerExpiry, new StateMachine::Act<Pco>(this, &Pco::smPollWhileIdle), stateIdle);
	stateMachine->transition(stateIdle, requestArm, new StateMachine::Act<Pco>(this, &Pco::smRequestArm), stateArmed, stateIdle);
	stateMachine->transition(stateIdle, requestAcquire, new StateMachine::Act<Pco>(this, &Pco::smArmAndAcquire), stateUnarmedAcquiring, stateIdle);
	stateMachine->transition(stateIdle, requestImageReceived, new StateMachine::Act<Pco>(this, &Pco::smDiscardImages), stateIdle);
	stateMachine->transition(stateIdle, requestReboot, new StateMachine::Act<Pco>(this, &Pco::smRequestReboot), stateUnconnected);
	stateMachine->transition(stateIdle, requestStop, new StateMachine::Act<Pco>(this, &Pco::smAlreadyStopped), stateIdle);
	stateMachine->transition(stateArmed, requestTimerExpiry, new StateMachine::Act<Pco>(this, &Pco::smPollWhileAcquiring), stateArmed);
	stateMachine->transition(stateArmed, requestAcquire, new StateMachine::Act<Pco>(this, &Pco::smAcquire), stateAcquiring);
	stateMachine->transition(stateArmed, requestImageReceived, new StateMachine::Act<Pco>(this, &Pco::smFirstImageWhileArmed), stateExternalAcquiring, stateIdle, stateArmed, stateArmed);
	stateMachine->transition(stateArmed, requestDisarm, new StateMachine::Act<Pco>(this, &Pco::smDisarmAndDiscard), stateIdle);
	stateMachine->transition(stateArmed, requestStop, new StateMachine::Act<Pco>(this, &Pco::smDisarmAndDiscard), stateIdle);
	stateMachine->transition(stateAcquiring, requestTimerExpiry, new StateMachine::Act<Pco>(this, &Pco::smPollWhileAcquiring), stateAcquiring);
	stateMachine->transition(stateAcquiring, requestImageReceived, new StateMachine::Act<Pco>(this, &Pco::smAcquireImage), stateAcquiring, stateIdle, stateArmed, stateDraining);
	stateMachine->transition(stateAcquiring, requestMakeImages, new StateMachine::Act<Pco>(this, &Pco::smMakeGangedImage), stateAcquiring, stateIdle, stateArmed);
	stateMachine->transition(stateAcquiring, requestTrigger, new StateMachine::Act<Pco>(this, &Pco::smTrigger), stateAcquiring);
	stateMachine->transition(stateAcquiring, requestStop, new StateMachine::Act<Pco>(this, &Pco::smStopAcquisition), stateIdle, stateArmed);
	stateMachine->transition(stateAcquiring, requestAcquire, new StateMachine::Act<Pco>(this, &Pco::smTrigger), stateAcquiring);
	stateMachine->transition(stateDraining, requestImageReceived, new StateMachine::Act<Pco>(this, &Pco::smDrainImage), stateDraining, stateIdle, stateArmed);
	stateMachine->transition(stateDraining, requestTimerExpiry, new StateMachine::Act<Pco>(this, &Pco::smPollWhileDraining), stateDraining);
	stateMachine->transition(stateDraining, requestStop, new StateMachine::Act<Pco>(this, &Pco::smStopAcquisition), stateIdle, stateArmed);
	stateMachine->transition(stateExternalAcquiring, requestTimerExpiry, new StateMachine::Act<Pco>(this, &Pco::smPollWhileAcquiring), stateExternalAcquiring);
	stateMachine->transition(stateExternalAcquiring, requestImageReceived, new StateMachine::Act<Pco>(this, &Pco::smExternalAcquireImage), stateExternalAcquiring, stateIdle, stateArmed, stateExternalDraining);
	stateMachine->transition(stateExternalAcquiring, requestMakeImages, new StateMachine::Act<Pco>(this, &Pco::smMakeGangedImage), stateExternalAcquiring, stateIdle, stateArmed);
	stateMachine->transition(stateExternalAcquiring, requestStop, new StateMachine::Act<Pco>(this, &Pco::smStopAcquisition), stateIdle, stateArmed);
	stateMachine->transition(stateExternalAcquiring, requestAcquire, new StateMachine::Act<Pco>(this, &Pco::smAcquire), stateExternalAcquiring);
	stateMachine->transition(stateExternalDraining, requestImageReceived, new StateMachine::Act<Pco>(this, &Pco::smExternalDrainImage), stateExternalDraining, stateIdle, stateArmed);
	stateMachine->transition(stateExternalDraining, requestTimerExpiry, new StateMachine::Act<Pco>(this, &Pco::smPollWhileDraining), stateExternalDraining);
	stateMachine->transition(stateExternalDraining, requestStop, new StateMachine::Act<Pco>(this, &Pco::smStopAcquisition), stateIdle, stateArmed);
	stateMachine->transition(stateUnarmedAcquiring, requestTimerExpiry, new StateMachine::Act<Pco>(this, &Pco::smPollWhileAcquiring), stateUnarmedAcquiring);
	stateMachine->transition(stateUnarmedAcquiring, requestImageReceived, new StateMachine::Act<Pco>(this, &Pco::smUnarmedAcquireImage), stateUnarmedAcquiring, stateIdle, stateUnarmedDraining);
	stateMachine->transition(stateUnarmedAcquiring, requestMakeImages, new StateMachine::Act<Pco>(this, &Pco::smUnarmedMakeGangedImage), stateUnarmedAcquiring, stateIdle);
	stateMachine->transition(stateUnarmedAcquiring, requestTrigger, new StateMachine::Act<Pco>(this, &Pco::smTrigger), stateUnarmedAcquiring);
	stateMachine->transition(stateUnarmedAcquiring, requestStop, new StateMachine::Act<Pco>(this, &Pco::smExternalStopAcquisition), stateIdle);
	stateMachine->transition(stateUnarmedDraining, requestImageReceived, new StateMachine::Act<Pco>(this, &Pco::smUnarmedDrainImage), stateUnarmedDraining, stateIdle);
	stateMachine->transition(stateUnarmedDraining, requestTimerExpiry, new StateMachine::Act<Pco>(this, &Pco::smPollWhileDraining), stateUnarmedDraining);
	stateMachine->transition(stateUnarmedDraining, requestStop, new StateMachine::Act<Pco>(this, &Pco::smExternalStopAcquisition), stateIdle);
	stateMachine->transition(stateIdle, requestApplyBinningAndRoi, new StateMachine::Act<Pco>(this, &Pco::smApplyBinningAndRoi), stateIdle);
	// State machine starting state
	stateMachine->initialState(stateUninitialised);
	// A timer for the trigger
    triggerTimer = new StateMachine::Timer(stateMachine);
}

/**
 * Destructor
 */
Pco::~Pco()
{
    try
    {
        api->setRecordingState(this->camera, DllApi::recorderStateOff);
        api->cancelImages(this->camera);
        for(int i=0; i<Pco::numApiBuffers; i++)
        {
            api->freeBuffer(camera, i);
        }
        api->closeCamera(camera);
    }
    catch(PcoException&)
    {
    }
    for(int i=0; i<Pco::numApiBuffers; i++)
    {
        if(buffers[i].buffer != NULL)
        {
			_aligned_free(buffers[i].buffer);
        }
    }
    delete triggerTimer;
    delete stateMachine;
    delete performanceMonitor;
}

/**
 * Connects the DLL API to the main PCO class.
 */
void Pco::registerDllApi(DllApi* api)
{
    this->api = api;
}

/**
 * This function is called by the EPICS init hook once the IOC is running.
 */
void Pco::initialiseOnceRunning()
{
    stateMachine->startTimer(Pco::initialisationPeriod, Pco::requestInitialise);
}

/**
 * Return the pco corresponding to the port name
 * \param[in] p The port name
 * \return The pco object, NULL if not found
 */
Pco* Pco::getPco(const char* portName)
{
    Pco* result = NULL;
    std::map<std::string, Pco*>::iterator pos = Pco::thePcos.find(portName);
    if(pos != Pco::thePcos.end())
    {
        result = pos->second;
    }
    return result;
}

/**
 * Reboot the camera
 */
void Pco::doReboot()
{
	// TODO: check if this parameter being set (used for embedded PCO screen) fixes pixel rate selection
	paramConnected = 0;
	TakeLock takeLock(this);
	unsigned long setupData[DllApi::cameraSetupDataSize];
	unsigned short setupDataLen = DllApi::cameraSetupDataSize;
	unsigned short setupType;
	this->api->getCameraSetup(this->camera, &setupType, setupData, &setupDataLen);
	api->setTimeouts(this->camera, 2000, 3000, 250);
	if(paramCameraSetup == DllApi::cameraSetupRollingShutter ||
			paramCameraSetup == DllApi::cameraSetupGlobalShutter)
	{
		setupData[0] = paramCameraSetup;
	}
	this->api->setCameraSetup(this->camera, setupType, setupData, setupDataLen);
	api->rebootCamera(this->camera);
    api->closeCamera(this->camera);
    camera = NULL;
}

/**
 * Output a message to the status PV.
 * \param[in] text The message to output
 */
void Pco::outputStatusMessage(const char* text)
{
	TakeLock takeLock(this);
    paramADStatusMessage = text;
}

/**
 * Trigger the wait before we try to connect to the camera.
 * returns: firstState: always
 */
StateMachine::StateSelector Pco::smInitialiseWait()
{
	TakeLock takeLock(this);
	performanceMonitor->count(takeLock, PerformanceMonitor::PERF_REBOOT, /*fault=*/false);
    stateMachine->startTimer(Pco::connectPeriod, Pco::requestTimerExpiry);
    return StateMachine::firstState;
}

/**
 * Connect to the camera
 * returns: firstState: success
 *          secondState: failure
 */
StateMachine::StateSelector Pco::smConnectToCamera()
{

	StateMachine::StateSelector result;
	TakeLock takeLock(this);
    // Close the camera if we think it might be open
    if(camera != NULL)
    {
        try
        {
            api->closeCamera(camera);
        }
        catch(PcoException&)
        {
            // Swallow errors from this
        }
    }
    // Now try to open it again
    try
    {
        // Open and initialise the camera
        camera = NULL;
        api->openCamera(&camera, 0);
        initialiseCamera(takeLock);
        discardImages();
        stateMachine->startTimer(Pco::statusPollPeriod, Pco::requestTimerExpiry);
        outputStatusMessage("");
        performanceMonitor->count(takeLock, PerformanceMonitor::PERF_CONNECT, /*fault=*/false);
        result = StateMachine::firstState;
    }
    catch(PcoException&)
    {
        stateMachine->startTimer(Pco::reconnectPeriod, Pco::requestTimerExpiry);
        result = StateMachine::secondState;
    }
    return result;
}

/**
 * Poll the camera while it is not taking image.
 * returns: firstState: always
 */
StateMachine::StateSelector Pco::smPollWhileIdle()
{
    pollCameraNoAcquisition();
    pollCamera();
	paramConnected = 1;
    stateMachine->startTimer(Pco::statusPollPeriod, Pco::requestTimerExpiry);
    return StateMachine::firstState;
}

/**
 * Poll the camera while it is taking images (or is armed).
 * returns: firstState: always
 */
StateMachine::StateSelector Pco::smPollWhileAcquiring()
{
	// Don't poll for edge type cameras, there's nothing of interest
	// and it gets in the way of fast acquisitions.
	if(this->camType.camType != DllApi::cameraTypeEdge &&
			this->camType.camType != DllApi::cameraTypeEdgeGl &&
			this->camType.camType != DllApi::cameraTypeEdgeCLHS)
	{
		TakeLock takeLock(&this->apiLock);
		try
		{
			pollCameraAcquisition();
			pollCamera();
		}
		catch(PcoException&)
		{
		}
	}
    stateMachine->startTimer(Pco::acquisitionStatusPollPeriod, Pco::requestTimerExpiry);
    return StateMachine::firstState;
}

/**
 * Poll the camera while it is draining images
 * returns: firstState: always
 */
StateMachine::StateSelector Pco::smPollWhileDraining()
{
	// Actually, nothing to poll at this time.
    stateMachine->startTimer(Pco::acquisitionStatusPollPeriod, Pco::requestTimerExpiry);
    return StateMachine::firstState;
}

/**
 * Try to arm the camera
 * returns: firstState: success
 *          secondState: failure
 */
StateMachine::StateSelector Pco::smRequestArm()
{
	StateMachine::StateSelector result;
    try
    {
		try
		{
			doArm();
		}
		catch(std::exception&)
		{
			// Sometimes the arm fails in a way that is fixed by the clean up.
			// This especially happens on the way back from a hardware ROI.
			// So we disarm and try once more if that happens.
			doDisarm();
			doArm();
		}
        outputStatusMessage("");
        result = StateMachine::firstState;
		paramArmComplete = 1;
    }
    catch(std::exception& e)
    {
		{
			TakeLock(&this->apiLock);
			this->api->stopFrameCapture();
		}
        doDisarm();
        errorTrace << "Failed to arm, " << e.what() << std::endl;
        outputStatusMessage(e.what());
		TakeLock takeLock(this);
		paramADStatus = ADStatusIdle;
		paramADAcquire = 0;
        result = StateMachine::secondState;
    }
    return result;
}

/**
 * Arm the camera and start acquiring images
 * returns: firstState: success
 *          secondState: failure
 */
StateMachine::StateSelector Pco::smArmAndAcquire()
{
	StateMachine::StateSelector result;
    try
    {
		try
		{
			doArm();
		}
		catch(std::exception&)
		{
			// Sometimes the arm fails in a way that is fixed by the clean up.
			// This especially happens on the way back from a hardware ROI.
			// So we disarm and try once more if that happens.
			doDisarm();
			doArm();
		}
        outputStatusMessage("");
        result = StateMachine::firstState;
		{
			TakeLock takeLock(this);
			paramArmComplete = 1;
		}
		this->nowAcquiring();
		this->startCamera();
    }
    catch(std::exception& e)
    {
		{
			TakeLock(&this->apiLock);
			this->api->stopFrameCapture();
		}
        doDisarm();
        errorTrace << "Failed to arm, " << e.what() << std::endl;
        outputStatusMessage(e.what());
		TakeLock takeLock(this);
		paramADStatus = ADStatusIdle;
		paramADAcquire = 0;
        result = StateMachine::secondState;
    }
    return result;
}

/**
 * Start an already armed camera.
 * returns firstState: always
 */
StateMachine::StateSelector Pco::smAcquire()
{
    this->nowAcquiring();
    this->startCamera();
    return StateMachine::firstState;
}

/**
 * Discard all queued images
 * Returns: firstState: always
 */
StateMachine::StateSelector Pco::smDiscardImages()
{
    discardImages();
    return StateMachine::firstState;
}

/**
 * Start the reboot of a camera
 * Returns: firstState: always
 */
StateMachine::StateSelector Pco::smRequestReboot()
{
	// We need to stop the poll timer and discard any events that have
	// already been passed to the state machine
	stateMachine->stopTimer();
	stateMachine->clear();
	// Now do the reboot
	doReboot();
    stateMachine->startTimer(Pco::rebootPeriod, Pco::requestTimerExpiry);
    outputStatusMessage("Disconnected");
    return StateMachine::firstState;
}

/**
 * Handle the first image received once the camera is armed.
 * Returns: firstState: further images to be acquired
 *          secondState: acquisition complete and disarmed
 *          thirdState: acquisition complete and still armed
 *          fourthState: image discarded and still armed
 */
StateMachine::StateSelector Pco::smFirstImageWhileArmed()
{
	StateMachine::StateSelector result;
    if(triggerMode != DllApi::triggerSoftware)
    {
        nowAcquiring();
        if(!receiveImages())
        {
        	result = StateMachine::firstState;
        }
        else if(triggerMode == DllApi::triggerAuto)
        {
            acquisitionComplete();
            doDisarm();
            result = StateMachine::secondState;
        }
        else
        {
            this->acquisitionComplete();
            result = StateMachine::thirdState;
        }
    }
    else
    {
        discardImages();
        result = StateMachine::fourthState;
    }
    return result;
}

/**
 * Handle an image during an acquisition.
 * Returns: firstState: further images to be acquired
 *          secondState: acquisition complete and disarmed
 *          thirdState: acquisition complete and still armed
 *          fourthState: start draining memory
 */
StateMachine::StateSelector Pco::smAcquireImage()
{
	StateMachine::StateSelector result;
    if(!receiveImages())
    {
        //startCamera();
        result = StateMachine::firstState;
    }
    else
    {
		if(paramStorageMode == DllApi::storageModeRecorder)
		{
			// Burst mode, start reading the memory
			readFirstMemoryImage();
			result = StateMachine::fourthState;
		}
		else if((triggerMode == DllApi::triggerAuto) || (triggerMode == DllApi::triggerExternalOnly))
		{
			// Normal mode, automatic triggering
			acquisitionComplete();
			doDisarm();
			result = StateMachine::secondState;
		}
		else
		{
			// Normal mode, non-automatic triggering
			acquisitionComplete();
			result = StateMachine::thirdState;
		}
    }
    return result;
}

/**
 * Handle an image during an unarmed acquisition.
 * Returns: firstState: further images to be acquired
 *          secondState: acquisition complete and disarmed
 *          thirdState: acquisition complete images to be read from the memory
 */
StateMachine::StateSelector Pco::smUnarmedAcquireImage()
{
	StateMachine::StateSelector result;
    if(!receiveImages())
    {
		// More images to be received
        startCamera();
        result = StateMachine::firstState;
    }
    else
    {
		if(paramStorageMode == DllApi::storageModeRecorder)
		{
			// Burst mode, start reading the memory
			readFirstMemoryImage();
			result = StateMachine::thirdState;
		}
		else
		{
			// Normal mode, acquisition complete
			acquisitionComplete();
			doDisarm();
			discardImages();
			result = StateMachine::secondState;
		}
    }
    return result;
}

/**
 * Handle an image during unarmed memory draining.
 * Returns: firstState: further images to be drained
 *          secondState: acquisition complete and disarmed
 */
StateMachine::StateSelector Pco::smUnarmedDrainImage()
{
	StateMachine::StateSelector result;
	// Process this image and read next
	if(readNextMemoryImage())
	{
		// More to handle
		result = StateMachine::firstState;
	}
	else
	{
		// Draining complete
	    this->api->clearRamSegment(this->camera);
		acquisitionComplete();
		doDisarm();
		discardImages();
		result = StateMachine::secondState;
	}
	return result;
}

/**
 * Handle an image during an externally triggered acquisition.
 * Returns: firstState: further images to be acquired
 *          secondState: acquisition complete and disarmed
 *          thirdState: acquisition complete and still armed
 *          fourthState: start draining memory
 */
StateMachine::StateSelector Pco::smExternalAcquireImage()
{
	StateMachine::StateSelector result;
    if(!receiveImages())
    {
        result = StateMachine::firstState;
    }
    else
    {
		if(paramStorageMode == DllApi::storageModeRecorder)
		{
			// Burst mode, start reading the memory
			readFirstMemoryImage();
			result = StateMachine::fourthState;
		}
		else if(triggerMode == DllApi::triggerAuto)
		{
			// Normal mode, automatic triggering
			acquisitionComplete();
			doDisarm();
			result = StateMachine::secondState;
		}
		else
		{
			// Normal mode, non-automatic triggering
			acquisitionComplete();
			result = StateMachine::thirdState;
		}
    }
    return result;
}

/**
 * Handle an image during memory draining.
 * Returns: firstState: further images to be drained
 *          secondState: acquisition complete and disarmed
 *          thirdState: acquisition complete and still armed
 */
StateMachine::StateSelector Pco::smDrainImage()
{
	StateMachine::StateSelector result;
	// Process this image and read next
	if(readNextMemoryImage())
	{
		// More draining to perform
		result = StateMachine::firstState;
	}
	else if((triggerMode == DllApi::triggerAuto) || (triggerMode == DllApi::triggerExternalOnly))
	{
		// Complete and disarmed
	    this->api->clearRamSegment(this->camera);
		acquisitionComplete();
		doDisarm();
		discardImages();
		result = StateMachine::secondState;
	}
	else
	{
		// Complete but still armed
	    this->api->clearRamSegment(this->camera);
		acquisitionComplete();
		discardImages();
		result = StateMachine::thirdState;
	}
	return result;
}

/**
 * Handle an image during unarmed memory draining.
 * Returns: firstState: further images to be drained
 *          secondState: acquisition complete and disarmed
 *          thirdState: acquisition complete and still armed
 */
StateMachine::StateSelector Pco::smExternalDrainImage()
{
	StateMachine::StateSelector result;
	// Process this image and read next
	if(readNextMemoryImage())
	{
		// More to handle
		result = StateMachine::firstState;
	}
	else if(triggerMode == DllApi::triggerAuto)
	{
		// Normal mode, automatic triggering
	    this->api->clearRamSegment(this->camera);
		acquisitionComplete();
		doDisarm();
		discardImages();
		result = StateMachine::secondState;
	}
	else
	{
		// Normal mode, non-automatic triggering
	    this->api->clearRamSegment(this->camera);
		acquisitionComplete();
		discardImages();
		result = StateMachine::thirdState;
	}
	return result;
}

/**
 * A stop is received but we are already stopped.
 * Returns: firstState: always
 */
StateMachine::StateSelector Pco::smAlreadyStopped()
{
	// Release the stop confirm busy record
	TakeLock takeLock(this);
	paramConfirmedStop = 0;
	return StateMachine::firstState;
}

/**
 * Return true if the camera requires that a region of
 * interest be symmetrical in the horizontal direction
 */
bool Pco::roiSymmetryRequiredX() {
	return (this->adcMode == DllApi::adcModeDual ||
			this->camType.camType == DllApi::cameraTypeDimaxStd ||
			this->camType.camType == DllApi::cameraTypeDimaxTv ||
			this->camType.camType == DllApi::cameraTypeDimaxAutomotive);
}

/**
 * Return true if the camera requires that a region of
 * interest be symmetrical in the vertical direction
 */
bool Pco::roiSymmetryRequiredY() {
	return (this->camType.camType == DllApi::cameraTypeEdge ||
			this->camType.camType == DllApi::cameraTypeEdgeGl ||
			this->camType.camType == DllApi::cameraTypeEdgeCLHS ||
			this->camType.camType == DllApi::cameraTypeDimaxStd ||
			this->camType.camType == DllApi::cameraTypeDimaxTv ||
			this->camType.camType == DllApi::cameraTypeDimaxAutomotive);
}

/**
 * Validate and set the ROI and binning while the camera is idle
 * Returns: firstState: always
 */
StateMachine::StateSelector Pco::smApplyBinningAndRoi()
{
	// Apply the binning and ROI settings
	bool andUpdateParameters = true;

	this->cfgBinningAndRoi( andUpdateParameters );

	return StateMachine::firstState;
}

/**
 * Start reading the camera's internal memory
 */
void Pco::readFirstMemoryImage()
{
	// Read the first image
	this->api->cancelImages(this->camera);
	try
	{
		this->api->setRecordingState(this->camera, DllApi::recorderStateOff);
	}
	catch(PcoException&)
	{
	}
	memoryImageCounter = 1;
	this->api->getImageEx(this->camera, /*segment=*/1, memoryImageCounter,
		memoryImageCounter, /*bufferNumber=*/0, 
		this->xCamSize, this->yCamSize, this->camDescription.dynResolution);
	discardImages();
	// Get an ND array
	NDArray* image = allocArray(this->xCamSize, this->yCamSize, NDUInt16);
	if(image != NULL)
	{
		// Copy the image into an NDArray
		::memcpy(image->pData, this->buffers[0].buffer,
				this->xCamSize*this->yCamSize*sizeof(unsigned short));
		// And pass it to the state machine
		this->receivedImageQueue.send(&image, sizeof(NDArray*));
	}
	this->post(Pco::requestImageReceived);
}

/**
 * Process the previous image and read the next image from the camera's internal memory.
 * Returns true if another image is read.
 */
bool Pco::readNextMemoryImage()
{
	bool result = false;
	// Get the image from the queue
	NDArray* image;
    this->receivedImageQueue.tryReceive(&image, sizeof(NDArray*));
	// Continue processing the image
	if(image != NULL)
	{
		validateAndProcessFrame(image);
	}
	// Update statistics
	TakeLock takeLock(this);
	paramADNumExposuresCounter = this->numExposuresCounter;
	paramImageNumber = this->lastImageNumber;
	paramCamRamUseFrames = paramCamRamUseFrames - 1;
	memoryImageCounter++;
	// Are we finished?
	if(this->numImagesCounter < this->numImages)
	{
		// More images to read
		this->api->getImageEx(this->camera, /*segment=*/1, memoryImageCounter,
			memoryImageCounter, /*bufferNumber=*/0, 
			this->xCamSize, this->yCamSize, this->camDescription.dynResolution);
		// Get an ND array
		NDArray* image = allocArray(this->xCamSize, this->yCamSize, NDUInt16);
		if(image != NULL)
		{
			// Copy the image into an NDArray
			::memcpy(image->pData, this->buffers[0].buffer,
					this->xCamSize*this->yCamSize*sizeof(unsigned short));
			// And pass it to the state machine
			this->receivedImageQueue.send(&image, sizeof(NDArray*));
		}
		this->post(Pco::requestImageReceived);
		result = true;
	}
	return result;
}

/**
 * Try and make stitched images in the full control ganged mode.
 * Returns: firstState: further images to be acquired
 *          secondState: acquisition complete and disarmed
 *          thirdState: acquisition complete and still armed
 */
StateMachine::StateSelector Pco::smMakeGangedImage()
{
	StateMachine::StateSelector result;
	if(!makeImages())
	{
        result = StateMachine::firstState;
	}
    else if(triggerMode != DllApi::triggerSoftware)
    {
        acquisitionComplete();
        doDisarm();
        result = StateMachine::secondState;
    }
    else
    {
        acquisitionComplete();
        result = StateMachine::thirdState;
    }
    return result;
}

/**
 * Try and make stiched images in the full control ganged mode during unarmed acquisition.
 * Returns: firstState: further images to be acquired
 *          secondState: acquisition complete and disarmed
 */
StateMachine::StateSelector Pco::smUnarmedMakeGangedImage()
{
	StateMachine::StateSelector result;
	if(!makeImages())
	{
        result = StateMachine::firstState;
	}
    else
    {
        acquisitionComplete();
        doDisarm();
        discardImages();
        result = StateMachine::secondState;
    }
    return result;
}

/**
 * Disarm the camera and discard any images in the queues.
 * Returns: firstState: always
 */
StateMachine::StateSelector Pco::smDisarmAndDiscard()
{
	acquisitionComplete();
    doDisarm();
    discardImages();
    // Release the stop busy record
	TakeLock takeLock(this);
	paramConfirmedStop = 0;
	return StateMachine::firstState;
}

/**
 * Software trigger the camera
 * Returns: firstState: always
 */
StateMachine::StateSelector Pco::smTrigger()
{
    startCamera();
    return StateMachine::firstState;
}

/**
 * Stop the camera acquiring
 * Returns: firstState: camera is stopped and disarmed
 *          secondState: camera is stopped but still armed
 */
StateMachine::StateSelector Pco::smStopAcquisition()
{
	StateMachine::StateSelector result;
    if(triggerMode != DllApi::triggerSoftware)
    {
        acquisitionComplete();
        doDisarm();
		discardImages();
        result = StateMachine::firstState;
    }
    else
    {
        acquisitionComplete();
		discardImages();
        result = StateMachine::secondState;
    }
    // Release the stop busy record
	TakeLock takeLock(this);
	paramConfirmedStop = 0;
    return result;
}

/**
 * Stop the camera acquiring when triggered by an external trigger
 * Returns: firstState: always
 */
StateMachine::StateSelector Pco::smExternalStopAcquisition()
{
	acquisitionComplete();
	doDisarm();
	discardImages();
    // Release the stop busy record
	TakeLock takeLock(this);
	paramConfirmedStop = 0;
	return StateMachine::firstState;
}


/** Initialise the camera
 */
void Pco::initialiseCamera(TakeLock& takeLock)
{
	// Get various camera data
	api->getGeneral(camera);
	api->getCameraType(camera, &camType);
	api->getSensorStruct(camera);
	api->getCameraDescription(camera, &camDescription);
	api->getTimingStruct(camera);
	api->getStorageStruct(camera, &camStorage);
	api->getRecordingStruct(camera);

	// Get camera firmware
	api->getFirmwareInfo(camera, 0, &firmware);
	paramuCName = firmware.uCName;
	paramuCFWVersion = firmware.uCVersion;
	paramphyuCName = firmware.phyuCName;
	paramphyuCFWVersion = firmware.phyuCVersion;
	paramFPGAName = firmware.FPGAName;
	paramFPGAFWVersion = firmware.FPGAVersion;
	paramzFPGAName = firmware.zFPGAName;
	paramzFPGAFWVersion = firmware.zFPGAVersion;
	paramXMLName = firmware.XMLName;
	paramXMLFWVersion = firmware.XMLVersion;

	// Corrections for values that appear to be incorrectly returned by the SDK
	switch(this->camType.camType)
	{
	case DllApi::cameraTypeDimaxStd:
	case DllApi::cameraTypeDimaxTv:
	case DllApi::cameraTypeDimaxAutomotive:
		camDescription.roiVertSteps = 4;
		break;
	default:
		break;
	}

	// reset the camera
	try
	{
		api->setRecordingState(camera, DllApi::recorderStateOff);
	}
	catch(PcoException&)
	{
		// Swallow errors from this
	}
#if 0
	try
	{
		api->resetSettingsToDefault(camera);
	}
	catch(PcoException&)
	{
		// Swallow errors from this
	}
#endif
	// Camera RAM size
	paramCameraRam = camStorage.ramSizePages * camStorage.pageSizePixels * 
		sizeof(unsigned short) / 1048576; // MBytes

	// Record binning and roi capabilities
	paramMaxBinHorz = (int)camDescription.maxBinHorz;
	paramMaxBinVert = (int)camDescription.maxBinVert;
	paramBinHorzStepping = (int)camDescription.binHorzStepping;
	paramBinVertStepping = (int)camDescription.binVertStepping;
	paramRoiHorzSteps = (int)camDescription.roiHorSteps;
	paramRoiVertSteps = (int)camDescription.roiVertSteps;

	// Build the set of binning values
	setValidBinning(availBinX, camDescription.maxBinHorz,
			camDescription.binHorzStepping);
	setValidBinning(availBinY, camDescription.maxBinVert,
			camDescription.binVertStepping);

	// Deduce camera ROI symmetry requirements
	this->paramRoiSymmetryX = roiSymmetryRequiredX();
	this->paramRoiSymmetryY = roiSymmetryRequiredY();

	// Get more camera information
	this->api->getTransferParameters(this->camera, &this->camTransfer);
	this->api->getSizes(this->camera, &this->camSizes);
	paramADMaxSizeX = (int)this->camDescription.maxHorzRes;
	paramADMaxSizeY = (int)this->camDescription.maxVertRes;
	paramADSizeX = (int)this->camSizes.xResActual;
	paramADSizeY = (int)this->camSizes.yResActual;
	paramCamlinkClock = (int)this->camTransfer.clockFrequency;

	// Initialise the cooling setpoint information
	paramMinCoolingSetpoint = this->camDescription.minCoolingSetpoint;
	paramMaxCoolingSetpoint = this->camDescription.maxCoolingSetpoint;
	paramDefaultCoolingSetpoint = this->camDescription.defaultCoolingSetpoint;
	paramCoolingSetpoint = this->camDescription.defaultCoolingSetpoint;
	this->onCoolingSetpoint(takeLock);

	// Acquisition timing parameters
	paramDelayTimeMin = (double)this->camDescription.minDelayNs * 1e-9;
	paramDelayTimeMax = (double)this->camDescription.maxDelayMs * 1e-3;
	paramDelayTimeStep = (double)this->camDescription.minDelayStepNs * 1e-9;
	paramExpTimeMin = (double)this->camDescription.minExposureNs * 1e-9;
	paramExpTimeMax = (double)this->camDescription.maxExposureMs * 1e-3;
	paramExpTimeStep = (double)this->camDescription.minExposureStepNs * 1e-9;

	// Update area detector information strings
	switch(this->camType.camType)
	{
	case DllApi::cameraType1200Hs:
		paramADModel = "pco.1200";
		break;
	case DllApi::cameraType1300:
		paramADModel = "pco.1300";
		break;
	case DllApi::cameraType1600:
		paramADModel = "pco.1600";
		break;
	case DllApi::cameraType2000:
		paramADModel = "pco.2000";
		break;
	case DllApi::cameraType4000:
		paramADModel = "pco.4000";
		break;
	case DllApi::cameraTypeEdge:
		paramADModel = "pco.edge GL";
		break;
	case DllApi::cameraTypeEdgeGl:
		paramADModel = "pco.edge GL";
		break;
	case DllApi::cameraTypeEdgeCLHS:
		paramADModel = "pco.edge CLHS";
		break;
	case DllApi::cameraTypeDimaxStd:
	case DllApi::cameraTypeDimaxTv:
	case DllApi::cameraTypeDimaxAutomotive:
		paramADModel = "pco.dimax";
		break;
	default:
		paramADModel = "Unknown pco";
		break;
	}
	paramADManufacturer = "PCO";
	paramSerialNumber = (int)this->camType.serialNumber;
	paramHardwareVersion = (int)this->camType.hardwareVersion;
	paramFirmwareVersion = (int)this->camType.firmwareVersion;
	paramIsEdge = (int)(
			this->camType.camType == DllApi::cameraTypeEdge ||
			this->camType.camType == DllApi::cameraTypeEdgeGl ||
			this->camType.camType == DllApi::cameraTypeEdgeCLHS);
	paramInterfaceType = (int)this->camType.interfaceType;
	if (paramInterfaceType == DllApi::cameraLink) paramInterfaceIsCameraLink = 1;

	// Work out how to decode the BCD frame number in the image
	this->shiftLowBcd = Pco::bitsPerShortWord - this->camDescription.dynResolution;
	this->shiftHighBcd = this->shiftLowBcd + Pco::bitsPerNybble;

	// Set the camera clock
	this->setCameraClock();

	// Handle the pixel rates
	this->initialisePixelRate();

	// Make Edge specific function calls
	if(this->camType.camType == DllApi::cameraTypeEdge || 
			this->camType.camType == DllApi::cameraTypeEdgeGl ||
			this->camType.camType == DllApi::cameraTypeEdgeCLHS)
	{
		// Get Edge camera setup mode
		unsigned long setupData[DllApi::cameraSetupDataSize];
		unsigned short setupDataLen = DllApi::cameraSetupDataSize;
		unsigned short setupType = 0;
		this->api->getCameraSetup(this->camera, &setupType, setupData, &setupDataLen);
		paramCameraSetup = setupData[0];
	}

	// Set the default binning
	this->api->setBinning(this->camera, Pco::defaultHorzBin, Pco::defaultVertBin);
	paramADBinX = Pco::defaultHorzBin;
	paramADBinY = Pco::defaultVertBin;

	// Set the default ROI (apparently a must do step)
	int roix1, roix2, roiy1, roiy2; // region of interest
	// to maximise in x dimension
	roix1 = Pco::defaultRoiMinX;
	roix2 = this->camDescription.maxHorzRes/Pco::defaultHorzBin/
			this->camDescription.roiHorSteps;
	roix2 *= this->camDescription.roiHorSteps;
	// to maximise in y dimension
	roiy1 = Pco::defaultRoiMinY;
	roiy2 = this->camDescription.maxVertRes/Pco::defaultVertBin/
			this->camDescription.roiVertSteps;
	roiy2 *= this->camDescription.roiVertSteps;
	this->api->setRoi(this->camera,
			(unsigned short)roix1, (unsigned short)roiy1,
			(unsigned short)roix2, (unsigned short)roiy2);
	paramADMinX = roix1-1;
	paramADMinY = roiy1-1;
	paramADSizeX = roix2-roix1+1;
	paramADSizeY = roiy2-roiy1+1;

	// Set initial trigger mode to auto
	this->api->setTriggerMode(this->camera, DllApi::triggerExternal);

	// Set the storage mode to FIFO
	this->api->setStorageMode(this->camera, DllApi::storageModeFifoBuffer);

	// Default data type
	paramNDDataType = NDUInt16;

	// Camera booted
	paramReboot = 0;

	// Lets have a look at the status of the camera
	unsigned short recordingState;
	this->api->getRecordingState(this->camera, &recordingState);

	// refresh everything
	this->pollCameraNoAcquisition();
	this->pollCamera();

	// Inform server if we have one
	if(gangConnection != NULL)
	{
		gangConnection->sendMemberConfig(takeLock);
	}
}

/**
 * Initialise the pixel rate information.
 * The various members are used as follows:
 *   camDescription.pixelRate[] contains the available pixel rates in Hz, zeroes
 *                              for unused locations.
 *   pixRateEnumValues[] contains indices into the camDescription.pixelRate[] array
 *                       for the mbbx PV values.
 *   pixRateEnumStrings[] contains the mbbx strings
 *   pixRateEnumSeverities[] contains the severity codes for the mbbx PV.
 *   pixRate contains the current setting in Hz.
 *   pixRateValue contains the mbbx value of the current setting
 *   pixRateMax contains the maximum available setting in Hz.
 *   pixRateMaxValue contains the mbbx value of the maximum setting.
 *   pixRateNumEnums is the number of valid rates
 */
void Pco::initialisePixelRate()
{
    // Get the current rate
    unsigned long r;
    this->api->getPixelRate(this->camera, &r);
    this->pixRate = (int)r;
    this->pixRateValue = 0;
    // Work out the information
    this->pixRateMax = 0;
    this->pixRateMaxValue = 0;
    this->pixRateNumEnums = 0;
    for(int i = 0; i<DllApi::descriptionNumPixelRates; i++)
    {
        if(this->camDescription.pixelRate[i] > 0)
        {
            epicsSnprintf(this->pixRateEnumStrings[this->pixRateNumEnums], MAX_ENUM_STRING_SIZE,
                "%ld Hz", this->camDescription.pixelRate[i]);
            this->pixRateEnumValues[this->pixRateNumEnums] = i;
            this->pixRateEnumSeverities[this->pixRateNumEnums] = 0;
            if((int)this->camDescription.pixelRate[i] > this->pixRateMax)
            {
                this->pixRateMax = this->camDescription.pixelRate[i];
                this->pixRateMaxValue = this->pixRateNumEnums;
            }
            this->pixRateNumEnums++;
            if((int)this->camDescription.pixelRate[i] == this->pixRate)
            {
            	this->pixRateValue = i;
            }

        }
    }
    // Give the enum strings to the PV
    this->doCallbacksEnum(this->pixRateEnumStrings, this->pixRateEnumValues, this->pixRateEnumSeverities,
        this->pixRateNumEnums, paramPixRate.getHandle(), 0);
    paramPixRate = this->pixRateValue;
}

/**
 * Populate a binning validity set
 */
void Pco::setValidBinning(std::set<int>& valid, int max, int step) throw()
{
    valid.clear();
    int bin = 1;
    while(bin <= max)
    {
        valid.insert(bin);
        if(step == DllApi::binSteppingLinear)
        {
            bin += 1;
        }
        else
        {
            bin *= 2;
        }
    }
}

/**
 * Poll the camera for status information.  This function may only be called
 * while the camera is not acquiring.
 */
bool Pco::pollCameraNoAcquisition()
{
    bool result = true;
    try
    {
		// Nothing
    }
    catch(PcoException& e)
    {
        this->errorTrace << "Failure: " << e.what() << std::endl;
        result = false;
    }
    return result;
}

/**
 * Poll the camera for status information that can only be gathered during acquisition.
 */
bool Pco::pollCameraAcquisition()
{
    bool result = true;
    try
    {
		// Buffer state
		int flags = 0;
		int mask = 1;
		unsigned long statusDll;
		unsigned long statusDrv;
		for(int i=0; i<Pco::numApiBuffers; i++)
		{
			this->api->getBufferStatus(this->camera, this->buffers[i].bufferNumber, &statusDll, &statusDrv);
			if((statusDll & DllApi::statusDllEventSet) != 0)
			{
				flags |= mask << this->buffers[i].bufferNumber;
			}
		}
        // Update EPICS
		TakeLock takeLock(this);
		paramBuffersReady = (int)flags;
    }
    catch(PcoException& e)
    {
        this->errorTrace << "Failure: " << e.what() << std::endl;
        result = false;
    }
    return result;
}

/**
 * Poll the camera for status information that can be gathered at any time
 */
bool Pco::pollCamera()
{
    bool result = true;
    try
    {
        // Get the temperature information
        short ccdtemp, camtemp, powtemp;
        this->api->getTemperature(this->camera, &ccdtemp, &camtemp, &powtemp);
        // Get memory usage
        int ramUsePercent;
		int ramUseFrames;
		this->checkMemoryBuffer(ramUsePercent, ramUseFrames);
		// Camera state
		unsigned short camBusy;
		unsigned short expTrigger;
		unsigned short acqEnable;
		this->api->getCameraBusyStatus(this->camera, &camBusy);
		this->api->getExpTrigSignalStatus(this->camera, &expTrigger);
		this->api->getAcqEnblSignalStatus(this->camera, &acqEnable);
        // Update EPICS
		TakeLock takeLock(this);
        paramADTemperature = (double)ccdtemp/DllApi::ccdTemperatureScaleFactor;
        paramElectronicsTemp = (double)camtemp;
        paramPowerTemp = (double)powtemp;
		paramCameraBusy = (int)camBusy;
		paramExpTrigger = (int)expTrigger;
		paramAcqEnable = (int)acqEnable;
	    paramCamRamUse = ramUsePercent;
		paramCamRamUseFrames = ramUseFrames;
		paramBuffersInUse = this->pNDArrayPool->getNumBuffers() - this->pNDArrayPool->getNumFree();
    }
    catch(PcoException& e)
    {
        this->errorTrace << "Failure: " << e.what() << std::endl;
        result = false;
    }
    return result;
}

/**
 * Report the percentage of camera on board memory that contains images.
 * For cameras without on board memory this will always return 0%.
 * Note for a camera with a single image in memory the percentage returned will
 * be at least 1% even if the camera has a massive memory containing a small image.
 */
void Pco::checkMemoryBuffer(int& percentUsed, int& numFrames) throw(PcoException)
{
    percentUsed = 0;
	numFrames = 0;
	if(this->camStorage.ramSizePages > 0)
    {
        unsigned short segment;
        unsigned long validImages;
        unsigned long maxImages;
        try
        {
            this->api->getActiveRamSegment(this->camera, &segment);
            this->api->getNumberOfImagesInSegment(this->camera, segment, &validImages,
                    &maxImages);
			numFrames = validImages;
            if(maxImages > 0)
            {
                percentUsed = (validImages*100)/maxImages;
                if(validImages > 0 && percentUsed == 0)
                {
                    percentUsed = 1;
                }
            }
        }
        catch(PcoException&)
        {
        }
    }
}

/**
 * Handle a change to the ADAcquire parameter.
 */
void Pco::onAcquire(TakeLock& takeLock)
{
    if(paramADAcquire)
    {
        // Start an acquisition
        this->post(Pco::requestAcquire);
    	if(gangServer)
    	{
    		gangServer->start();
    	}
    }
    else
    {
        // Stop the acquisition
        this->post(Pco::requestStop);
    	if(gangServer)
    	{
    		gangServer->stop();
    	}
    }
}

/**
 * Handle a change to the ArmMode parameter
 */
void Pco::onArmMode(TakeLock& takeLock)
{
    // Perform an arm/disarm
    if(paramArmMode)
    {
        this->post(Pco::requestArm);
    	if(gangServer)
    	{
    		gangServer->arm();
    	}
    }
    else
    {
        this->post(Pco::requestDisarm);
    	if(gangServer)
    	{
    		gangServer->disarm();
    	}
    }
}

/**
 * Handle a change to the ClearStateRecord parameter
 */
void Pco::onClearStateRecord(TakeLock& takeLock)
{
    if(paramClearStateRecord)
    {
        paramStateRecord = "";
        paramClearStateRecord = 0;
    }
}

/**
 * Start a camera reboot
 */
void Pco::onReboot(TakeLock& takeLock)
{
	// Reboot only supported on the edge camera
	switch(this->camType.camType)
	{
	case DllApi::cameraTypeEdge:
	case DllApi::cameraTypeEdgeGl:
	case DllApi::cameraTypeEdgeCLHS:
		this->post(Pco::requestReboot);
		break;
	default:
		break;
	}
}

/**
 * Force get the top image (experimental fix for PCO4000 jamming issue)
 */
void Pco::onGetImage(TakeLock& takeLock)
{
	try
	{
		TakeLock(&this->apiLock);
		this->api->getImageEx(this->camera, /*segment=*/1, 0,
			0, /*bufferNumber=*/Pco::getImageBuffer, 
			this->xCamSize, this->yCamSize, this->camDescription.dynResolution);
		// Get an ND array
		NDArray* image = allocArray(this->xCamSize, this->yCamSize, NDUInt16);
		if(image != NULL)
		{
			// Copy the image into an NDArray
			::memcpy(image->pData, this->buffers[0].buffer,
					this->xCamSize*this->yCamSize*sizeof(unsigned short));
			// And pass it to the state machine
			this->receivedImageQueue.send(&image, sizeof(NDArray*));
			this->post(Pco::requestImageReceived);
		}
	}
	catch(PcoException&)
	{
	}
	paramGetImage = 0;
}

/**
 * Interpret an attempt to set the temperature as setting the cooling set point
 */
void Pco::onADTemperature(TakeLock& takeLock)
{
    paramCoolingSetpoint = (int)paramADTemperature;
    onCoolingSetpoint(takeLock);
}

/**
 * Post a request
 */
void Pco::post(const StateMachine::Event* req)
{
    this->stateMachine->post(req);
}

/**
 * Allocate an ND array
 */
NDArray* Pco::allocArray(int sizeX, int sizeY, NDDataType_t dataType)
{
    size_t maxDims[] = {sizeX, sizeY};
    NDArray* image = this->pNDArrayPool->alloc(sizeof(maxDims)/sizeof(size_t),
            maxDims, dataType, 0, NULL);
    if(image == NULL)
    {
        // Out of area detector NDArrays
    	TakeLock takeLock(this);
    	performanceMonitor->count(takeLock, PerformanceMonitor::PERF_OUTOFARRAYS);
    }
    return image;
}

/**
 * A frame has been received.  This is an indication that
 * a frame is ready in the specified buffer.  We must also
 * check all buffers from the last frame to this one for
 * valid frames otherwise things may go out of order.
 */
void Pco::frameReceived(int bufferNumber)
{
// JAT: This version needs a lock to avoid reentrant calls to the dll
//      We need to get the image out of the DLL buffer as soon as possible
//      and return the buffer to the DLL.  Otherwise, the edge at high
//      rates loses frames.
	// To avoid deadlocks, we mustn't take the parameter lock at the same time
	// as the api lock.  So we count the errors locally and then pass them
	// to the performance monitor at the end.
	int frameStatusError = 0;
	int captureError = 0;
	{
		// We need to grab the API for the whole of this part
		TakeLock takeLock(&this->apiLock);
		// Try receiving from the current head to the given buffer number
		int tryBuffer;
		bool going = true;
		do
		{
			tryBuffer = this->queueHead;
			// Get the buffer status
			unsigned long statusDll;
			unsigned long statusDrv;
			this->api->getBufferStatus(this->camera, tryBuffer, &statusDll, &statusDrv);
			if((statusDll & DllApi::statusDllEventSet) != 0)
			{
				// Buffer is good for further processing
				// No error or buffer cancelled.
				if(statusDrv == 0)
				{
					// Copy the image from the buffer into a frame
					NDArray* image = allocArray(this->xCamSize, this->yCamSize, NDUInt16);
					if(image != NULL)
					{
						// Copy the image into an NDArray
						::memcpy(image->pData, this->buffers[tryBuffer].buffer,
								this->xCamSize*this->yCamSize*sizeof(unsigned short));
						// And pass it to the state machine
						this->receivedImageQueue.send(&image, sizeof(NDArray*));
						this->post(Pco::requestImageReceived);
					}
				}
				else
				{
					// There was an error associated with the frame
					frameStatusError++;
				}
				// Give the buffer back to the driver
				this->api->addBufferEx(this->camera, /*firstImage=*/0,
					/*lastImage=*/0, tryBuffer,
					this->xCamSize, this->yCamSize, this->camDescription.dynResolution);
			}
			else
			{
				// Buffer has not been given to us
				captureError++;
				going = false;
			}
			if(going)
			{
				this->queueHead = (tryBuffer + 1) % this->fifoQueueSize;
			}
		}
		while(tryBuffer != bufferNumber && going);
	}
	// Now update the performance monitor
	if(frameStatusError > 0 || captureError > 0)
	{
		TakeLock takeLock(this);
		performanceMonitor->count(takeLock, PerformanceMonitor::PERF_FRAMESTATUSERROR, true, frameStatusError);
		performanceMonitor->count(takeLock, PerformanceMonitor::PERF_CAPTUREERROR, true, captureError);
	}
}

/**
 * The driver dll is signalling a fault while waiting for a frame
 */
void Pco::frameWaitFault()
{
	TakeLock takeLock(this);
	performanceMonitor->count(takeLock, PerformanceMonitor::PERF_WAITFAULT);
}

/**
 * Get frames from the PCO4000.  This version
 * uses the getImageEx function to receive
 * images from the camera rather than the usual
 * event driven system.
 */
void Pco::getFrames()
{
	try
	{
		// We need to grab the API for the whole of this function
		TakeLock takeLock(&this->apiLock);
		// Are there any frames in memory?
		unsigned long maxImages;
		unsigned long validImages;
		this->api->getNumberOfImagesInSegment(this->camera, /*segment=*/1, &validImages, &maxImages);
		while(validImages > 0 && !this->api->isStopped())
		{
			// Try to receive an image
			this->api->getImageEx(this->camera, /*segment=*/1, 0,
				0, /*bufferNumber=*/Pco::getImageBuffer, 
				this->xCamSize, this->yCamSize, this->camDescription.dynResolution);
			// Get an ND array
			NDArray* image = allocArray(this->xCamSize, this->yCamSize, NDUInt16);
			if(image != NULL)
			{
				// Copy the image into an NDArray
				::memcpy(image->pData, this->buffers[Pco::getImageBuffer].buffer,
						this->xCamSize*this->yCamSize*sizeof(unsigned short));
				// And pass it to the state machine
				this->receivedImageQueue.send(&image, sizeof(NDArray*));
			}
			this->post(Pco::requestImageReceived);
			// More frames?
			this->api->getNumberOfImagesInSegment(this->camera, /*segment=*/1, &validImages, &maxImages);
		}
	}
	catch(PcoException&)
	{
	}
}

/**
 * The driver is asking for a frame ready poll.
 */
void Pco::pollForFrames()
{
	// The PCO4000 sometimes jams up, here we extract stuck frames
	if(this->camType.camType == DllApi::cameraType4000 &&
		this->storageMode == DllApi::storageModeFifoBuffer)
	{
		// We need to grab the API for the whole of this function
		TakeLock takeLock(&this->apiLock);
		unsigned long validImages;
		bool anyReady = false;
		do
		{
			// Are there any frames in memory?
			unsigned long maxImages;
			this->api->getNumberOfImagesInSegment(this->camera, /*segment=*/1, &validImages, &maxImages);
			// See if any of the queue buffers admit to being ready
			unsigned long statusDll;
			unsigned long statusDrv;
			for(int i=0; i<this->fifoQueueSize; i++)
			{
				this->api->getBufferStatus(this->camera, this->buffers[i].bufferNumber, &statusDll, &statusDrv);
				if((statusDll & DllApi::statusDllEventSet) != 0)
				{
					anyReady = true;
				}
			}
			// If there's no buffers ready and frames in memory, try to recover
			if(validImages > 3 && !anyReady)
			{

				// This version does a get
				try
				{
					printf("#### Getting an image\n");
					this->api->getImageEx(this->camera, /*segment=*/1, 0,
						0, /*bufferNumber=*/Pco::getImageBuffer, 
						this->xCamSize, this->yCamSize, this->camDescription.dynResolution);
				}
				catch(PcoException&)
				{
					TakeLock takeLock(this);
					performanceMonitor->count(takeLock, PerformanceMonitor::PERF_DRIVERERROR);
				}
				// Get an ND array
				NDArray* image = allocArray(this->xCamSize, this->yCamSize, NDUInt16);
				if(image != NULL)
				{
					// Copy the image into an NDArray
					::memcpy(image->pData, this->buffers[0].buffer,
							this->xCamSize*this->yCamSize*sizeof(unsigned short));
					// And pass it to the state machine
					this->receivedImageQueue.send(&image, sizeof(NDArray*));
					this->post(Pco::requestImageReceived);
				}
				// Count frames read by the poll
				TakeLock takeLock(this);
				performanceMonitor->count(takeLock, PerformanceMonitor::PERF_POLLGETFRAME);
			}
		}
		while(validImages > 3 && !anyReady);
	}
}

/**
 * Return my asyn user object for use in tracing etc.
 */
asynUser* Pco::getAsynUser()
{
    return this->pasynUserSelf;
}

/**
 * Allocate image buffers and give them to the SDK.  We allocate actual memory here,
 * rather than using the NDArray memory because the SDK hangs onto the buffers, it only
 * shows them to us when there is a frame ready.  We must copy the frame out of the buffer
 * into an NDArray for use by the rest of the system.
 */
void Pco::allocateImageBuffers() throw(std::bad_alloc, PcoException)
{
    // Now allocate the memory and tell the SDK
	int bufferSize = this->xCamSize * this->yCamSize;
    try
    {
        for(int i=0; i<Pco::numApiBuffers; i++)
        {
            if(this->buffers[i].buffer != NULL)
            {
				_aligned_free(this->buffers[i].buffer);
            }
			this->buffers[i].buffer = (unsigned short*)_aligned_malloc(bufferSize*sizeof(unsigned short), 0x10000);
            this->buffers[i].bufferNumber = DllApi::bufferUnallocated;
            this->buffers[i].eventHandle = NULL;
            this->api->allocateBuffer(this->camera, &this->buffers[i].bufferNumber,
                    bufferSize * sizeof(short), &this->buffers[i].buffer,
                    &this->buffers[i].eventHandle);
            this->buffers[i].ready = true;
        }
    }
    catch(std::bad_alloc& e)
    {
        // Recover from memory allocation failure
		{
			TakeLock(&this->apiLock);
			this->api->stopFrameCapture();
		}
        this->freeImageBuffers();
        throw e;
    }
    catch(PcoException& e)
    {
        // Recover from PCO camera failure
		{
			TakeLock(&this->apiLock);
			this->api->stopFrameCapture();
		}
        this->freeImageBuffers();
        throw e;
    }
}

/**
 * Free the image buffers
 */
void Pco::freeImageBuffers() throw()
{
    // Free the buffers in the camera.  Since we are recovering,
    // ignore any SDK error this may cause.
    // JAT: We didn't originally free the buffers from the DLL routinely.
    //      However, for the Dimax it seems to be essential.
    for(int i=0; i<Pco::numApiBuffers; i++)
    {
		try
		{
			this->api->freeBuffer(this->camera, i);
		}
		catch(PcoException&)
		{
		}
    }
	// For some reason, when buffer allocation has failed, to clear the error
	// we need to put the camera into recording state and back out again.
	try
	{
		this->api->setRecordingState(this->camera, DllApi::recorderStateOn);
	}
	catch(PcoException&)
	{
	}
	try
	{
		this->api->setRecordingState(this->camera, DllApi::recorderStateOff);
	}
	catch(PcoException&)
	{
	}
	// Now cancel all images
	try
	{
	    this->api->cancelImages(this->camera);
	}
	catch(PcoException&)
	{
	}
}

/**
 * Depending on the camera, pixel rate and x image size we may have to adjust the transfer parameters
 * in order to achieve the frame rate required across camlink. 
 * The Edge in rolling shutter mode and > 50fps we have to select 12 bit transfer and a look up 
 * table to do the compression.
 * By experiment the following formats appear to work/not work with the Edge:
 *  Global shutter  : PCO_CL_DATAFORMAT_5x12 works
 *                  : PCO_CL_DATAFORMAT_5x16 PCO_CL_DATAFORMAT_5x12L doesn't work
 *  Rolling Shutter : PCO_CL_DATAFORMAT_5x12L PCO_CL_DATAFORMAT_5x12R PCO_CL_DATAFORMAT_5x16 PCO_CL_DATAFORMAT_5x12 works
 */
void Pco::adjustTransferParamsAndLut() throw(PcoException)
{
	unsigned short lutIdentifier = 0;
    // Configure according to camera type
	switch(this->camType.camType)
    {
    case DllApi::cameraTypeEdge:
    case DllApi::cameraTypeEdgeGl:
    case DllApi::cameraTypeEdgeCLHS:
        // Set the camlink transfer parameters, reading them back
        // again to make sure.
        if(this->cameraSetup == DllApi::edgeSetupGlobalShutter)
        {
            // Works in global and rolling modes
            this->camTransfer.dataFormat = DllApi::camlinkDataFormat5x12 |
                DllApi:: sccmosFormatTopCenterBottomCenter;
            lutIdentifier = DllApi::camlinkLutNone;
            this->dataFormat = dataFormat5x12;
        }
        else 
        {
        	// TODO Check: I believe this should be above 1920 (change applied), rather than above or equal to
            if(this->xCamSize>Pco::edgeXSizeNeedsReducedCamlink &&
                    this->pixRate>=Pco::edgePixRateNeedsReducedCamlink)
            {
                // Options for edge are PCO_CL_DATAFORMAT_5x12L (uses sqrt LUT) and 
                // PCO_CL_DATAFORMAT_5x12 (data shifted 2 LSBs lost)
                this->camTransfer.dataFormat = DllApi::camlinkDataFormat5x12L |
                    DllApi::sccmosFormatTopCenterBottomCenter;
                lutIdentifier = DllApi::camLinkLutSqrt;
                this->dataFormat = dataFormat5x12sqrtLut;
            } 
            else 
            {
                // Doesn't work in global, works in rolling
                this->camTransfer.dataFormat = DllApi::camlinkDataFormat5x16 |
                    DllApi::sccmosFormatTopCenterBottomCenter;
                lutIdentifier = DllApi::camlinkLutNone;
                this->dataFormat = dataFormat5x16;
            }
        }
        this->camTransfer.baudRate = Pco::edgeBaudRate;
        this->camTransfer.transmit = DllApi::transferTransmitEnable;
        if(this->camlinkLongGap)
        {
        	this->camTransfer.transmit |= DllApi::transferTransmitLongGap;
        }
        this->api->setTransferParameters(this->camera, &this->camTransfer);
        this->api->getTransferParameters(this->camera, &this->camTransfer);
        this->api->setActiveLookupTable(this->camera, lutIdentifier);
        break;
    default:
    	this->dataFormat = dataformatNotEdge;
        break;
    }
}

/**
 * Set the camera clock to match EPICS time.
 */
void Pco::setCameraClock() throw(PcoException)
{
    epicsTimeStamp epicsTime;
    epicsTimeGetCurrent(&epicsTime);
    unsigned long nanoSec;
    struct tm currentTime;
    epicsTimeToTM(&currentTime, &nanoSec, &epicsTime);
    this->api->setDateTime(this->camera, &currentTime);
    // Record the year for timestamp correction purposes
    this->cameraYear = currentTime.tm_year;
}

/**
 * Set the camera cooling set point.
 */
void Pco::onCoolingSetpoint(TakeLock& takeLock)
{
    if(paramCoolingSetpoint == 0 && paramMaxCoolingSetpoint == 0)
    {
        // Min and max = 0 means there is no cooling available for this camera.
    }
    else
    {
        try
        {
            this->api->setCoolingSetpoint(this->camera, (short)paramCoolingSetpoint);
        }
        catch(PcoException&)
        {
            // Not much we can do with this error
        }
        try
        {
            short actualCoolingSetpoint;
            this->api->getCoolingSetpoint(this->camera, &actualCoolingSetpoint);
            paramCoolingSetpoint = (int)actualCoolingSetpoint;
        }
        catch(PcoException&)
        {
            // Not much we can do with this error
        }
    }
}

/**
 * Pass buffer to SDK so it can populate it 
 */
void Pco::addAvailableBuffer(int index) throw(PcoException)
{
    if(this->buffers[index].ready)
    {
        this->api->addBufferEx(this->camera, /*firstImage=*/0,
            /*lastImage=*/0, this->buffers[index].bufferNumber,
            this->xCamSize, this->yCamSize, this->camDescription.dynResolution);
        this->buffers[index].ready = false;
    }
}

/**
 * Pass all buffers to the SDK so it can populate them 
 */
void Pco::addAvailableBufferAll() throw(PcoException)
{
	this->queueHead = 0;
    for(int i=0; i<Pco::numQueuedBuffers; i++)
    {
        addAvailableBuffer(i);
    }
	this->fifoQueueSize = Pco::numQueuedBuffers;
}

/**
 * Arm the camera, ie. prepare the camera for acquisition.
 * Throws exceptions on failure.
 */
void Pco::doArm() throw(std::bad_alloc, PcoException)
{
	TakeLock takeLock(this);
	performanceMonitor->count(takeLock, PerformanceMonitor::PERF_ARM, /*fault=*/false);
	// Camera now busy
	paramADStatus = ADStatusReadout;
	try
	{
		this->api->setRecordingState(this->camera, DllApi::recorderStateOff);
	}
	catch(PcoException&)
	{
	}
	try
	{
		this->api->resetSettingsToDefault(this->camera);
	}
	catch(PcoException&)
	{
	}
	// Get configuration information
	this->triggerMode = paramADTriggerMode;
	this->numImages = paramADNumImages;
    this->numExposures = paramADNumExposures;
	this->imageMode = paramADImageMode;
	this->timestampMode = paramTimestampMode;
	this->xMaxSize = paramADMaxSizeX;
	this->yMaxSize = paramADMaxSizeY;
	this->reqRoiStartX = paramADMinX;
	this->reqRoiStartY = paramADMinY;
	this->reqRoiSizeX = paramADSizeX;
	this->reqRoiSizeY = paramADSizeY;
	this->reqBinX = paramADBinX;
	this->reqBinY = paramADBinY;
    this->reqRoiPercentX = paramRoiPercentX;
    this->reqRoiPercentY = paramRoiPercentY;
	this->adcMode = paramAdcMode;
	this->bitAlignmentMode = paramBitAlignment;
	this->acquireMode = paramAcquireMode;
	this->pixRateValue = paramPixRate;
	this->pixRate = this->camDescription.pixelRate[this->pixRateEnumValues[this->pixRateValue]];
	this->exposureTime = paramADAcquireTime;
	this->acquisitionPeriod = paramADAcquirePeriod;
	this->delayTime = paramDelayTime;
	this->cameraSetup = paramCameraSetup;
	this->dataType = paramNDDataType;
	this->reverseX = paramADReverseX;
	this->reverseY = paramADReverseY;
	this->minExposureTime = paramExpTimeMin;
	this->maxExposureTime = paramExpTimeMax;
	this->minDelayTime = paramDelayTimeMin;
	this->maxDelayTime = paramDelayTimeMax;
	this->camlinkLongGap = paramCamlinkLongGap;
	this->recoderSubmode = paramRecorderSubmode;
	this->storageMode = paramStorageMode;

	// Clear error counters
	performanceMonitor->clear(takeLock);

	// Configure the camera (reading back the actual settings)
	this->api->setSensorFormat(this->camera, 0);
	if(this->camType.camType == DllApi::cameraType4000)
	{
		this->api->setDoubleImageMode(this->camera, 0);
	}
	this->cfgStorage();
	this->cfgMemoryMode();
	this->cfgBinningAndRoi();    // Also sets camera image size
	this->cfgTriggerMode();
	this->cfgTimestampMode();
	this->cfgAcquireMode();
	this->cfgAdcMode();
	this->cfgBitAlignmentMode();
	this->cfgPixelRate();
	this->cfgAcquisitionTimes();

	this->allocateImageBuffers();

	// Set the image parameters for the image buffer transfer inside the CamLink and GigE interface.
	// While using CamLink or GigE this function must be called, before the user tries to get images
	// from the camera and the sizes have changed. With all other interfaces this is a dummy call.
	this->api->camlinkSetImageParameters(this->camera, this->xCamSize, this->yCamSize);

	this->adjustTransferParamsAndLut();

	// Update what we have really set
	paramADMinX = this->reqRoiStartX;
	paramADMinY = this->reqRoiStartY;
	paramADSizeX = this->reqRoiSizeX;
	paramADSizeY = this->reqRoiSizeY;
	paramADBinX = this->reqBinX;
	paramADBinY = this->reqBinY;
	paramADTriggerMode = this->triggerMode;
	paramTimestampMode = this->timestampMode;
	paramAcquireMode = this->acquireMode;
	paramAdcMode = this->adcMode;
	paramBitAlignment = this->bitAlignmentMode;
	paramPixRate = this->pixRateValue;
	paramADAcquireTime = this->exposureTime;
	paramADAcquirePeriod = this->acquisitionPeriod;
	paramDelayTime = this->delayTime;
	paramHwBinX = this->hwBinX;
	paramHwBinY = this->hwBinY;
	paramHwRoiX1 = this->hwRoiX1;
	paramHwRoiY1 = this->hwRoiY1;
	paramHwRoiX2 = this->hwRoiX2;
	paramHwRoiY2 = this->hwRoiY2;
	paramXCamSize = this->xCamSize;
	paramYCamSize = this->yCamSize;
	paramRecorderSubmode = this->recoderSubmode;
	paramStorageMode = this->storageMode;
	paramDataFormat = this->dataFormat;
	// Inform server if we have one
	if(gangConnection != NULL)
	{
		gangConnection->sendMemberConfig(takeLock);
	}
	
    // Since ADC mode might have been changed above,
    // Update ROI symmetry requirement display
    onAdcMode(takeLock);

	// Should we use the special PCO4000 poll mode to avoid the wierdo FIFO
	// jamming up problem?
	this->useGetFrames = this->camType.camType == DllApi::cameraType4000 &&
		this->storageMode == DllApi::storageModeFifoBuffer &&
		this->acquisitionPeriod >= 0.1 &&
		(this->triggerMode == DllApi::triggerExternal || this->triggerMode == DllApi::triggerExternalOnly);
	// Let's try without.
	this->useGetFrames = false;

	// Now Arm the camera, so it is ready to take images, all settings should have been made by now
	this->api->arm(this->camera);

	// For non-edge cameras, switch on recording state before buffers given
	if(this->camType.camType != DllApi::cameraTypeEdge &&
			this->camType.camType != DllApi::cameraTypeEdgeGl &&
			this->camType.camType != DllApi::cameraTypeEdgeCLHS)
	{
		this->api->setRecordingState(this->camera, DllApi::recorderStateOn);
	}

	// Start the api capturing frames
	this->api->startFrameCapture(this->useGetFrames);

	// Give the buffers to the camera unless we are using get frame poll mode
	if(!this->useGetFrames)
	{
		this->addAvailableBufferAll();
	}
	this->lastImageNumber = 0;
	this->lastImageNumberValid = false;

	// For the edge, switch on recording after buffers given
	if(this->camType.camType == DllApi::cameraTypeEdge ||
			this->camType.camType == DllApi::cameraTypeEdgeGl ||
			this->camType.camType == DllApi::cameraTypeEdgeCLHS)
	{
		this->api->setRecordingState(this->camera, DllApi::recorderStateOn);
	}

}

/**
 * Configure the ADC mode
 */
void Pco::cfgAdcMode() throw(PcoException)
{
    unsigned short v;
	if(this->camType.camType == DllApi::cameraType1600 ||
		this->camType.camType == DllApi::cameraType2000 ||
		this->camType.camType == DllApi::cameraType4000)
    {
        this->api->setAdcOperation(this->camera, this->adcMode);
        this->api->getAdcOperation(this->camera, &v);
        this->adcMode = v;
    }
    else
    {
        this->adcMode = DllApi::adcModeSingle;
    }
}

/**
 * Configure the acquire mode
 */
void Pco::cfgMemoryMode() throw(PcoException)
{
    unsigned short v;
	this->api->setRecorderSubmode(this->camera, this->recoderSubmode);
	this->api->getRecorderSubmode(this->camera, &v);
	this->recoderSubmode = v;
	this->api->setStorageMode(this->camera, this->storageMode);
	this->api->getStorageMode(this->camera, &v);
	this->storageMode = v;
}

/**
 * Configure the storage (if any)
 */
void Pco::cfgStorage() throw(PcoException)
{
	// Does this camera have memory?
	if(this->camStorage.ramSizePages > 0)
	{
		// Yes, we want the memory all in the first segment
	    this->api->clearRamSegment(this->camera);
		this->api->setCameraRamSegmentSize(this->camera, 
			this->camStorage.ramSizePages, 0, 0, 0);
		this->api->setActiveRamSegment(this->camera, 1);
	}
}

/**
 * Configure the acquire mode
 */
void Pco::cfgAcquireMode() throw(PcoException)
{
    unsigned short v;
    this->api->setAcquireMode(this->camera, this->acquireMode);
    this->api->getAcquireMode(this->camera, &v);
    this->acquireMode = v;
}

/**
 * Configure the bit alignment mode
 */
void Pco::cfgBitAlignmentMode() throw(PcoException)
{
    unsigned short v;
    this->api->setBitAlignment(this->camera, this->bitAlignmentMode);
    this->api->getBitAlignment(this->camera, &v);
    this->bitAlignmentMode = v;
}

/**
 * Configure the timestamp mode
 */
void Pco::cfgTimestampMode() throw(PcoException)
{
    unsigned short v;
    if(this->camDescription.generalCaps & DllApi::generalCapsNoTimestamp)
    {
        // No timestamp available
        this->timestampMode = DllApi::timestampModeOff;
    }
    else if(this->camDescription.generalCaps & DllApi::generalCapsTimestampAsciiOnly)
    {
        // All timestamp modes are available
        this->api->setTimestampMode(this->camera, this->timestampMode);
        this->api->getTimestampMode(this->camera, &v);
        this->timestampMode = v;
    }
    else
    {
        // No ASCII only timestamps available
        if(this->timestampMode == DllApi::timestampModeAscii)
        {
            this->timestampMode = DllApi::timestampModeBinaryAndAscii;
        }
        this->api->setTimestampMode(this->camera, this->timestampMode);
        this->api->getTimestampMode(this->camera, &v);
        this->timestampMode = v;
    }
}

/**
 * Configure the trigger mode.
 * Handle the external only trigger mode by translating to the
 * regular external trigger mode.
 */
void Pco::cfgTriggerMode() throw(PcoException)
{
    unsigned short v;
    if(this->triggerMode == DllApi::triggerExternalOnly)
    {
        this->api->setTriggerMode(this->camera, DllApi::triggerExternal);
        this->api->getTriggerMode(this->camera, &v);
        if(v != DllApi::triggerExternal)
        {
            this->triggerMode = (int)v;
        }
    }
    else
    {
        this->api->setTriggerMode(this->camera, this->triggerMode);
        this->api->getTriggerMode(this->camera, &v);
        this->triggerMode = (int)v;
    }
}

/**
 * Configure the binning and region of interest.
 * @param updateParams If true, update values of parameters for ROI and binning here. Otherwise leave it up to the caller. Default false
 */
void Pco::cfgBinningAndRoi(bool updateParams) throw(PcoException)
{
    if (updateParams == true)
    {
        // If we're setting params at teh end,
        // infer that we need to get parameter information first
        this->xMaxSize = paramADMaxSizeX;
        this->yMaxSize = paramADMaxSizeY;
        this->reqRoiStartX = paramADMinX;
        this->reqRoiStartY = paramADMinY;
        this->reqRoiSizeX = paramADSizeX;
        this->reqRoiSizeY = paramADSizeY;
        this->reqBinX = paramADBinX;
        this->reqBinY = paramADBinY;
        this->reqRoiPercentX = paramRoiPercentX;
        this->reqRoiPercentY = paramRoiPercentY;
    }
    
    // Work out the software and hardware binning
    if(this->availBinX.find(this->reqBinX) == this->availBinX.end())
    {
        // Not a binning the camera can do
        this->hwBinX = Pco::defaultHorzBin;
#if DO_SOFTWARE_ROI
		// Software to do the job
		this->swBinX = this->reqBinX;
#else
		// Don't do ones hardware cannot manage
		this->swBinX = Pco::defaultHorzBin;
		this->reqBinX = Pco::defaultHorzBin;
#endif
    }
    else
    {
        // A binning the camera can do
        this->hwBinX = this->reqBinX;
        this->swBinX = Pco::defaultHorzBin;
    }
    if(this->availBinY.find(this->reqBinY) == this->availBinY.end())
    {
        // Not a binning the camera can do
        this->hwBinY = Pco::defaultVertBin;
#if DO_SOFTWARE_ROI
		// Software to do the job
        this->swBinY = this->reqBinY;
#else
		// Don't do ones hardware cannot manage
		this->swBinY = Pco::defaultVertBin;
		this->reqBinY = Pco::defaultVertBin;
#endif
    }
    else
    {
        // A binning the camera can do
        this->hwBinY = this->reqBinY;
        this->swBinY = Pco::defaultHorzBin;
    }
    this->api->setBinning(this->camera, this->hwBinX, this->hwBinY);
    
    // xCamSize and yCamSize determined from full sensor size / binning
    this->xCamSize = this->xMaxSize / this->hwBinX;
    this->yCamSize = this->yMaxSize / this->hwBinY;

    // this->errorTrace << "At beginning of cfgBinningAndRoi: xCamSize = " << this->xCamSize << ", yCamSize = " << this->yCamSize;

    // Friendly ROI setting from a percentage
    if (this->paramFriendlyRoiSetting == 1)
    {
    	// Reset this flag
    	this->paramFriendlyRoiSetting = 0;

    	// Make requested percentages valid
    	// so we definitely have something between 5 and 100 % in X and Y
    	this->reqRoiPercentX = std::max(this->reqRoiPercentX, 5);
    	this->reqRoiPercentX = std::min(this->reqRoiPercentX, 100);
    	this->reqRoiPercentY = std::max(this->reqRoiPercentY, 5);
    	this->reqRoiPercentY = std::min(this->reqRoiPercentY, 100);

    	// Deduce region size from percentage
    	this->reqRoiSizeX = (this->xCamSize * this->reqRoiPercentX) / 100.0;
    	this->reqRoiSizeY = (this->yCamSize * this->reqRoiPercentY) / 100.0;

    	// Deduce starting coordinates by making region symmetrical
    	this->reqRoiStartX = (this->xCamSize - this->reqRoiSizeX) / 2;
    	this->reqRoiStartY = (this->yCamSize - this->reqRoiSizeY) / 2;
    }

    // Make requested ROI valid
    this->reqRoiStartX = std::max(this->reqRoiStartX, 0);
    this->reqRoiStartX = std::min(this->reqRoiStartX, this->xCamSize-1);
    this->reqRoiStartY = std::max(this->reqRoiStartY, 0);
    this->reqRoiStartY = std::min(this->reqRoiStartY, this->yCamSize-1);
    this->reqRoiSizeX = std::max(this->reqRoiSizeX, 0);
    this->reqRoiSizeX = std::min(this->reqRoiSizeX, this->xCamSize-this->reqRoiStartX);
    this->reqRoiSizeY = std::max(this->reqRoiSizeY, 0);
    this->reqRoiSizeY = std::min(this->reqRoiSizeY, this->yCamSize-this->reqRoiStartY);

    // Get the desired hardware ROI (zero based, end not inclusive)
    this->hwRoiX1 = this->reqRoiStartX;
    this->hwRoiX2 = this->reqRoiStartX+this->reqRoiSizeX;
    this->hwRoiY1 = this->reqRoiStartY;
    this->hwRoiY2 = this->reqRoiStartY+this->reqRoiSizeY;

    // Enforce horizontal symmetry requirements
    if(this->roiSymmetryRequiredX())
    {
        if(this->hwRoiX1 <= this->xCamSize-this->hwRoiX2)
        {
            this->hwRoiX2 = this->xCamSize - this->hwRoiX1;
        }
        else
        {
            this->hwRoiX1 = this->xCamSize - this->hwRoiX2;
        }
    }

    // Enforce vertical symmetry requirements
	if(this->roiSymmetryRequiredY())
    {
        if(this->hwRoiY1 <= this->yCamSize-this->hwRoiY2)
        {
            this->hwRoiY2 = this->yCamSize - this->hwRoiY1;
        }
        else
        {
            this->hwRoiY1 = this->yCamSize - this->hwRoiY2;
        }
    }

    // Enforce stepping requirements
    this->hwRoiX1 = (this->hwRoiX1 / this->camDescription.roiHorSteps) *
            this->camDescription.roiHorSteps;
    this->hwRoiY1 = (this->hwRoiY1 / this->camDescription.roiVertSteps) *
            this->camDescription.roiVertSteps;
    this->hwRoiX2 = ((this->hwRoiX2+this->camDescription.roiHorSteps-1) /
            this->camDescription.roiHorSteps) * this->camDescription.roiHorSteps;
    this->hwRoiY2 = ((this->hwRoiY2+this->camDescription.roiVertSteps-1) /
            this->camDescription.roiVertSteps) * this->camDescription.roiVertSteps;

    // Work out the software ROI that cuts off the remaining bits in coordinates
    // relative to the hardware ROI
#if DO_SOFTWARE_ROI
	// Version that does SW ROI to complete the job
    this->swRoiStartX = this->reqRoiStartX - this->hwRoiX1;
    this->swRoiStartY = this->reqRoiStartY - this->hwRoiY1;
    this->swRoiSizeX = this->reqRoiSizeX;
    this->swRoiSizeY = this->reqRoiSizeY;
#else
	// Version that just accepts what the HW ROI can do
	this->swRoiStartX = 0;
	this->swRoiStartY = 0;
    this->swRoiSizeX = this->hwRoiX2 - this->hwRoiX1;
    this->swRoiSizeY = this->hwRoiY2 - this->hwRoiY1;
    // No image reversal either
    this->reverseX = 0;
    this->reverseY = 0;
    // And no pixel type translation
    this->dataType = NDUInt16;
#endif

    // Record the size of the frame coming from the camera
    this->xCamSize = this->hwRoiX2 - this->hwRoiX1;
    this->yCamSize = this->hwRoiY2 - this->hwRoiY1;

	// Record the ROI parameters achieved
	this->reqRoiStartX = this->swRoiStartX + this->hwRoiX1;
	this->reqRoiStartY = this->swRoiStartY + this->hwRoiY1;
	this->reqRoiSizeX = this->swRoiSizeX;
	this->reqRoiSizeY = this->swRoiSizeY;

    // Now change to 1 based coordinates and inclusive end, set the ROI
    // in the hardware
    this->hwRoiX1 += 1;
    this->hwRoiY1 += 1;
    
    // Set the hardware ROI and catch if an exceptions comes back
    try{
    	// temporary debug
    	/*this->errorTrace << "Set ROI to ( X1: " << (unsigned short)this->hwRoiX1 << ", Y1: " << (unsigned short)this->hwRoiY1 << ", X2: "
    					<< (unsigned short)this->hwRoiX2 << ", Y2: " << (unsigned short)this->hwRoiY2;*/
		this->api->setRoi(this->camera,
				(unsigned short)this->hwRoiX1, (unsigned short)this->hwRoiY1,
				(unsigned short)this->hwRoiX2, (unsigned short)this->hwRoiY2);
    }
    catch(PcoException& e)
    {
        this->errorTrace << "Failure: " << e.what() << std::endl;
        this->errorTrace << "Failed to set ROI to ( X1: " << (unsigned short)this->hwRoiX1 << ", Y1: " << (unsigned short)this->hwRoiY1 << ", X2: "
				<< (unsigned short)this->hwRoiX2 << ", Y2: " << (unsigned short)this->hwRoiY2;
    }
    // Set up the software ROI
    ::memset(this->arrayDims, 0, sizeof(NDDimension_t) * Pco::numDimensions);
    this->arrayDims[Pco::xDimension].offset = this->swRoiStartX;
    this->arrayDims[Pco::yDimension].offset = this->swRoiStartY;
    this->arrayDims[Pco::xDimension].size = this->swRoiSizeX;
    this->arrayDims[Pco::yDimension].size = this->swRoiSizeY;
    this->arrayDims[Pco::xDimension].binning = this->swBinX;
    this->arrayDims[Pco::yDimension].binning = this->swBinY;
    this->arrayDims[Pco::xDimension].reverse = this->reverseX;
    this->arrayDims[Pco::yDimension].reverse = this->reverseY;
    this->roiRequired =
            this->arrayDims[Pco::xDimension].offset != 0 ||
            this->arrayDims[Pco::yDimension].offset != 0 ||
            (int)this->arrayDims[Pco::xDimension].size != this->xCamSize ||
            (int)this->arrayDims[Pco::yDimension].size != this->yCamSize ||
            this->arrayDims[Pco::xDimension].binning != 1 ||
            this->arrayDims[Pco::yDimension].binning != 1 ||
            this->arrayDims[Pco::xDimension].reverse != 0 ||
            this->arrayDims[Pco::yDimension].reverse != 0 ||
            (NDDataType_t)this->dataType != NDUInt16;

	// Validate the burst mode
	if(paramStorageMode == DllApi::storageModeRecorder)
	{
		// Number of frames that can fit in the RAM
		int pixelsPerFrame = this->xCamSize * this->yCamSize; 
		int maxFrames = (int)((double)this->camStorage.ramSizePages * 
			(double)this->camStorage.pageSizePixels / (double)pixelsPerFrame);
		if(this->numImages*this->numExposures > maxFrames)
		{
			throw PcoException("Too many images for burst buffer", -1);
		}
	}

	// Should we update the driver parameters here? Configured by argument
	// otherwise the caller might want to do it themselves
	if (updateParams)
	{
		paramADMinX = this->reqRoiStartX;
		paramADMinY = this->reqRoiStartY;
		paramADSizeX = this->reqRoiSizeX;
		paramADSizeY = this->reqRoiSizeY;
		paramADBinX = this->reqBinX;
		paramADBinY = this->reqBinY;
		paramHwBinX = this->hwBinX;
		paramHwBinY = this->hwBinY;
		paramHwRoiX1 = this->hwRoiX1;
		paramHwRoiY1 = this->hwRoiY1;
		paramHwRoiX2 = this->hwRoiX2;
		paramHwRoiY2 = this->hwRoiY2;
		paramXCamSize = this->xCamSize;
		paramYCamSize = this->yCamSize;
	}
}

/**
 * Configure the pixel rate
 */
void Pco::cfgPixelRate() throw(PcoException)
{
    this->api->setPixelRate(this->camera, (unsigned long)this->pixRate);
    unsigned long v;
    this->api->getPixelRate(this->camera, &v);
    this->pixRate = (int)v;
}

/**
 * Write the acquisition times to the camera
 */
void Pco::cfgAcquisitionTimes() throw(PcoException)
{
    // Get the information
    // Work out the delay time to achieve the desired period.  Note that the
    // configured delay time is used unless it is zero, in which case the
    // acquisition period is used.
    double exposureTime = this->exposureTime;
    double delayTime = this->delayTime;
    if(delayTime == 0.0)
    {
        delayTime = std::max(this->acquisitionPeriod - this->exposureTime, 0.0);
    }
    // Check them against the camera's constraints;
    if(delayTime < this->minDelayTime)
    {
        delayTime = this->minDelayTime;
    }
    if(delayTime > this->maxDelayTime)
    {
        delayTime = this->maxDelayTime;
    }
    if(exposureTime < this->minExposureTime)
    {
        exposureTime = this->minExposureTime;
    }
    if(exposureTime > this->maxExposureTime)
    {
        exposureTime = this->maxExposureTime;
    }
    // Work out the best ranges to use to represent to the camera
    unsigned short exposureBase;
    unsigned long exposure;
    unsigned short delayBase;
    unsigned long delay;
    if(this->exposureTime < Pco::timebaseNanosecondsThreshold)
    {
        exposureBase = DllApi::timebaseNanoseconds;
    }
    else if(this->exposureTime < Pco::timebaseMicrosecondsThreshold)
    {
        exposureBase = DllApi::timebaseMicroseconds;
    }
    else
    {
        exposureBase = DllApi::timebaseMilliseconds;
    }
    if(delayTime < Pco::timebaseNanosecondsThreshold)
    {
        delayBase = DllApi::timebaseNanoseconds;
    }
    else if(delayTime < Pco::timebaseMicrosecondsThreshold)
    {
        delayBase = DllApi::timebaseMicroseconds;
    }
    else
    {
        delayBase = DllApi::timebaseMilliseconds;
    }
    // Set the camera
    delay = (unsigned long)(delayTime * DllApi::timebaseScaleFactor[delayBase]);
    exposure = (unsigned long)(exposureTime * DllApi::timebaseScaleFactor[exposureBase]);
    this->api->setDelayExposureTime(this->camera, delay, exposure,
            delayBase, exposureBase);
    // Read back what the camera is actually set to
    this->api->getDelayExposureTime(this->camera, &delay, &exposure,
            &delayBase, &exposureBase);
    this->exposureTime = (double)exposure / DllApi::timebaseScaleFactor[exposureBase];
    delayTime = (double)delay / DllApi::timebaseScaleFactor[delayBase];
    if(this->delayTime != 0.0)
    {
        this->delayTime = delayTime;
    }
    this->acquisitionPeriod = this->exposureTime + delayTime;
}

/**
 * Indicate to EPICS that acquisition has begun.
 */
void Pco::nowAcquiring() throw()
{
	TakeLock takeLock(this);
	performanceMonitor->count(takeLock, PerformanceMonitor::PERF_START, /*fault=*/false);
    // Get info
    this->arrayCounter = paramNDArrayCounter;
    this->numImages = paramADNumImages;
    this->numExposures = paramADNumExposures;
    if(this->imageMode == ADImageSingle)
    {
        this->numImages = 1;
    }
    // Clear counters
    this->numImagesCounter = 0;
    this->numExposuresCounter = 0;
    // Set info
    paramADStatus = ADStatusReadout;
    paramADAcquire = 1;
    paramNDArraySize = this->xCamSize*this->yCamSize*sizeof(unsigned short);
    paramNDArraySizeX = this->xCamSize;
    paramNDArraySizeY = this->yCamSize;
    paramADNumImagesCounter = this->numImagesCounter;
    paramADNumExposuresCounter = this->numExposuresCounter;
}

/**
 * An acquisition has completed
 */
void Pco::acquisitionComplete() throw()
{
	TakeLock takeLock(this);
    paramADStatus = ADStatusIdle;
    paramADAcquire = 0;
    this->triggerTimer->stop();
}

/**
 * Exit the armed state
 */
void Pco::doDisarm() throw()
{
	{
		TakeLock lock(&this->apiLock);
		this->api->stopFrameCapture();
		this->freeImageBuffers();
	}
	{
		TakeLock lock(this);
		paramArmMode = 0;
		paramArmComplete = 0;
	}
}

/**
 * Start the camera by sending a software trigger if we are in one
 * of the soft modes
 */
void Pco::startCamera() throw()
{
    // Start the camera if we are in one of the soft modes
    if(this->triggerMode == DllApi::triggerSoftware ||
        this->triggerMode == DllApi::triggerExternal)
    {
        unsigned short triggerState = 0;
		TakeLock takeLock(&this->apiLock);
        try
        {
            this->api->forceTrigger(this->camera, &triggerState);
        }
        catch(PcoException&)
        {
			TakeLock takeLock(this);
			performanceMonitor->count(takeLock, PerformanceMonitor::PERF_DRIVERERROR);
        }
        // Schedule a retry if it fails
        if(!triggerState)
        {
            // Trigger did not succeed, try again soon
            this->triggerTimer->start(Pco::triggerRetryPeriod, Pco::requestTrigger);
        }
    }
}

/**
 * Discard all images waiting in the queue.
 */
void Pco::discardImages() throw()
{
    while(this->receivedImageQueue.pending() > 0)
    {
        NDArray* image;
        this->receivedImageQueue.tryReceive(&image, sizeof(NDArray*));
		image->release();
    }
}

/**
 * Receive all available images from the camera.  This function is called in
 * response to an image ready event, but we read all images and cope if there are
 * none so that missing image ready events don't stall the system.  Receiving
 * stops when the queue is empty or the acquisition is complete.  Returns
 * true if the acquisition is complete or there are enough frames in memory in burst mode.
 */
bool Pco::receiveImages() throw()
{
	bool result = false;
    // Poll the buffer queue
    // Note that the API has aready reset the event so the event status bit
    // returned by getBufferStatus will already be clear.  However, for
    // buffers that do have data ready return a statusDrv of zero.
    while(this->receivedImageQueue.pending() > 0 &&
    		(this->imageMode == ADImageContinuous ||
            this->numImagesCounter < this->numImages))
    {
		// Get the image
		NDArray* image;
        this->receivedImageQueue.tryReceive(&image, sizeof(NDArray*));
		if(paramStorageMode == DllApi::storageModeRecorder)
		{
			// Burst mode...
			image->release();
			// Read the memory state
			int ramUsePercent;
			int ramUseFrames;
			this->checkMemoryBuffer(ramUsePercent, ramUseFrames);
			// Update EPICS
			TakeLock takeLock(this);
			paramCamRamUse = ramUsePercent;
			paramCamRamUseFrames = ramUseFrames;
			// Done?
			result = ramUseFrames >= this->numImages*this->numExposures;
			image->release();
		}
		else
		{
			// Not burst mode...
			validateAndProcessFrame(image);
			// Update statistics
			TakeLock takeLock(this);
			paramADNumExposuresCounter = this->numExposuresCounter;
			paramImageNumber = this->lastImageNumber;
			// Done?
			result = this->imageMode != ADImageContinuous &&
    				this->numImagesCounter >= this->numImages;
		}
	}
    return result;
}

/**
 * Validate and process a frame that has been received into an ND array
 */
void Pco::validateAndProcessFrame(NDArray* image)
{
	// If there is a binary timestamp in the frame, we can sort
	// out the invalid frames that some cameras output when 
	// arming.
	if(this->isImageValid((unsigned short*)image->pData))
	{
		// What is the number of the image?  If the image does not
		// contain the BCD image number
		// use the dead reckoning number instead.
		long imageNumber = this->lastImageNumber + 1;
		if(this->timestampMode == DllApi::timestampModeBinary ||
			this->timestampMode == DllApi::timestampModeBinaryAndAscii)
		{
			imageNumber = this->extractImageNumber(
					(unsigned short*)image->pData);
		}
		// If this is the image we are expecting?
		if(imageNumber != this->lastImageNumber+1)
		{
			printf("Missing frame, got=%ld, exp=%ld\n", imageNumber, this->lastImageNumber+1);
			// If we are missing just one frame, duplicate this one
			if(imageNumber == this->lastImageNumber+2)
			{
				printf("One frame missing, duplicating\n");
				image->reserve();
				processFrame(image);
			}
			TakeLock takeLock(this);
			performanceMonitor->count(takeLock, PerformanceMonitor::PERF_MISSINGFRAME);
		}
		this->lastImageNumber = imageNumber;
		// Further processing of the frame
		processFrame(image);
	}
	else
	{
		TakeLock takeLock(this);
		performanceMonitor->count(takeLock, PerformanceMonitor::PERF_INVALIDFRAME);
		image->release();
	}
}

/**
 * Process a frame that has been received into an ND array
 */
void Pco::processFrame(NDArray* image)
{
	// Do software ROI, binning and reversal if required
	if(this->roiRequired)
	{
		NDArray* scratch;
		this->pNDArrayPool->convert(image, &scratch,
				(NDDataType_t)this->dataType, this->arrayDims);
		image->release();
		image = scratch;
	}
	// Handle summing of multiple exposures
	bool nextImageReady = false;
	if(this->numExposures > 1)
	{
		this->numExposuresCounter++;
		if(this->numExposuresCounter > 1)
		{
			switch(image->dataType)
			{
			case NDUInt8:
			case NDInt8:
				sumArray<epicsUInt8>(image, this->imageSum);
				break;
			case NDUInt16:
			case NDInt16:
				sumArray<epicsUInt16>(image, this->imageSum);
				break;
			case NDUInt32:
			case NDInt32:
				sumArray<epicsUInt32>(image, this->imageSum);
				break;
			default:
				break;
			}
			// throw away the previous accumulator
			this->imageSum->release();
		}
		// keep the sum of previous images for the next iteration
		this->imageSum = image;
		if(this->numExposuresCounter >= this->numExposures)
		{
			// we have finished accumulating
			nextImageReady = true;
			this->numExposuresCounter = 0;
		}
	}
	else
	{
		nextImageReady = true;
	}
	if(nextImageReady)
	{
		// Attach the image information
		image->uniqueId = this->arrayCounter;
		epicsTimeStamp imageTime;
		if(this->timestampMode == DllApi::timestampModeBinary ||
				this->timestampMode == DllApi::timestampModeBinaryAndAscii)
		{
			this->extractImageTimeStamp(&imageTime,
					(unsigned short*)image->pData);
		}
		else
		{
			epicsTimeGetCurrent(&imageTime);
		}
		image->timeStamp = imageTime.secPastEpoch +
				imageTime.nsec / Pco::oneNanosecond;
		this->getAttributes(image->pAttributeList);
		// Show the image to the gang system
		if(this->gangConnection)
		{
            this->gangConnection->sendImage(image, this->numImagesCounter);
		}
		if(this->gangServer == NULL ||
                !gangServer->imageReceived(this->numImagesCounter, image))
		{
            // Gang system did not consume it, pass it on now
            imageComplete(image);
		}
	}
	TakeLock takeLock(this);
	performanceMonitor->count(takeLock, PerformanceMonitor::PERF_GOODFRAME, /*fault=*/false);
}

/**
 * An image has been completed, pass it on.
 */
void Pco::imageComplete(NDArray* image)
{
    // Update statistics
    this->arrayCounter++;
    this->numImagesCounter++;
    // Pass the array on
    this->doCallbacksGenericPointer(image, NDArrayData, 0);
    image->release();
    TakeLock takeLock(this);
    paramNDArrayCounter = arrayCounter;
    paramADNumImagesCounter = this->numImagesCounter;
}

/**
 * Handle the construction of images in the ganged mode.
 * Returns true if the acquisition is complete.
 */
bool Pco::makeImages()
{
	bool result = false;
	if(this->gangServer)
	{
		TakeLock takeLock(this);
		gangServer->makeCompleteImages(takeLock);
		result = this->imageMode != ADImageContinuous &&
				this->numImagesCounter >= this->numImages;
	}
	return result;
}

/**
 * Convert BCD coded number in image to int 
 */
long Pco::bcdToInt(unsigned short pixel) throw()
{
    int shiftLowBcd = 0;
    if(paramBitAlignment == DllApi::bitAlignmentMsb)
    {
        // In MSB mode, need to shift down
        shiftLowBcd = Pco::bitsPerShortWord - this->camDescription.dynResolution;
    }
    int shiftHighBcd = shiftLowBcd + Pco::bitsPerNybble;
    long p1 = (pixel>>shiftLowBcd)&(Pco::nybbleMask);
    long p2 = (pixel>>shiftHighBcd)&(Pco::nybbleMask);
    return p2*bcdDigitValue + p1;    
}

/**
 * Convert bcd number in first 4 pixels of image to extract image counter value
 */
long Pco::extractImageNumber(unsigned short* imagebuffer) throw()
{

    long imageNumber = 0;
    for(int i=0; i<Pco::bcdPixelLength; i++) 
    {
        imageNumber *= Pco::bcdDigitValue * Pco::bcdDigitValue;
        imageNumber += bcdToInt(imagebuffer[i]);
    };
    return imageNumber;
}

/**
 * Check the image is valid.  Some cameras output completely zero
 * frames on arming.  If there is an embedded timestamp we can detect
 * this as the timestamp is also completely zero.  
 */
bool Pco::isImageValid(unsigned short* imagebuffer) throw()
{
	bool result = true;
    if(this->timestampMode == DllApi::timestampModeBinary ||
        this->timestampMode == DllApi::timestampModeBinaryAndAscii)
    {
		result = false;
		for(int i=0; i<Pco::binaryHeaderLength && !result; i++)
		{
			result = imagebuffer[i] != 0;
		}
    }
	return result;
}

/**
 * Convert bcd numbers in pixels 5 to 14 of image to extract time stamp 
 */
void Pco::extractImageTimeStamp(epicsTimeStamp* imageTime,
        unsigned short* imageBuffer) throw()
{
    unsigned long nanoSec = 0;
    struct tm ct;
    ct.tm_year = bcdToInt(imageBuffer[4])*100 + bcdToInt(imageBuffer[5] - 1900);
    ct.tm_mon = bcdToInt(imageBuffer[6])-1;
    ct.tm_mday = bcdToInt(imageBuffer[7]);
    ct.tm_hour = bcdToInt(imageBuffer[8]);
    ct.tm_min = bcdToInt(imageBuffer[9]);
    ct.tm_sec = bcdToInt(imageBuffer[10]);
    nanoSec = (bcdToInt(imageBuffer[11])*10000 + bcdToInt(imageBuffer[12])*100 +
            bcdToInt(imageBuffer[13]))*1000;
   
#if 0
    // JAT: We'll comment this out for now and see what the PCO
    //      does return.
    // fix year if necessary
    // (pco4000 seems to always return 2036 for year)
    if(ct.tm_year >= 2036)
    {
        ct.tm_year = this->cameraYear;
    }
#endif
        
    epicsTimeFromTM (imageTime, &ct, nanoSec );
}

/**
 * This stop command is designed for use with a busy record
 */
void Pco::onConfirmedStop(TakeLock& takeLock)
{
	// Simulate the regular stop command
	paramADAcquire = 0;
	this->onAcquire(takeLock);
}

/**
 * Queue a request to apply the requested binning and ROI settings
 */
void Pco::onApplyBinningAndRoi(TakeLock& takeLock)
{
	this->post(Pco::requestApplyBinningAndRoi);
}

/**
 * Raise a flag that friendly ROI setting should be used
 * e.g. after setting a percentage ROI
 */
void Pco::onRequestPercentageRoi(TakeLock& takeLock)
{
	this->paramFriendlyRoiSetting = 1;
}

/**
 * We need to be notified when ADC mode is changed
 * to update the readback of ROI symmetry requirement
 */
void Pco::onAdcMode(TakeLock& takeLock)
{
	// Deduce camera ROI symmetry requirements
	this->adcMode = paramAdcMode;
	this->paramRoiSymmetryX = roiSymmetryRequiredX();
	this->paramRoiSymmetryY = roiSymmetryRequiredY();
}

/**
 * Register the gang server object
 */
void Pco::registerGangServer(GangServer* gangServer)
{
	this->gangServer = gangServer;
	lock();
	paramGangMode = gangModeServer;
	unlock();
}


/**
 * Register the gang client object
 */
void Pco::registerGangConnection(GangConnection* gangConnection)
{
	this->gangConnection = gangConnection;
	lock();
	paramGangMode = gangModeConnection;
	unlock();
}

/**
 * Helper function to sum 2 NDArrays
 */
template<typename T> void Pco::sumArray(NDArray* startingArray,
        NDArray* addArray) throw()
{
    T* inOutData = reinterpret_cast<T*>(startingArray->pData);
    T* addData = reinterpret_cast<T*>(addArray->pData);
    NDArrayInfo_t inInfo;

    startingArray->getInfo(&inInfo);
    for(int i=0; i<this->xCamSize*this->yCamSize; i++)
    {
        *inOutData += *addData;
        inOutData++;
        addData++;
    }
}

// IOC shell configuration command
extern "C" int pcoConfig(const char* portName, int maxBuffers, size_t maxMemory)
{
    Pco* existing = Pco::getPco(portName);
    if(existing == NULL)
    {
        new Pco(portName, maxBuffers, maxMemory);
    }
    else
    {
        printf("Error: port name \"%s\" already exists\n", portName);
    }
    return asynSuccess;
}
static const iocshArg pcoConfigArg0 = {"Port name", iocshArgString};
static const iocshArg pcoConfigArg1 = {"maxBuffers", iocshArgInt};
static const iocshArg pcoConfigArg2 = {"maxMemory", iocshArgInt};
static const iocshArg * const pcoConfigArgs[] = {&pcoConfigArg0, &pcoConfigArg1,
        &pcoConfigArg2};
static const iocshFuncDef configPco = {"pcoConfig", 3, pcoConfigArgs};
static void configPcoCallFunc(const iocshArgBuf *args)
{
    pcoConfig(args[0].sval, args[1].ival, args[2].ival);
}

/** Register the commands */
static void pcoRegister(void)
{
    iocshRegister(&configPco, configPcoCallFunc);
}
extern "C" { epicsExportRegistrar(pcoRegister); }

