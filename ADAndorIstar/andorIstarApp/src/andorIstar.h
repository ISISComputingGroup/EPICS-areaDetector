/**
 * Area Detector driver for the Andor iStar.
 *
 * @author Matthew Pearson
 * @date June 2009
 *
 * Updated Dec 2011 for Asyn 4-17 and areaDetector 1-7 
 *
 * Major updates to get callbacks working, etc. by Mark Rivers Feb. 2011
 */

#ifndef AndorIstar_H
#define AndorIstar_H

#include "tinyxml.h"
#include "ADDriver.h"
#include "SPEHeader.h"

#define MAX_ENUM_STRING_SIZE 26
#define MAX_ADC_SPEEDS 16
#define MAX_PREAMP_GAINS 16

#define AndorCoolerParamString             "ANDOR_COOLER"
#define AndorTempStatusMessageString       "ANDOR_TEMP_STAT"
#define AndorMessageString                 "ANDOR_MESSAGE"
#define AndorShutterModeString             "ANDOR_SHUTTER_MODE"
#define AndorShutterExTTLString            "ANDOR_SHUTTER_EXTTL"
#define AndorPalFileNameString             "ANDOR_PAL_FILE_PATH"
#define AndorAccumulatePeriodString        "ANDOR_ACCUMULATE_PERIOD"
#define AndorPreAmpGainString              "ANDOR_PREAMP_GAIN"
#define AndorAdcSpeedString                "ANDOR_ADC_SPEED"
// (Gabriele Salvato) MCP (Image Intensifier) and DDG (Digital Delay Generator)
#define AndorMCPGainString                 "ANDOR_MCP_GAIN"
#define AndorDDGGateDelayString            "ANDOR_DDG_GATE_DELAY"
#define AndorDDGGateWidthString            "ANDOR_DDG_GATE_WIDTH"
#define AndorDDGIOCString                  "ANDOR_DDG_IOC"
// (Gabriele Salvato) end

#define AT_GATEMODE_FIRE_AND_GATE 0
#define AT_GATEMODE_FIRE_ONLY     1
#define AT_GATEMODE_GATE_ONLY     2
#define AT_GATEMODE_CW_ON         3
#define AT_GATEMODE_CW_OFF        4
// (Gabriele Salvato) DDG
#define AT_GATEMODE_DDG           5
// (Gabriele Salvato) end

/**
 * Structure defining an ADC speed for the ADAndorIstar driver.
 *
 */
typedef struct {
  int ADCIndex;
  int AmpIndex;
  int HSSpeedIndex;
  float HSSpeed;
  int BitDepth;
  char *EnumString;
  int EnumValue;
} AndorADCSpeed_t;

/**
 * Structure defining a pre-amp gain for the ADAndorIstar driver.
 *
 */
typedef struct {
  float Gain;
  char *EnumString;
  int EnumValue;
} AndorPreAmpGain_t;

/**
 * Driver for Andor iStar cameras using version 2 of their SDK; inherits from ADDriver class in ADCore.
 *
 */
class AndorIstar : public ADDriver {
 public:
  AndorIstar(const char *portName, const char *installPath, int shamrockID, 
           int maxBuffers, size_t maxMemory, int priority, int stackSize);
  virtual ~AndorIstar();

