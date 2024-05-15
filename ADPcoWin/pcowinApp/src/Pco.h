/* Pco.h
 *
 * Revamped PCO area detector driver.
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */
#ifndef PCO_H_
#define PCO_H_

#include "ADDriverEx.h"
#include <string>
#include <map>
#include <set>
#include "StateMachine.h"
#include "DllApi.h"
#include "TraceStream.h"
#include "NDArrayException.h"
#include "IntegerParam.h"
#include "DoubleParam.h"
#include "StringParam.h"
#include "epicsMutex.h"
class GangServer;
class GangConnection;
class PerformanceMonitor;
class TakeLock;

class Pco: public ADDriverEx
{
// Construction
public:
    Pco(const char* portName, int maxBuffers, size_t maxMemory, int numCameraDevices);
    virtual ~Pco();

// These are the methods that we override from ADDriverEx */
public:
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

// Parameters
public:
    IntegerParam paramPixRate;
    IntegerParam paramAdcMode;
    IntegerParam paramCamRamUse;
    DoubleParam paramElectronicsTemp;
    DoubleParam paramPowerTemp;
    IntegerParam paramStorageMode;
    IntegerParam paramRecorderSubmode;
    IntegerParam paramTimestampMode;
    IntegerParam paramAcquireMode;
    DoubleParam paramDelayTime;
    IntegerParam paramArmMode;
    IntegerParam paramImageNumber;
    IntegerParam paramCameraSetup;
    IntegerParam paramBitAlignment;
    StringParam paramStateRecord;
    IntegerParam paramClearStateRecord;
    IntegerParam paramHwBinX;
    IntegerParam paramHwBinY;
    IntegerParam paramHwRoiX1;
    IntegerParam paramHwRoiY1;
    IntegerParam paramHwRoiX2;
    IntegerParam paramHwRoiY2;
    IntegerParam paramXCamSize;
    IntegerParam paramYCamSize;
    IntegerParam paramCamlinkClock;
    IntegerParam paramMinCoolingSetpoint;
    IntegerParam paramMaxCoolingSetpoint;
    IntegerParam paramDefaultCoolingSetpoint;
    IntegerParam paramCoolingSetpoint;
    DoubleParam paramDelayTimeMin;
    DoubleParam paramDelayTimeMax;
    DoubleParam paramDelayTimeStep;
    DoubleParam paramExpTimeMin;
    DoubleParam paramExpTimeMax;
    DoubleParam paramExpTimeStep;
    IntegerParam paramMaxBinHorz;
    IntegerParam paramMaxBinVert;
    IntegerParam paramBinHorzStepping;
    IntegerParam paramBinVertStepping;
    IntegerParam paramRoiHorzSteps;
    IntegerParam paramRoiVertSteps;
    IntegerParam paramReboot;
    IntegerParam paramCamlinkLongGap;
    IntegerParam paramGangMode;
    IntegerParam paramADAcquire;
    DoubleParam paramADTemperature;
    IntegerParam paramCameraRam;
    IntegerParam paramCameraBusy;
    IntegerParam paramExpTrigger;
    IntegerParam paramAcqEnable;
    IntegerParam paramSerialNumber;
    IntegerParam paramHardwareVersion;
    IntegerParam paramFirmwareVersion;
    IntegerParam paramCamRamUseFrames;
    IntegerParam paramArmComplete;
    IntegerParam paramConnected;
    IntegerParam paramBuffersReady;
    IntegerParam paramIsEdge;
    IntegerParam paramGetImage;
    IntegerParam paramBuffersInUse;
    IntegerParam paramDataFormat;
    IntegerParam paramConfirmedStop;
    IntegerParam paramApplyBinningAndRoi;
    IntegerParam paramRoiPercentX;
    IntegerParam paramRoiPercentY;
    IntegerParam paramFriendlyRoiSetting;
    IntegerParam paramRoiSymmetryX;
    IntegerParam paramRoiSymmetryY;
    IntegerParam paramInterfaceType;
    IntegerParam paramInterfaceIsCameraLink;

