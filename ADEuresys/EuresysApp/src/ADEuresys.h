#ifndef ADEURESYS_H
#define ADEURESYS_H

#include <epicsEvent.h>

#include <ADGenICam.h>
#include <EGrabber.h>

using namespace Euresys;

typedef EGrabber<CallbackSingleThread> EGRABBER_CALLBACK;

#define ESTimeStampModeString               "ES_TIME_STAMP_MODE"                // asynParamInt32, R/O
#define ESUniqueIdModeString                "ES_UNIQUE_ID_MODE"                 // asynParamInt32, R/O
#define ESBufferSizeString                  "ES_BUFFER_SIZE"                    // asynParamInt32, R/O
#define ESOutputQueueString                 "ES_OUTPUT_QUEUE"                   // asynParamInt32, R/O
#define ESRejectedFramesString              "ES_REJECTED_FRAMES"                // asynParamInt32, R/O
#define ESCRCErrorCountString               "ES_CRC_ERROR_COUNT"                // asynParamInt32, R/O
#define ESResetErrorCountsString            "ES_RESET_ERROR_COUNTS"             // asynParamInt32, R/O
#define ESProcessTotalTimeString            "ES_PROCESS_TOTAL_TIME"             // asynParamFloat64, R/O
#define ESProcessCopyTimeString             "ES_PROCESS_COPY_TIME"              // asynParamFloat64, R/O
#define ESConvertPixelFormatString          "ES_CONVERT_PIXEL_FORMAT"           // asynParamInt32, R/W
#define ESUnpackingModeString               "ES_UNPACKING_MODE"                 // asynParamInt32, R/W


/** Main driver class inherited from areaDetectors ADDriver class.
 * One instance of this class will control one camera.
 */
class ADEuresys : public ADGenICam
{
public:
    ADEuresys(const char *portName, const char* cameraId, int numESBuffers,
              size_t maxMemory, int priority, int stackSize);

    // virtual methods to override from ADGenICam
    void report(FILE *fp, int details);
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual GenICamFeature *createFeature(GenICamFeatureSet *set, 
                                          std::string const & asynName, asynParamType asynType, int asynIndex,
                                          std::string const & featureName, GCFeatureType_t featureType);
    
    void processFrame(ScopedBuffer &buf);
    EGRABBER_CALLBACK *getGrabber();
    void shutdown();

private:
    int ESTimeStampMode;
#define FIRST_ES_PARAM ESTimeStampMode
    int ESUniqueIdMode;
    int ESBufferSize;
    int ESOutputQueue;
    int ESRejectedFrames;
    int ESCRCErrorCount;
    int ESResetErrorCounts;
    int ESProcessTotalTime;
    int ESProcessCopyTime;
    int ESConvertPixelFormat;
    int ESUnpackingMode;

    /* Local methods to this class */
    asynStatus startCapture();
    asynStatus stopCapture();
    asynStatus connectCamera();
    asynStatus disconnectCamera();
    asynStatus readStatus();
    void resetErrorCounts();
    void reportNode(FILE *fp, const char *nodeName, int level);

    /* Data */
    EGRABBER_CALLBACK *mGrabber_;
    int numEGBuffers_;
    int bitsPerPixel_;
    int exiting_;
    int uniqueId_;
};

#endif

