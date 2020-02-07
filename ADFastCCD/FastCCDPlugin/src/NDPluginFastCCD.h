#ifndef NDPluginFastCCD_H
#define NDPluginFastCCD_H

#include "NDPluginDriver.h"

#define FCCD_GAIN_1                             0xC000
#define FCCD_GAIN_1_M                           8
#define FCCD_GAIN_2                             0x4000
#define FCCD_GAIN_2_M                           4
#define FCCD_GAIN_8_M                           1
#define FCCD_MASK                               0x1FFF

#define FCCD_SCOL_N                             10

#define NDPluginFastCCDNameString               "NAME"
#define NDPluginFastCCDRowOffsetString          "ROW_OFFSET"
#define NDPluginFastCCDRowsString               "ROWS"
#define NDPluginFastCCDOverscanColsString       "OVERSCAN_COLS"
#define NDPluginFastCCDEnableGainString         "ENABLE_GAIN"
#define NDPluginFastCCDEnableSizeString         "ENABLE_SIZE"
#define NDPluginFastCCDAttrOverString           "ATTR_OVER"
#define NDPluginFastCCDCaptureBgndString        "CAPTURE_BGND"
#define NDPluginFastCCDEnableBgndString         "ENABLE_BGND"
#define NDPluginFastCCDValidBgndString          "VALID_BGND"
#define NDPluginFastCCDValidImageString         "VALID_IMAGE"
#define NDPluginFastCCDBgndSubtrString          "BGND_SUBTR"
#define NDPluginFastCCDTestString               "TEST"
#define NDPluginFastCCDGain8String              "GAIN_8"
#define NDPluginFastCCDGain2String              "GAIN_2"
#define NDPluginFastCCDGain1String              "GAIN_1"

class epicsShareClass NDPluginFastCCD : public NDPluginDriver {
public:
    NDPluginFastCCD(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize, int maxThreads);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

protected:
    /* ROI general parameters */
    int NDPluginFastCCDName;
    #define FIRST_NDPLUGIN_ROI_PARAM NDPluginFastCCDName

    /* ROI definition */
    int NDPluginFastCCDRowOffset;
    int NDPluginFastCCDRows;
    int NDPluginFastCCDOverscanCols;
    int NDPluginFastCCDEnableGain;
    int NDPluginFastCCDEnableSize;
    int NDPluginFastCCDEnableBgnd;
    int NDPliginFastCCDCaptureBgnd;
    int NDPluginFastCCDAttrOver;
    int NDPluginFastCCDCaptureBgnd;
    int NDPluginFastCCDValidBgnd;
    int NDPluginFastCCDValidImage;
    int NDPluginFastCCDBgndSubtr;
    int NDPluginFastCCDGain8;
    int NDPluginFastCCDGain2;
    int NDPluginFastCCDGain1;
    int NDPluginFastCCDTest;

private:

    asynStatus processImage(NDArray *pIn, NDArray *pOut, int rowOffset, int overscanCols, int correctGain,
                            int bgndSubtract);
    template <typename epicsType> asynStatus processImageT(NDArray *pIn, NDArray *pOut, 
                                                           int rowOffset, int overscanCols, int correctGain,
                                                           int bgndSubtract);
    template <typename epicsType> epicsFloat32 correctPixel(epicsType inp, epicsInt16 bgnd, int correctGain);
    NDArray *pBackground;
};
    
#endif