  /* These are the methods that we override from ADDriver */
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
  virtual void report(FILE *fp, int details);
  virtual asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[], int severities[], 
                              size_t nElements, size_t *nIn);

  // Should be private, but are called from C so must be public
  void statusTask(void);
  void dataTask(void);

 protected:
  int AndorCoolerParam;
  #define FIRST_ANDOR_PARAM AndorCoolerParam
  int AndorTempStatusMessage;
  int AndorMessage;
  int AndorShutterMode;
  int AndorShutterExTTL;
  int AndorPalFileName;
  int AndorAccumulatePeriod;
  int AndorPreAmpGain;
  int AndorAdcSpeed;
  // (Gabriele Salvato) MCP (Image Intensifier) and DDG (Digital Delay Generator) 
  int AndorMCPGain;
  int AndorDDGGateDelay;
  int AndorDDGGateWidth;
  int AndorDDGIOC;
  // (Gabriele Salvato) Andor Driver file writing message
  #define LAST_ANDOR_PARAM AndorDDGIOC

 private:

  unsigned int checkStatus(unsigned int returnStatus);
  asynStatus setupAcquisition();
  asynStatus setupShutter(int command);
  void saveDataFrame(int frameNumber);
  void setupADCSpeeds();
  void setupPreAmpGains();
  unsigned int SaveAsSPE(char *fullFileName);
  // (Gabriele Salvato) To save in ISIS FITS Format 
  unsigned int SaveAsFitsFile(char *fullFileName, int FITSType);
  int WriteKeys(char *fullFileName, int* iStatus);
  // (Gabriele Salvato) end
  /**
   * Additional image mode to those in ADImageMode_t
   */
   static const epicsInt32 AImageFastKinetics;

  /**
   * List of acquisiton modes.
   */
  static const epicsUInt32 AASingle;
  static const epicsUInt32 AAAccumulate;
  static const epicsUInt32 AAKinetics;
  static const epicsUInt32 AAFastKinetics;
  static const epicsUInt32 AARunTillAbort;
  static const epicsUInt32 AATimeDelayedInt;

  /**
   * List of trigger modes.
   */
  static const epicsUInt32 ATInternal;
  static const epicsUInt32 ATExternal;
  static const epicsUInt32 ATExternalStart;
  static const epicsUInt32 ATExternalExposure;
  static const epicsUInt32 ATExternalFVB;
  static const epicsUInt32 ATSoftware;

  /**
   * List of detector status states
   */
  static const epicsUInt32 ASIdle;
  static const epicsUInt32 ASTempCycle;
  static const epicsUInt32 ASAcquiring;
  static const epicsUInt32 ASAccumTimeNotMet;
  static const epicsUInt32 ASKineticTimeNotMet;
  static const epicsUInt32 ASErrorAck;
  static const epicsUInt32 ASAcqBuffer;
  static const epicsUInt32 ASSpoolError;

  /**
   * List of detector readout modes.
   */
  static const epicsInt32 ARFullVerticalBinning;
  static const epicsInt32 ARMultiTrack;
  static const epicsInt32 ARRandomTrack;
  static const epicsInt32 ARSingleTrack;
  static const epicsInt32 ARImage;

  /**
   * List of shutter modes
   */
  static const epicsInt32 AShutterAuto;
  static const epicsInt32 AShutterOpen;
  static const epicsInt32 AShutterClose;

  // (Gabriele Salvato) List of Integrate On Chip modes
  static const epicsInt32 AndorIstar::AIOC_Off;
  static const epicsInt32 AndorIstar::AIOC_On;
  // (Gabriele Salvato) end
  
  /**
   * List of file formats
   */
  static const epicsInt32 AFFTIFF;
  static const epicsInt32 AFFBMP;
  static const epicsInt32 AFFSIF;
  static const epicsInt32 AFFEDF;
  static const epicsInt32 AFFRAW;
  static const epicsInt32 AFFFITS;
  static const epicsInt32 AFFSPE;

  epicsEventId statusEvent;
  epicsEventId dataEvent;
  double mPollingPeriod;
  double mFastPollingPeriod;
  unsigned int mAcquiringData;
  char *mInstallPath;
  bool mExiting;
  
  /**
   * ADC speed parameters
   */
  int mNumAmps;
  int mNumADCs;
  int mNumADCSpeeds;
  AndorADCSpeed_t mADCSpeeds[MAX_ADC_SPEEDS];
  int mTotalPreAmpGains;
  int mNumPreAmpGains;
  AndorPreAmpGain_t mPreAmpGains[MAX_PREAMP_GAINS];

  //Shutter control parameters
  float mAcquireTime;
  float mAcquirePeriod;
  float mAccumulatePeriod;
 
  // (Gabriele Salvato) will contain the camera capabilities
  AndorCapabilities capabilities;

  //(Gabriele Salvato) for iStar Support
  bool mIsCameraiStar;
  int mLowMCPGain;
  int mHighMCPGain;
  // (Gabriele Salvato) end
  
  // Shamrock spectrometer ID
  int mShamrockId;

  // SPE file header
  tagCSMAHEAD *mSPEHeader;
  TiXmlDocument *mSPEDoc;

};

#define NUM_ANDOR_DET_PARAMS ((int)(&LAST_ANDOR_PARAM - &FIRST_ANDOR_PARAM + 1))

#endif //AndorIstar_H