    // Camera devices
    std::vector<int> pcoCameraDeviceName;
    std::vector<int> pcoCameraDeviceVersion;
    std::vector<int> pcoCameraDeviceVariant;

// Constants
public:
    static const int traceFlagsDllApi;
    static const int traceFlagsGang;
    static const int traceFlagsPerformance;
    static const int traceFlagsPcoState;
    static const int requestQueueCapacity;
    static const int numHandles;
    static const double rebootPeriod;
    static const double reconnectPeriod;
    static const double connectPeriod;
    static const double statusPollPeriod;
    static const double armIgnoreImagesPeriod;
    static const double acquisitionStatusPollPeriod;
    static const double initialisationPeriod;
    static const double acquireStartEventTimeout;
    static const char* stateNames[];
    static const char* eventNames[];
    static const int bitsPerShortWord;
    static const int bitsPerNybble;
    static const long nybbleMask;
    static const long bcdDigitValue;
    static const int bcdPixelLength;
    static const int binaryHeaderLength;
    static const int defaultHorzBin;
    static const int defaultVertBin;
    static const int defaultRoiMinX;
    static const int defaultRoiMinY;
    static const int defaultExposureTime;
    static const int defaultDelayTime;
    enum {numQueuedBuffers=2, numApiBuffers=8, getImageBuffer=7};
    static const int edgeXSizeNeedsReducedCamlink;
    static const int edgePixRateNeedsReducedCamlink;
    static const int edgeBaudRate;
    static const double timebaseNanosecondsThreshold;
    static const double timebaseMicrosecondsThreshold;
    static const double oneNanosecond;
    static const double oneMillisecond;
    static const double triggerRetryPeriod;
    static const int statusMessageSize;
    enum {xDimension=0, yDimension=1, numDimensions=2};
    enum {recordingStateRetry=10};
    enum {gangModeNone=0, gangModeServer=1, gangModeConnection=2};
    enum {ADImageBurst=3};
    enum {dataformatDefault=0,dataFormat5x12=1,dataFormat5x12sqrtLut=2,dataFormat5x16=3};

// API for use by component classes
public:
    void post(const StateMachine::Event* req);
    void frameReceived(int bufferNumber);
    void getFrames();
    void pollForFrames();
    void frameWaitFault();
    void trace(int flags, const char* format, ...);
    asynUser* getAsynUser();
    void registerDllApi(DllApi* api);
    void registerGangServer(GangServer* gangServer);
    void registerGangConnection(GangConnection* gangConnection);
    NDArray* allocArray(int sizeX, int sizeY, NDDataType_t dataType);
    void imageComplete(NDArray* image);
    void initialiseOnceRunning();

// Member variables
private:
    StateMachine* stateMachine;
    StateMachine::Timer* triggerTimer;
    DllApi* api;
    DllApi::Handle camera;
    DllApi::CameraType camType;
    DllApi::Description camDescription;
    DllApi::Storage camStorage;
    DllApi::Transfer camTransfer;
    DllApi::Sizes camSizes;
    int shiftLowBcd;         // Shift for decoding the BCD frame number in image
    int shiftHighBcd;        // Shift for decoding the BCD frame number in image
    TraceStream errorTrace;
public:
    TraceStream apiTrace;
    TraceStream gangTrace;
    TraceStream performanceTrace;
private:
    TraceStream stateTrace;
    struct
    {
        short bufferNumber;
        unsigned short* buffer;
        DllApi::Handle eventHandle;
        bool ready;
    } buffers[Pco::numApiBuffers];
    int queueHead;
    long lastImageNumber;
    bool lastImageNumberValid;
    epicsMessageQueue receivedImageQueue;
    int numImagesCounter;
    int numExposuresCounter;
    int numImages;
    int numExposures;
    int cameraYear;
    int arrayCounter;
    std::set<int> availBinX;
    std::set<int> availBinY;
    // Pixel rate information
    char* pixRateEnumStrings[DllApi::descriptionNumPixelRates];
    int pixRateEnumValues[DllApi::descriptionNumPixelRates];
    int pixRateEnumSeverities[DllApi::descriptionNumPixelRates];
    int pixRateNumEnums;
    int pixRateMaxRateEnum;
    int pixRate;
    int pixRateValue;
    int pixRateMax;
    int pixRateMaxValue;
    // Config information for an acquisition
    int xMaxSize;        // The sensor size...
    int yMaxSize;        // ...as returned by camera info
    int xCamSize;        // The size being generated by the camera...
    int yCamSize;        // ...which is max size / binning - ROI
    int triggerMode;
    int imageMode;
    int timestampMode;
    double exposureTime;
    double acquisitionPeriod;
    double delayTime;
    double minExposureTime;
    double maxExposureTime;
    double minDelayTime;
    double maxDelayTime;
    int camlinkLongGap;
    int reverseX;
    int reverseY;
    int adcMode;
    int bitAlignmentMode;
    int recoderSubmode;
    int storageMode;
    int acquireMode;
    int cameraSetup;
    int dataFormat;
    int dataType;
    int hwBinX;
    int hwBinY;
    int swBinX;
    int swBinY;
    int reqBinX;
    int reqBinY;
    int hwRoiX1;
    int hwRoiY1;
    int hwRoiX2;
    int hwRoiY2;
    int swRoiStartX;
    int swRoiStartY;
    int swRoiSizeX;
    int swRoiSizeY;
    int reqRoiStartX;
    int reqRoiStartY;
    int reqRoiSizeX;
    int reqRoiSizeY;
    int reqRoiPercentX;
    int reqRoiPercentY;
    //int friendlyRoiSetting;
    NDArray* imageSum;
    NDDimension_t arrayDims[numDimensions];
    bool roiRequired;
    GangServer* gangServer;
    GangConnection* gangConnection;
    PerformanceMonitor* performanceMonitor;
    epicsMutex apiLock;
    unsigned long memoryImageCounter;
    int fifoQueueSize;
    bool useGetFrames;
    epicsEventId acquireStartedEvent;

public:
    static std::map<std::string, Pco*> thePcos;
    static Pco* getPco(const char* portName);

// Utility functions
private:
    void initialiseCamera(TakeLock& takeLock);
    bool pollCameraNoAcquisition();
    bool pollCameraAcquisition();
    bool pollCamera();
    void doArm() throw(std::bad_alloc, PcoException);
    void doDisarm() throw();
    void nowAcquiring() throw();
    void startCamera() throw();
    void allocateImageBuffers() throw(std::bad_alloc, PcoException);
    void freeImageBuffers() throw();
    void adjustTransferParamsAndLut() throw(PcoException);
    void setCameraClock() throw(PcoException);
    void addAvailableBuffer(int index) throw(PcoException);
    void addAvailableBufferAll() throw(PcoException);
    bool receiveImages() throw();
    void discardImages() throw();
    long extractImageNumber(unsigned short* imagebuffer) throw();
    bool isImageValid(unsigned short* imagebuffer) throw();
    long bcdToInt(unsigned short pixel) throw();
    void extractImageTimeStamp(epicsTimeStamp* imageTime, unsigned short* imageBuffer) throw();
    void acquisitionComplete() throw();
    void checkMemoryBuffer(int& percentUsed, int& numFrames) throw(PcoException);
    void setValidBinning(std::set<int>& valid, int max, int step) throw();
    void cfgMemoryMode() throw(PcoException);
    void cfgBinningAndRoi(bool updateParams = false) throw(PcoException);
    void cfgTriggerMode() throw(PcoException);
    void cfgTimestampMode() throw(PcoException);
    void cfgAcquireMode() throw(PcoException);
    void cfgAdcMode() throw(PcoException);
    void cfgBitAlignmentMode() throw(PcoException);
    void cfgPixelRate() throw(PcoException);
    void cfgAcquisitionTimes() throw(PcoException);
    void cfgStorage() throw(PcoException);
    template<typename T> void sumArray(NDArray* startingArray,
            NDArray* addArray) throw();
    void initialisePixelRate();
    void outputStatusMessage(const char* text);
    void doReboot();
    bool makeImages();
    void onAcquire(TakeLock& takeLock);
    void onArmMode(TakeLock& takeLock);
    void onClearStateRecord(TakeLock& takeLock);
    void onCoolingSetpoint(TakeLock& takeLock);
    void onADTemperature(TakeLock& takeLock);
    void onReboot(TakeLock& takeLock);
    void onGetImage(TakeLock& takeLock);
    void onConfirmedStop(TakeLock& takeLock);
    void onApplyBinningAndRoi(TakeLock& takeLock);
    void onRequestPercentageRoi(TakeLock& takeLock);
    void onAdcMode(TakeLock& takeLock);
    void validateAndProcessFrame(NDArray* image);
    void processFrame(NDArray* image);
    void readFirstMemoryImage();
    bool readNextMemoryImage();
    bool roiSymmetryRequiredX();
    bool roiSymmetryRequiredY();
    void getDeviceFirmwareInfo();


public:
    // States
    const StateMachine::State* stateUninitialised;
    const StateMachine::State* stateUnconnected;
    const StateMachine::State* stateIdle;
    const StateMachine::State* stateArmed;
    const StateMachine::State* stateAcquiring;
    const StateMachine::State* stateUnarmedAcquiring;
    const StateMachine::State* stateExternalAcquiring;
    const StateMachine::State* stateUnarmedDraining;
    const StateMachine::State* stateExternalDraining;
    const StateMachine::State* stateDraining;
    // Events
    const StateMachine::Event* requestInitialise;
    const StateMachine::Event* requestTimerExpiry;
    const StateMachine::Event* requestAcquire;
    const StateMachine::Event* requestStop;
    const StateMachine::Event* requestArm;
    const StateMachine::Event* requestImageReceived;
    const StateMachine::Event* requestDisarm;
    const StateMachine::Event* requestTrigger;
    const StateMachine::Event* requestReboot;
    const StateMachine::Event* requestMakeImages;
    const StateMachine::Event* requestApplyBinningAndRoi;

public:
    StateMachine::StateSelector smInitialiseWait();
    StateMachine::StateSelector smConnectToCamera();
    StateMachine::StateSelector smPollWhileIdle();
    StateMachine::StateSelector smPollWhileAcquiring();
    StateMachine::StateSelector smPollWhileDraining();
    StateMachine::StateSelector smRequestArm();
    StateMachine::StateSelector smArmAndAcquire();
    StateMachine::StateSelector smAcquire();
    StateMachine::StateSelector smDiscardImages();
    StateMachine::StateSelector smRequestReboot();
    StateMachine::StateSelector smFirstImageWhileArmed();
    StateMachine::StateSelector smDisarmAndDiscard();
    StateMachine::StateSelector smAcquireImage();
    StateMachine::StateSelector smExternalAcquireImage();
    StateMachine::StateSelector smUnarmedAcquireImage();
    StateMachine::StateSelector smMakeGangedImage();
    StateMachine::StateSelector smUnarmedMakeGangedImage();
    StateMachine::StateSelector smTrigger();
    StateMachine::StateSelector smStopAcquisition();
    StateMachine::StateSelector smExternalStopAcquisition();
    StateMachine::StateSelector smUnarmedDrainImage();
    StateMachine::StateSelector smExternalDrainImage();
    StateMachine::StateSelector smDrainImage();
    StateMachine::StateSelector smAlreadyStopped();
    StateMachine::StateSelector smApplyBinningAndRoi();
};

#endif
