/**
 * Area Detector driver for the LBNL FastCCD
 * Modifed by S. Wilkins
 *
 */

#include <stdio.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsString.h>
#include <iocsh.h>
#include <epicsExport.h>
#include <epicsExit.h>
#include <alarm.h>

#include "cin.h"
#include "FastCCD.h"

static const char *driverName = "FastCCD";

asynStatus FastCCD::connect(asynUser *pasynUser){
  return connectCamera();
}
  
asynStatus FastCCD::connectCamera(){

  if(cin_ctl_init(&cin_ctl, cinBaseIP, 0, 0, NULL, 0, 0))
  {
    return asynError;
  }
  if(cin_data_init(&cin_data, 
                   cinFabricIP, 0, NULL, 0, 0,
                   cinPacketBuffer, cinImageBuffer,  
                   allocateImageC, processImageC, this)) 
  {
    return asynError;
  }

  cin_ctl_set_msg_callback(&cin_ctl, &messageCallbackC, (void*)this);

  int _status = 0;

  _status |= cin_ctl_set_fabric_address(&cin_ctl, (char *)cinFabricIP);
  _status |= cin_data_send_magic(&cin_data);


  if(_status)
  {
    return asynError;
  }

  int timing_max = (CIN_CONFIG_MAX_TIMING_MODES > 10)  ? 10 : CIN_CONFIG_MAX_TIMING_MODES; 
  for(int i=0;i<timing_max;i++)
  {
    char *name;
    if(cin_config_get_timing_name(&cin_ctl, i, &name) == CIN_OK)
    {
      setStringParam(FastCCDTimingNameN[i], name);
    } else {
      setStringParam(FastCCDTimingNameN[i], "");
    }
  }

  return asynSuccess;
}

asynStatus FastCCD::disconnect(asynUser *pasynUser){
  return disconnectCamera();
}

asynStatus FastCCD::disconnectCamera(){
  cin_ctl_destroy(&cin_ctl);
  cin_data_destroy(&cin_data);
  return asynSuccess; 
}

static void messageCallbackC(const char *message, int severity, void *ptr)
{
  FastCCD *_ptr = (FastCCD*)ptr;
  _ptr->messageCallback(message, severity);
}

void FastCCD::messageCallback(const char *message, int severity)
{
  setStringParam(ADStatusMessage, message);
  if(severity == CIN_CTL_MSG_OK)
  {
    setParamAlarmSeverity(ADStatusMessage, NO_ALARM);
    setParamAlarmStatus(ADStatusMessage, NO_ALARM);
  } else if(severity == CIN_CTL_MSG_MINOR){
    setParamAlarmSeverity(ADStatusMessage, MINOR_ALARM);
    setParamAlarmStatus(ADStatusMessage, STATE_ALARM);
  } else if(severity == CIN_CTL_MSG_MAJOR){
    setParamAlarmSeverity(ADStatusMessage, MAJOR_ALARM);
    setParamAlarmStatus(ADStatusMessage, STATE_ALARM);
  }

  callParamCallbacks();
}

static void allocateImageC(cin_data_frame_t *frame, void* ptr){
  FastCCD *_ptr = (FastCCD*)ptr;
  _ptr->allocateImage(frame);
}

void FastCCD::allocateImage(cin_data_frame_t *frame)
{
  size_t dims[2];
  int nDims = 2;

  int dataType;
  getIntegerParam(NDDataType, &dataType);
  
  // Lets allocate a size from 
  // the descramble params
  int rows, ovscan, x, y;
  cin_data_get_descramble_params(&cin_data, &rows, &ovscan, &x, &y);
  
  if(x != 0)
  {
    dims[0] = x;
  } else {
    dims[0] = CIN_DATA_MAX_FRAME_X;
  }
  
  if(y != 0)
  {
    dims[1] = y;
  } else {
    dims[1] = CIN_DATA_MAX_FRAME_Y;
  }

  while(!(pImage = this->pNDArrayPool->alloc(nDims, dims, (NDDataType_t)dataType, 
                                             0, NULL))) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "Unable to allocate array from pool....\n");
    sleep(1);
  }
      
  frame->data = (uint16_t*)pImage->pData;

  return;
}

static void processImageC(cin_data_frame_t *frame, void *ptr)
{
  FastCCD *_ptr = (FastCCD*)ptr;
  _ptr->processImage(frame);
}

void FastCCD::processImage(cin_data_frame_t *frame)
{
  const char* functionName = "processImage";

  if(firstFrameFlag){
    this->lock();
    pImage->release();
    this->unlock();
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "Dropped frame %d as framesore frame %d\n",
              frame->number, firstFrameFlag);
    firstFrameFlag--;
    return;
  }

  int _status;
  getIntegerParam(ADStatus, &_status);
  if(_status != ADStatusAcquire)
  {
    // Lets dump the image as we are not acquiring
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "Dropped frame %d as we are not acquiring\n",
              frame->number);
    this->lock();
    pImage->release();
    this->unlock();
    return;
  }

  this->lock();

  if(!freeRun){
    this->framesRemaining--;
    if (this->framesRemaining <= 0) {
      cin_ctl_int_trigger_stop(&cin_ctl);
      cin_ctl_ext_trigger_stop(&cin_ctl);
      setIntegerParam(ADAcquire, 0);
      setIntegerParam(ADStatus, ADStatusIdle);
    }
    if(this->framesRemaining < 0){
      // Ok we have a problem. STOP!
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
                "Dropped frame %d due to \"framesRemaining\" overrun.\n",
                frame->number);
      pImage->release();
      this->unlock();
      return;
    }
  }

  /* Update the frame counter */
  int imageCounter;
  getIntegerParam(NDArrayCounter, &imageCounter);
  imageCounter++;
  setIntegerParam(NDArrayCounter, imageCounter);

  // Set the unique ID
  pImage->uniqueId = frame->number;
  pImage->dims[0].size = frame->size_x;
  pImage->dims[0].offset = 0;
  pImage->dims[0].binning = 1;
  pImage->dims[1].size = frame->size_y;
  pImage->dims[1].offset = 0;
  pImage->dims[1].binning = 1;

  // Process Timestamps.
  
  // Frame timestamp is a timespec always trust driver becuase it is correct!
  pImage->timeStamp = frame->timestamp.tv_sec + 1e-9 * frame->timestamp.tv_nsec;
  pImage->epicsTS.secPastEpoch = frame->timestamp.tv_sec;
  pImage->epicsTS.nsec = frame->timestamp.tv_nsec;
  updateTimeStamp(&pImage->epicsTS);
  lastFrameTimestamp = frame->timestamp;

  // Get any attributes for the driver
  this->getAttributes(pImage->pAttributeList);
       
  int arrayCallbacks;
  getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
  if (arrayCallbacks) {
    /* Call the NDArray callback */
    doCallbacksGenericPointer(pImage, NDArrayData, 0);
  }

 
  asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER,
              "%s:%s: frameId=%d\n",
              driverName, functionName, frame->number);

  // Release the frame as we are done with it!
  pImage->release();

  /* Update any changed parameters */
  callParamCallbacks();

  this->unlock();
  return;
}

/** Constructor for FastCCD driver; most parameters are simply passed to ADDriver::ADDriver.
  * After calling the base class constructor this method creates a thread to collect the detector data, 
  * and sets reasonable default values the parameters defined in this class, asynNDArrayDriver, and ADDriver.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] packetBuffer The CINDATA packet buffer size
  * \param[in] imageBuffer The CINDATA image buffer size
  * \param[in] baseIP The base IP address
  * \param[in] fabricIP The fabric IP address
  * \param[in] fabricMAC The fabric MAC address
  */
FastCCD::FastCCD(const char *portName, int maxBuffers, size_t maxMemory, 
                 int priority, int stackSize, int packetBuffer, int imageBuffer,
		             const char *baseIP, const char *fabricIP, const char *fabricMAC)

  : ADDriver(portName, 1, NUM_FastCCD_DET_PARAMS, maxBuffers, maxMemory, 
             asynUInt32DigitalMask, asynUInt32DigitalMask,
             ASYN_CANBLOCK, 1, priority, stackSize)
{

  int status = asynSuccess;
  int sizeX, sizeY;
  
  static const char *functionName = "FastCCD";

  cin_set_debug_print(0);
  cin_set_error_print(0);

  /* Write the packet and frame buffer sizes */
  cinPacketBuffer = packetBuffer;
  cinImageBuffer = imageBuffer;

  /* Store the network information */
  if(strcmp(baseIP, ""))
  {
    cinBaseIP = (char *)malloc(sizeof(char) * 128);
    if(!cinBaseIP)
    {
      status = asynError;
    }
    strncpy(cinBaseIP, baseIP, 128);
  } else {
    cinBaseIP = NULL;
  }
    
  if(strcmp(fabricIP, ""))
  {
    cinFabricIP = (char *)malloc(sizeof(char) * 128);
    if(!cinFabricIP)
    {
      status = asynError;
    }
    strncpy(cinFabricIP, fabricIP, 128);
  } else {
    cinFabricIP = NULL;
  }

  if(strcmp(fabricMAC, ""))
  {
    cinFabricMAC = (char *)malloc(sizeof(char) * 128);
    if(!cinFabricMAC)
    {
      status = asynError;
    }
    strncpy(cinFabricMAC, fabricMAC, 128);
  } else {
    cinFabricMAC = NULL;
  }

  //Define the polling periods for the status thread.
  statusPollingPeriod = 30; //seconds
  dataStatsPollingPeriod = 0.1; //seconds

  // Assume we are in continuous mode
  framesRemaining = 0;
  freeRun = 1;

  // Set the first frame flag
  firstFrameFlag = 0;

  // Set last frame timestamp to zero.
  struct timespec _zero = {0, 0};
  lastFrameTimestamp = _zero;
  
  /* Create an EPICS exit handler */
  epicsAtExit(exitHandler, this);

  createParam(FastCCDPollingPeriodString,       asynParamFloat64,  &FastCCDPollingPeriod);

  createParam(FastCCDFramestoreString,          asynParamInt32,    &FastCCDFramestore);

  createParam(FastCCDMux1String,                asynParamInt32,    &FastCCDMux1);
  createParam(FastCCDMux2String,                asynParamInt32,    &FastCCDMux2);

  createParam(FastCCDFirmwarePathString,        asynParamOctet,    &FastCCDFirmwarePath);
  createParam(FastCCDBiasPathString,            asynParamOctet,    &FastCCDBiasPath);
  createParam(FastCCDClockPathString,           asynParamOctet,    &FastCCDClockPath);
  createParam(FastCCDFCRICPathString,           asynParamOctet,    &FastCCDFCRICPath);

  createParam(FastCCDFirmwareUploadString,      asynParamInt32,    &FastCCDFirmwareUpload);
  createParam(FastCCDBiasUploadString,          asynParamInt32,    &FastCCDBiasUpload);
  createParam(FastCCDClockUploadString,         asynParamInt32,    &FastCCDClockUpload);
  createParam(FastCCDFCRICUploadString,         asynParamInt32,    &FastCCDFCRICUpload);

  createParam(FastCCDPowerString,               asynParamInt32,    &FastCCDPower);
  createParam(FastCCDFPPowerString,             asynParamInt32,    &FastCCDFPPower);
  createParam(FastCCDCameraPowerString,         asynParamInt32,    &FastCCDCameraPower);
  createParam(FastCCDCameraPowerModeString,     asynParamInt32,    &FastCCDCameraPowerMode);

  createParam(FastCCDBiasString,                asynParamInt32,    &FastCCDBias);
  createParam(FastCCDClocksString,              asynParamInt32,    &FastCCDClocks);

  createParam(FastCCDFPGAStatusString,          asynParamUInt32Digital, &FastCCDFPGAStatus);
  createParam(FastCCDDCMStatusString,           asynParamUInt32Digital, &FastCCDDCMStatus);

  createParam(FastCCDOverscanRowsString,        asynParamInt32,    &FastCCDOverscanRows);
  createParam(FastCCDOverscanColsString,        asynParamInt32,    &FastCCDOverscanCols);

  createParam(FastCCDFclkString,                asynParamInt32,    &FastCCDFclk);

  createParam(FastCCDFCRICGainString,           asynParamInt32,    &FastCCDFCRICGain);
  createParam(FastCCDFCRICClampString,          asynParamInt32,    &FastCCDFCRICClamp);

  createParam(FastCCDVBus12V0String,            asynParamFloat64,  &FastCCDVBus12V0);
  createParam(FastCCDVMgmt3v3String,            asynParamFloat64,  &FastCCDVMgmt3v3);
  createParam(FastCCDVMgmt2v5String,            asynParamFloat64,  &FastCCDVMgmt2v5);
  createParam(FastCCDVMgmt1v2String,            asynParamFloat64,  &FastCCDVMgmt1v2);
  createParam(FastCCDVEnet1v0String,            asynParamFloat64,  &FastCCDVEnet1v0);
  createParam(FastCCDVS3E3v3String,             asynParamFloat64,  &FastCCDVS3E3v3);
  createParam(FastCCDVGen3v3String,             asynParamFloat64,  &FastCCDVGen3v3);
  createParam(FastCCDVGen2v5String,             asynParamFloat64,  &FastCCDVGen2v5);
  createParam(FastCCDV60v9String,               asynParamFloat64,  &FastCCDV60v9);
  createParam(FastCCDV61v0String,               asynParamFloat64,  &FastCCDV61v0);
  createParam(FastCCDV62v5String,               asynParamFloat64,  &FastCCDV62v5);
  createParam(FastCCDVFpString,                 asynParamFloat64,  &FastCCDVFp);

  createParam(FastCCDIBus12V0String,            asynParamFloat64,  &FastCCDIBus12V0);
  createParam(FastCCDIMgmt3v3String,            asynParamFloat64,  &FastCCDIMgmt3v3);
  createParam(FastCCDIMgmt2v5String,            asynParamFloat64,  &FastCCDIMgmt2v5);
  createParam(FastCCDIMgmt1v2String,            asynParamFloat64,  &FastCCDIMgmt1v2);
  createParam(FastCCDIEnet1v0String,            asynParamFloat64,  &FastCCDIEnet1v0);
  createParam(FastCCDIS3E3v3String,             asynParamFloat64,  &FastCCDIS3E3v3);
  createParam(FastCCDIGen3v3String,             asynParamFloat64,  &FastCCDIGen3v3);
  createParam(FastCCDIGen2v5String,             asynParamFloat64,  &FastCCDIGen2v5);
  createParam(FastCCDI60v9String,               asynParamFloat64,  &FastCCDI60v9);
  createParam(FastCCDI61v0String,               asynParamFloat64,  &FastCCDI61v0);
  createParam(FastCCDI62v5String,               asynParamFloat64,  &FastCCDI62v5);
  createParam(FastCCDIFpString,                 asynParamFloat64,  &FastCCDIFp);

  createParam(FastCCDBaseBoardIDString,         asynParamInt32,    &FastCCDBaseBoardID);
  createParam(FastCCDBaseSerialNumString,       asynParamInt32,    &FastCCDBaseSerialNum);
  createParam(FastCCDBaseFPGAVersionString,     asynParamInt32,    &FastCCDBaseFPGAVersion);
  createParam(FastCCDFabBoardIDString,          asynParamInt32,    &FastCCDFabBoardID);
  createParam(FastCCDFabSerialNumString,        asynParamInt32,    &FastCCDFabSerialNum);
  createParam(FastCCDFabFPGAVersionString,      asynParamInt32,    &FastCCDFabFPGAVersion);

  createParam(FastCCDStatusHBString,            asynParamInt32,    &FastCCDStatusHB);

  createParam(FastCCDBadPckString,              asynParamInt32,    &FastCCDBadPck);
  createParam(FastCCDDroppedPckString,          asynParamInt32,    &FastCCDDroppedPck);
  createParam(FastCCDLastFrameString,           asynParamInt32,    &FastCCDLastFrame);
  createParam(FastCCDResetStatsString,          asynParamInt32,    &FastCCDResetStats);
  createParam(FastCCDPacketBufferString,        asynParamInt32,    &FastCCDPacketBuffer);
  createParam(FastCCDFrameBufferString,         asynParamInt32,    &FastCCDFrameBuffer);

  createParam(FastCCDBiasPosHString,            asynParamFloat64,  &FastCCDBiasPosH);
  createParam(FastCCDBiasNegHString,            asynParamFloat64,  &FastCCDBiasNegH);
  createParam(FastCCDBiasPosRGString,           asynParamFloat64,  &FastCCDBiasPosRG);
  createParam(FastCCDBiasNegRGString,           asynParamFloat64,  &FastCCDBiasNegRG);
  createParam(FastCCDBiasPosSWString,           asynParamFloat64,  &FastCCDBiasPosSW);
  createParam(FastCCDBiasNegSWString,           asynParamFloat64,  &FastCCDBiasNegSW);
  createParam(FastCCDBiasPosVString,            asynParamFloat64,  &FastCCDBiasPosV);
  createParam(FastCCDBiasNegVString,            asynParamFloat64,  &FastCCDBiasNegV);
  createParam(FastCCDBiasPosTGString,           asynParamFloat64,  &FastCCDBiasPosTG);
  createParam(FastCCDBiasNegTGString,           asynParamFloat64,  &FastCCDBiasNegTG);
  createParam(FastCCDBiasPosVFString,           asynParamFloat64,  &FastCCDBiasPosVF);
  createParam(FastCCDBiasNegVFString,           asynParamFloat64,  &FastCCDBiasNegVF);
  createParam(FastCCDBiasNEDGEString,           asynParamFloat64,  &FastCCDBiasNEDGE);
  createParam(FastCCDBiasOTGString,             asynParamFloat64,  &FastCCDBiasOTG);
  createParam(FastCCDBiasVDDRString,            asynParamFloat64,  &FastCCDBiasVDDR);
  createParam(FastCCDBiasVDDOutString,          asynParamFloat64,  &FastCCDBiasVDDOut);
  createParam(FastCCDBiasBufBaseString,         asynParamFloat64,  &FastCCDBiasBufBase);
  createParam(FastCCDBiasBufDeltaString,        asynParamFloat64,  &FastCCDBiasBufDelta);
  createParam(FastCCDBiasSpare1String,          asynParamFloat64,  &FastCCDBiasSpare1);
  createParam(FastCCDBiasSpare2String,          asynParamFloat64,  &FastCCDBiasSpare2);
  
  createParam(FastCCDBiasPosHWString,           asynParamFloat64,  &FastCCDBiasPosHW);
  createParam(FastCCDBiasNegHWString,           asynParamFloat64,  &FastCCDBiasNegHW);
  createParam(FastCCDBiasPosRGWString,          asynParamFloat64,  &FastCCDBiasPosRGW);
  createParam(FastCCDBiasNegRGWString,          asynParamFloat64,  &FastCCDBiasNegRGW);
  createParam(FastCCDBiasPosSWWString,          asynParamFloat64,  &FastCCDBiasPosSWW);
  createParam(FastCCDBiasNegSWWString,          asynParamFloat64,  &FastCCDBiasNegSWW);
  createParam(FastCCDBiasPosVWString,           asynParamFloat64,  &FastCCDBiasPosVW);
  createParam(FastCCDBiasNegVWString,           asynParamFloat64,  &FastCCDBiasNegVW);
  createParam(FastCCDBiasPosTGWString,          asynParamFloat64,  &FastCCDBiasPosTGW);
  createParam(FastCCDBiasNegTGWString,          asynParamFloat64,  &FastCCDBiasNegTGW);
  createParam(FastCCDBiasPosVFWString,          asynParamFloat64,  &FastCCDBiasPosVFW);
  createParam(FastCCDBiasNegVFWString,          asynParamFloat64,  &FastCCDBiasNegVFW);
  createParam(FastCCDBiasNEDGEWString,          asynParamFloat64,  &FastCCDBiasNEDGEW);
  createParam(FastCCDBiasOTGWString,            asynParamFloat64,  &FastCCDBiasOTGW);
  createParam(FastCCDBiasVDDRWString,           asynParamFloat64,  &FastCCDBiasVDDRW);
  createParam(FastCCDBiasVDDOutWString,         asynParamFloat64,  &FastCCDBiasVDDOutW);
  createParam(FastCCDBiasBufBaseWString,        asynParamFloat64,  &FastCCDBiasBufBaseW);
  createParam(FastCCDBiasBufDeltaWString,       asynParamFloat64,  &FastCCDBiasBufDeltaW);
  createParam(FastCCDBiasSpare1WString,         asynParamFloat64,  &FastCCDBiasSpare1W);
  createParam(FastCCDBiasSpare2WString,         asynParamFloat64,  &FastCCDBiasSpare2W);

  createParam(FastCCDBiasWriteVString,          asynParamInt32,    &FastCCDBiasWriteV);

  createParam(FastCCDFOTestString,              asynParamInt32,    &FastCCDFOTest);

  createParam(FastCCDBootString,                asynParamInt32,    &FastCCDBoot);
  createParam(FastCCDSendBiasString,            asynParamInt32,    &FastCCDSendBias);
  createParam(FastCCDSendFCRICString,           asynParamInt32,    &FastCCDSendFCRIC);
  createParam(FastCCDSendTimingString,          asynParamInt32,    &FastCCDSendTiming);

  createParam(FastCCDTimingModeString,          asynParamInt32,    &FastCCDTimingMode);
  createParam(FastCCDTimingNameString,          asynParamOctet,    &FastCCDTimingName);
  createParam(FastCCDTimingName0String,         asynParamOctet,    &FastCCDTimingNameN[0]);
  createParam(FastCCDTimingName1String,         asynParamOctet,    &FastCCDTimingNameN[1]);
  createParam(FastCCDTimingName2String,         asynParamOctet,    &FastCCDTimingNameN[2]);
  createParam(FastCCDTimingName3String,         asynParamOctet,    &FastCCDTimingNameN[3]);
  createParam(FastCCDTimingName4String,         asynParamOctet,    &FastCCDTimingNameN[4]);
  createParam(FastCCDTimingName5String,         asynParamOctet,    &FastCCDTimingNameN[5]);
  createParam(FastCCDTimingName6String,         asynParamOctet,    &FastCCDTimingNameN[6]);
  createParam(FastCCDTimingName7String,         asynParamOctet,    &FastCCDTimingNameN[7]);
  createParam(FastCCDTimingName8String,         asynParamOctet,    &FastCCDTimingNameN[8]);
  createParam(FastCCDTimingName9String,         asynParamOctet,    &FastCCDTimingNameN[9]);

  // Create the epicsEvent for signaling to the status task when parameters should have changed.
  // This will cause it to do a poll immediately, rather than wait for the poll time period.
  
  this->statusEvent = epicsEventMustCreate(epicsEventEmpty);
  if (!this->statusEvent) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s: Failed to create event for status task.\n",
              driverName, functionName);
    return;
  }

  this->dataStatsEvent = epicsEventMustCreate(epicsEventEmpty);
  if (!this->dataStatsEvent) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s: Failed to create event for data stats task.\n",
              driverName, functionName);
    return;
  }


  this->detectorWaitEvent = epicsEventMustCreate(epicsEventEmpty);
  if (!this->detectorWaitEvent) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s:%s: Failed to create event for detector wait task.\n",
              driverName, functionName);
    return;
  }

  sizeX = CIN_DATA_MAX_FRAME_X;
  sizeY = CIN_DATA_MAX_FRAME_Y;

  /* Set some default values for parameters */
  status =  asynSuccess;
  status |= setDoubleParam(FastCCDPollingPeriod, statusPollingPeriod);

  status |= setStringParam(ADManufacturer, "Berkeley Laboratory");
  status |= setStringParam(ADModel, "1k x 2k FastCCD");
  status |= setIntegerParam(ADSizeX, sizeX);
  status |= setIntegerParam(ADSizeY, sizeY);
  status |= setIntegerParam(ADBinX, 1);
  status |= setIntegerParam(ADBinY, 1);
  status |= setIntegerParam(ADMinX, 0);
  status |= setIntegerParam(ADMinY, 0);
  status |= setIntegerParam(ADMaxSizeX, sizeX);
  status |= setIntegerParam(ADMaxSizeY, sizeY);  
  status |= setIntegerParam(ADReverseX, 0);  
  status |= setIntegerParam(ADReverseY, 0);  
  status |= setIntegerParam(ADImageMode, ADImageSingle);
  status |= setIntegerParam(ADTriggerMode, 1);
  status |= setDoubleParam(ADAcquireTime, 0.005);
  status |= setDoubleParam(ADAcquirePeriod, 1.0);
  status |= setIntegerParam(ADNumImages, 1);
  status |= setIntegerParam(ADNumExposures, 1);
  status |= setIntegerParam(NDArraySizeX, sizeX);
  status |= setIntegerParam(NDArraySizeY, sizeY);
  status |= setIntegerParam(NDDataType, NDInt16);
  status |= setIntegerParam(NDArraySize, sizeX*sizeY*sizeof(epicsInt16)); 
  status |= setDoubleParam(ADShutterOpenDelay, 0.);
  status |= setDoubleParam(ADShutterCloseDelay, 0.);

  status |= setIntegerParam(FastCCDFirmwareUpload, 0);
  status |= setIntegerParam(FastCCDClockUpload, 0);
  status |= setIntegerParam(FastCCDBiasUpload, 0);

  status |= setIntegerParam(FastCCDPower, 0);
  status |= setIntegerParam(FastCCDFPPower, 0);
  status |= setIntegerParam(FastCCDCameraPower, 0);
  status |= setIntegerParam(FastCCDCameraPowerMode, 3);

  status |= setIntegerParam(FastCCDBias, 0);
  status |= setIntegerParam(FastCCDClocks, 0);

  status |= setUIntDigitalParam(FastCCDFPGAStatus, 0x0, 0xFFFF);
  status |= setUIntDigitalParam(FastCCDDCMStatus, 0x0, 0xFFFF);

  status |= setIntegerParam(FastCCDMux1, 0);
  status |= setIntegerParam(FastCCDMux2, 0);

  status |= setStringParam(FastCCDFirmwarePath, "");
  status |= setStringParam(FastCCDBiasPath, "");
  status |= setStringParam(FastCCDClockPath, "");
  status |= setStringParam(FastCCDFCRICPath, "");

  status |= setIntegerParam(FastCCDOverscanRows, 0);
  status |= setIntegerParam(FastCCDOverscanCols, 0);

  status |= setIntegerParam(FastCCDFclk, 0);

  status |= setIntegerParam(FastCCDFCRICGain, 0);
  status |= setIntegerParam(FastCCDFCRICClamp, 0);

  status |= setDoubleParam(FastCCDVBus12V0, 0);
  status |= setDoubleParam(FastCCDVMgmt3v3, 0);
  status |= setDoubleParam(FastCCDVMgmt2v5, 0);
  status |= setDoubleParam(FastCCDVMgmt1v2, 0);
  status |= setDoubleParam(FastCCDVEnet1v0, 0);
  status |= setDoubleParam(FastCCDVS3E3v3, 0);
  status |= setDoubleParam(FastCCDVGen3v3, 0);
  status |= setDoubleParam(FastCCDVGen2v5, 0);
  status |= setDoubleParam(FastCCDV60v9, 0);
  status |= setDoubleParam(FastCCDV61v0, 0);
  status |= setDoubleParam(FastCCDV62v5, 0);
  status |= setDoubleParam(FastCCDVFp, 0);

  status |= setDoubleParam(FastCCDIBus12V0, 0);
  status |= setDoubleParam(FastCCDIMgmt3v3, 0);
  status |= setDoubleParam(FastCCDIMgmt2v5, 0);
  status |= setDoubleParam(FastCCDIMgmt1v2, 0);
  status |= setDoubleParam(FastCCDIEnet1v0, 0);
  status |= setDoubleParam(FastCCDIS3E3v3, 0);
  status |= setDoubleParam(FastCCDIGen3v3, 0);
  status |= setDoubleParam(FastCCDIGen2v5, 0);
  status |= setDoubleParam(FastCCDI60v9, 0);
  status |= setDoubleParam(FastCCDI61v0, 0);
  status |= setDoubleParam(FastCCDI62v5, 0);
  status |= setDoubleParam(FastCCDIFp, 0);

  status |= setDoubleParam(FastCCDBiasPosH, 0);
  status |= setDoubleParam(FastCCDBiasNegH, 0);
  status |= setDoubleParam(FastCCDBiasPosRG, 0);
  status |= setDoubleParam(FastCCDBiasNegRG, 0);
  status |= setDoubleParam(FastCCDBiasPosSW, 0);
  status |= setDoubleParam(FastCCDBiasNegSW, 0);
  status |= setDoubleParam(FastCCDBiasPosV, 0);
  status |= setDoubleParam(FastCCDBiasNegV, 0);
  status |= setDoubleParam(FastCCDBiasPosTG, 0);
  status |= setDoubleParam(FastCCDBiasNegTG, 0);
  status |= setDoubleParam(FastCCDBiasPosVF, 0);
  status |= setDoubleParam(FastCCDBiasNegVF, 0);
  status |= setDoubleParam(FastCCDBiasNEDGE, 0);
  status |= setDoubleParam(FastCCDBiasOTG, 0);
  status |= setDoubleParam(FastCCDBiasVDDR, 0);
  status |= setDoubleParam(FastCCDBiasVDDOut, 0);
  status |= setDoubleParam(FastCCDBiasBufBase, 0);
  status |= setDoubleParam(FastCCDBiasBufDelta, 0);
  status |= setDoubleParam(FastCCDBiasSpare1, 0);
  status |= setDoubleParam(FastCCDBiasSpare2, 0);

  status |= setStringParam(ADSDKVersion, (char *)cin_build_version);

  status |= setIntegerParam(FastCCDTimingMode, 0);

  // Now set timing names
  
  for(int i=0;i<10;i++)
  {
    status |= setStringParam(FastCCDTimingNameN[i], "");
  }

  try {
    this->lock();
    connectCamera();
    this->unlock();
    setStringParam(ADStatusMessage, "Camera connected");
  } catch (const std::string &e) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s:%s: %s\n",
      driverName, functionName, e.c_str());
    setStringParam(ADStatusMessage, "Camera failed to connect");
    return;
  }

  callParamCallbacks();

  // Signal the status thread to poll the detector
  epicsEventSignal(statusEvent);
  epicsEventSignal(dataStatsEvent);
  
  if (stackSize == 0) {
    stackSize = epicsThreadGetStackSize(epicsThreadStackMedium);
  }

  /* Create the thread that updates the detector status */
  status = (epicsThreadCreate("FastCCDStatusTask",
                              epicsThreadPriorityMedium,
                              stackSize,
                              (EPICSTHREADFUNC)FastCCDStatusTaskC,
                              this) == NULL);
  if(status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s:%s: Failed to create status task.\n",
      driverName, functionName);
    return;
  }

  /* Create the thread that updates the data stats */
  status = (epicsThreadCreate("FastCCDDataStatsTask",
                              epicsThreadPriorityMedium,
                              stackSize,
                              (EPICSTHREADFUNC)FastCCDDataStatsTaskC,
                              this) == NULL);
  if(status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s:%s: Failed to create data stats task.\n",
      driverName, functionName);
    return;
  }

  /* Create the thread that waits for camera completion */
  status = (epicsThreadCreate("FastCCDDetectorWaitTask",
                              epicsThreadPriorityMedium,
                              stackSize,
                              (EPICSTHREADFUNC)FastCCDDetectorWaitTaskC,
                              this) == NULL);
  if(status) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s:%s: Failed to create data stats task.\n",
      driverName, functionName);
    return;
  }
}

/**
 * Destructor.  Free resources and closes the FastCCD library
 */
FastCCD::~FastCCD() 
{
  static const char *functionName = "~FastCCD";

  try {
    this->lock();
    cin_data_destroy(&cin_data);
    cin_ctl_destroy(&cin_ctl);
    this->unlock();
  } catch (const std::string &e) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s:%s: %s\n",
      driverName, functionName, e.c_str());
  }
}


/**
 * Exit handler, delete the FastCCD object.
 */

static void exitHandler(void *drvPvt)
{
  FastCCD *pFastCCD = (FastCCD *)drvPvt;
  delete pFastCCD;
}

/*
 * Routines to configure CIN
 */

int FastCCD::uploadConfig(int status, int path){

  int _status = 0;
  char _path[256];

  getStringParam(path, sizeof(_path), _path);
  setIntegerParam(status, 1);
  setStringParam(ADStatusMessage, "Uploading Config File");
  callParamCallbacks();

  _status = cin_ctl_load_config(&cin_ctl, _path);

  setIntegerParam(status, 0);
  if(!_status){
    setStringParam(ADStatusMessage, "Config Uploaded to CIN");
    setParamStatus(path, asynSuccess);
  } else {
    setStringParam(ADStatusMessage, "ERROR Uploading Config to CIN");
    setParamStatus(path, asynError);
  }

  return _status;
}

int FastCCD::uploadFirmware(void){
  int _status = 0;

  char path[256];
  getStringParam(FastCCDFirmwarePath, sizeof(path), path);
  setIntegerParam(FastCCDFirmwareUpload, 1);

  // Power off the cin

  setStringParam(ADStatusMessage, "Powering CIN OFF");
  callParamCallbacks();
  if(cin_ctl_pwr(&cin_ctl, 0)){
    goto error;
  }

  sleep(3);
  getCameraStatus(0);

  // Power on the cin

  setStringParam(ADStatusMessage, "Powering CIN ON");
  callParamCallbacks();
  if(cin_ctl_pwr(&cin_ctl, 1)){
    goto error;
  }

  sleep(3);
  getCameraStatus(0);

  setStringParam(ADStatusMessage, "Uploading Firmware to CIN");
  callParamCallbacks();
  _status |= cin_ctl_load_firmware_file(&cin_ctl, path);
 
  if(!_status){
    _status |= cin_ctl_set_fabric_address(&cin_ctl, (char *)cinFabricIP);
    _status |= cin_data_send_magic(&cin_data);
  }

  setIntegerParam(FastCCDFirmwareUpload, 0);

  sleep(3);
  getCameraStatus(1);

error:

  if(_status){
    setStringParam(ADStatusMessage, "ERROR Uploading Firmware");
  } else {
    setStringParam(ADStatusMessage, "Firmware uploaded to CIN");
  }

  return _status;
}


/** Report status of the driver.
  * Prints details about the detector in us if details > 0.
  * It then calls the ADDriver::report() method.
  * \param[in] fp File pointed passed by caller where the output is written to.
  * \param[in] details Controls the level of detail in the report. */
void FastCCD::report(FILE *fp, int details)
{
  int xsize, ysize;
  static const char *functionName = "report";

  fprintf(fp, "FastCCD CCD port = %s\n", this->portName);
  if (details > 0) {
    try {
      getIntegerParam(ADMaxSizeX, &xsize);
      getIntegerParam(ADMaxSizeY, &ysize);
      fprintf(fp, "  X pixels: %d\n", xsize);
      fprintf(fp, "  Y pixels: %d\n", ysize);

    } catch (const std::string &e) {
      asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
        "%s:%s: %s\n",
        driverName, functionName, e.c_str());
    }
  }
  // Call the base class method
  ADDriver::report(fp, details);
}

asynStatus FastCCD::writeFloat64(asynUser *pasynUser, epicsFloat64 value){
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    int _status = 0;
    int _framestore = 0;

    const char *paramName;
    static const char *functionName = "writeFloat64";

    getParamName(function, &paramName);

    getIntegerParam(FastCCDFramestore, &_framestore);

    status = setDoubleParam(function, value);
    if(status != asynSuccess){
      return status;
    }

    if(function == ADAcquireTime){
      if(_framestore){
        _status = cin_ctl_set_cycle_time(&cin_ctl, (float)value);
        setDoubleParam(ADAcquirePeriod, value);
      } else {
        _status = cin_ctl_set_exposure_time(&cin_ctl, (float)value);
      }

    } else if (function == ADAcquirePeriod) {

      if(_framestore){
        _status = cin_ctl_set_cycle_time(&cin_ctl, (float)value);
        setDoubleParam(ADAcquireTime, value);
      } else {
        _status = cin_ctl_set_cycle_time(&cin_ctl, (float)value);
      }

    } else if (function == FastCCDPollingPeriod){

      // Set the new polling period and poll
      statusPollingPeriod = value;
      epicsEventSignal(statusEvent);

    } else {
      ADDriver::writeFloat64(pasynUser, value);
    }

    if(_status){
      setParamStatus(function, asynError);
      status = asynError;
    } else {
      setParamStatus(function, asynSuccess);
    }

    if(status){
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
              "%s:%s: error, status=%d function=%d, value=%f\n",
              driverName, functionName, status, function, value);
    } else {
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, value=%f\n",
              driverName, functionName, function, value);
    }
    return status;

}


/** Called when asyn clients call pasynInt32->writr().
  * This function performs actions for some parameters, including ADAcquire, ADBinX, etc.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus FastCCD::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;

    asynStatus status = asynSuccess;
    static const char *functionName = "writeInt32";

    //Set in param lib so the user sees a readback straight away. Save a backup in case of errors.
    setIntegerParam(function, value);

    int _status = 0; // For return of cin functions

    if(function == ADAcquire){
      if(value){ // User clicked 'Start' button

         // Send the hardware a start trigger command
         int n_images, t_mode, i_mode;
         double t_exp, t_period;
         int _framestore;

         getIntegerParam(FastCCDFramestore, &_framestore);
         getIntegerParam(ADTriggerMode, &t_mode);
         getIntegerParam(ADImageMode, &i_mode);
         getIntegerParam(ADNumImages, &n_images);
         getDoubleParam(ADAcquireTime, &t_exp);
         getDoubleParam(ADAcquirePeriod, &t_period);

         n_images &= 0xFFFF; // Only use least significant bits

         setIntegerParam(ADNumImages, n_images);

         switch(i_mode) {
           case ADImageSingle:
             this->framesRemaining = 1;
             this->freeRun = 0;
             n_images = 1 + _framestore;
             break;
           case ADImageMultiple:
             this->framesRemaining = n_images;
             this->freeRun = 0;
             if(_framestore){
               n_images += _framestore;
             }
             break;
           case ADImageContinuous:
             this->framesRemaining = 0;
             this->freeRun = 1;
             n_images = 0;
             break;
         }

         if(_framestore)
         {
           _status |= cin_ctl_set_exposure_time(&cin_ctl, 0);
         } else {
           _status |= cin_ctl_set_exposure_time(&cin_ctl, (float)t_exp);
         }

         if(t_mode == 0){
           if(_framestore) {
             _status |= cin_ctl_set_cycle_time(&cin_ctl, (float)t_exp);
             setDoubleParam(ADAcquirePeriod, t_exp);
             setDoubleParam(ADAcquireTime, t_exp);
             firstFrameFlag = _framestore;
           } else {
             _status |= cin_ctl_set_cycle_time(&cin_ctl, (float)t_period);
             firstFrameFlag = 0;
           }
           if(!_status){
             // Make areadetector do image counting in software.
             _status |= cin_ctl_int_trigger_start(&cin_ctl, 0);
           }
         } else {
           _status |= cin_ctl_ext_trigger_start(&cin_ctl, t_mode);
         }

         if(!_status){
           setIntegerParam(ADAcquire, 1);
           setIntegerParam(ADStatus, ADStatusAcquire);
           setParamStatus(ADAcquire, asynSuccess);
           setParamStatus(ADStatus, asynSuccess);
         } else {
           setIntegerParam(ADAcquire, 1);
           setIntegerParam(ADStatus, ADStatusIdle);
           setParamStatus(ADAcquire, asynError);
           setParamStatus(ADStatus, asynError);
         }

      } else {
         // Send the hardware a stop trigger command
         _status |= cin_ctl_int_trigger_stop(&cin_ctl);
         _status |= cin_ctl_ext_trigger_stop(&cin_ctl);

         if(!_status)
         {
           epicsEventSignal(detectorWaitEvent);
         }
      
      }

    } else if (function == FastCCDFirmwareUpload) {

      uploadFirmware();
      epicsEventSignal(statusEvent);

    } else if (function == FastCCDClockUpload) {

      _status = uploadConfig(FastCCDClockUpload, FastCCDClockPath);

    } else if (function == FastCCDBiasUpload) {

      _status = uploadConfig(FastCCDBiasUpload, FastCCDBiasPath);
      sleep(3);
      getCameraStatus(1);

    } else if (function == FastCCDFCRICUpload) {

      _status = uploadConfig(FastCCDFCRICUpload, FastCCDFCRICPath);

    } else if (function == FastCCDBoot) {
      // Get timing mode
      int mode;
      getIntegerParam(FastCCDTimingMode, &mode);

      setIntegerParam(FastCCDBoot, 1);
      callParamCallbacks();

      _status = cin_com_boot(&cin_ctl, &cin_data, mode);
      // Now we should set parameters such as size and overscan.
      
      int _val1, _val2, _x, _y;
      cin_data_get_descramble_params(&cin_data, &_val1, &_val2, &_x, &_y);
      setIntegerParam(ADSizeX, _x);
      setIntegerParam(ADSizeY, _y);
      setIntegerParam(FastCCDOverscanCols, _val2);

      setIntegerParam(FastCCDBoot, 0);
      callParamCallbacks();

      epicsEventSignal(statusEvent);

    } else if (function == FastCCDSendFCRIC) {
      setParamStatus(FastCCDSendFCRIC, asynSuccess);
      setIntegerParam(FastCCDSendFCRIC, 1);
      callParamCallbacks();

      _status = cin_ctl_set_fcric(&cin_ctl);

      setIntegerParam(FastCCDSendFCRIC, 0);
      callParamCallbacks();
      epicsEventSignal(statusEvent);

    } else if (function == FastCCDSendBias) {
      setParamStatus(FastCCDSendBias, asynSuccess);
      setIntegerParam(FastCCDSendBias, 1);
      callParamCallbacks();

      _status = cin_ctl_upload_bias(&cin_ctl);

      setIntegerParam(FastCCDSendBias, 0);
      callParamCallbacks();
      epicsEventSignal(statusEvent);

    } else if (function == FastCCDPower) {

      if(value) {
        _status |= cin_ctl_pwr(&cin_ctl, 1);
      } else {
        _status |= cin_ctl_pwr(&cin_ctl, 0);
      }

      sleep(3);

      epicsEventSignal(statusEvent);

    } else if (function == FastCCDFPPower) {
      if(value) {
        _status |= cin_ctl_fp_pwr(&cin_ctl, 1);
      } else {
        _status |= cin_ctl_fp_pwr(&cin_ctl, 0);
      }

      sleep(3);

      epicsEventSignal(statusEvent);

    } else if (function == FastCCDCameraPower) {
      
      // Get the mode we are in (clocks, bias or both)
      int _mode;
      getIntegerParam(FastCCDCameraPowerMode, &_mode); 

      if(value){
        if(_mode & 0x1){
          _status |= cin_ctl_set_clocks(&cin_ctl, 1);
        }
        if(_mode & 0x2){
          _status |= cin_ctl_set_bias(&cin_ctl, 1);
        }
      } else {
        _status |= cin_ctl_set_bias(&cin_ctl, 0);
        _status |= cin_ctl_set_clocks(&cin_ctl, 0);
      }

      sleep(3);

      epicsEventSignal(statusEvent);

    } else if ((function == FastCCDMux1) || (function == FastCCDMux2)) {

      int _val, _val1, _val2;
      getIntegerParam(FastCCDMux1, &_val1);
      getIntegerParam(FastCCDMux2, &_val2);
      _val = (_val2 << 4) | _val1;
      _status |= cin_ctl_set_mux(&cin_ctl, _val);   

      epicsEventSignal(statusEvent);

    } else if (function == FastCCDResetStats) {

      cin_data_reset_stats(&cin_data);

    } else if ((function == ADSizeY) || (function == FastCCDOverscanCols)) {

      // The Y size changed, change the descramble routine
      int _val1, _val2;
      getIntegerParam(ADSizeY, &_val1);
      getIntegerParam(FastCCDOverscanCols, &_val2);
      _status |= cin_data_set_descramble_params(&cin_data, _val1, _val2);

      // Read back to check all OK

      int _x, _y;
      cin_data_get_descramble_params(&cin_data, &_val1, &_val2, &_x, &_y);
      setIntegerParam(ADSizeX, _x);
      setIntegerParam(ADSizeY, _y);
      setIntegerParam(FastCCDOverscanCols, _val2);

    } else if (function == FastCCDFclk) {

      _status |= cin_ctl_set_fclk(&cin_ctl, value);
      epicsEventSignal(statusEvent);

    } else if (function == FastCCDFCRICGain){

      _status |= cin_ctl_set_fcric_gain(&cin_ctl, value);

    } else if (function == FastCCDFCRICClamp){

      _status |= cin_ctl_set_fcric_clamp(&cin_ctl, value);

    } else if (function == FastCCDBiasWriteV){
      float bias_voltage[CIN_CTL_NUM_BIAS];
      double _v;

      getDoubleParam(FastCCDBiasPosHW, &_v);
      bias_voltage[CIN_CTL_BIAS_POSH] = _v;
      
      getDoubleParam(FastCCDBiasNegHW, &_v);
      bias_voltage[CIN_CTL_BIAS_NEGH] = _v;

      getDoubleParam(FastCCDBiasPosRGW, &_v);
      bias_voltage[CIN_CTL_BIAS_POSRG] = _v;

      getDoubleParam(FastCCDBiasNegRGW, &_v);
      bias_voltage[CIN_CTL_BIAS_NEGRG] = _v;

      getDoubleParam(FastCCDBiasPosSWW, &_v);
      bias_voltage[CIN_CTL_BIAS_POSSW] = _v;

      getDoubleParam(FastCCDBiasNegSWW, &_v);
      bias_voltage[CIN_CTL_BIAS_NEGSW] = _v;

      getDoubleParam(FastCCDBiasPosVW, &_v);
      bias_voltage[CIN_CTL_BIAS_POSV] = _v;

      getDoubleParam(FastCCDBiasNegVW, &_v);
      bias_voltage[CIN_CTL_BIAS_NEGV] = _v;

      getDoubleParam(FastCCDBiasPosTGW, &_v);
      bias_voltage[CIN_CTL_BIAS_POSTG] = _v;

      getDoubleParam(FastCCDBiasNegTGW, &_v);
      bias_voltage[CIN_CTL_BIAS_NEGTG] = _v;

      getDoubleParam(FastCCDBiasPosVFW, &_v);
      bias_voltage[CIN_CTL_BIAS_POSVF] = _v;

      getDoubleParam(FastCCDBiasNegVFW, &_v);
      bias_voltage[CIN_CTL_BIAS_NEGVF] = _v;

      getDoubleParam(FastCCDBiasNEDGEW, &_v);
      bias_voltage[CIN_CTL_BIAS_NEDGE] = _v;

      getDoubleParam(FastCCDBiasOTGW, &_v);
      bias_voltage[CIN_CTL_BIAS_OTG] = _v;

      getDoubleParam(FastCCDBiasVDDRW, &_v);
      bias_voltage[CIN_CTL_BIAS_VDDR] = _v;

      getDoubleParam(FastCCDBiasVDDOutW, &_v);
      bias_voltage[CIN_CTL_BIAS_VDD_OUT] = _v;

      getDoubleParam(FastCCDBiasBufBaseW, &_v);
      bias_voltage[CIN_CTL_BIAS_BUF_BASE] = _v;

      getDoubleParam(FastCCDBiasBufDeltaW, &_v);
      bias_voltage[CIN_CTL_BIAS_BUF_DELTA] = _v;

      getDoubleParam(FastCCDBiasSpare1W, &_v);
      bias_voltage[CIN_CTL_BIAS_SPARE1] = _v;

      getDoubleParam(FastCCDBiasSpare2W, &_v);
      bias_voltage[CIN_CTL_BIAS_SPARE2] = _v;

      _status |= cin_ctl_set_bias_voltages(&cin_ctl, bias_voltage, 1);

      epicsEventSignal(statusEvent);
      
    } else if(function == FastCCDFOTest) {

      _status |= cin_ctl_fo_test_pattern(&cin_ctl, value);

    } else if(function == FastCCDSendTiming) {
      int val;
      getIntegerParam(FastCCDTimingMode, &val);

      // Check if mode exists

      if(cin_com_set_timing(&cin_ctl, &cin_data, val) != CIN_ERROR)
      {
        // Now readback the name
        char _name[CIN_CONFIG_MAX_TIMING_NAME];
        char *name = _name;
        _status = cin_config_get_current_timing_name(&cin_ctl, &name);
        setStringParam(FastCCDTimingName, name);
      } else {
        _status = CIN_ERROR;
      }
    } else if(function == FastCCDTimingMode){
      char _name[CIN_CONFIG_MAX_TIMING_NAME];
      char *name = _name;
      int _val;
      getIntegerParam(FastCCDTimingMode, &_val);
      _status = cin_config_get_timing_name(&cin_ctl, _val, &name);
      if(_status != CIN_ERROR)
      {
        setStringParam(FastCCDTimingName, name);
        setParamStatus(FastCCDTimingName, asynSuccess);
      } else {
        setStringParam(FastCCDTimingName, "");
        setParamStatus(FastCCDTimingName, asynError);
      }
    } else {
      status = ADDriver::writeInt32(pasynUser, value);
    }

    if(_status != CIN_OK){
      status = asynError;
      setParamStatus(function, asynError);
    } else {
      setParamStatus(function, asynSuccess);
    }

    if (status) {
      asynPrint(pasynUser, ASYN_TRACE_ERROR,
            "%s:%s: error, status=%d function=%d, value=%d\n",
            driverName, functionName, status, function, value);
    } else {
      asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
            "%s:%s: function=%d, value=%d\n",
            driverName, functionName, function, value);
    }

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks();

    return status;
}

asynStatus FastCCD::writeOctet(asynUser *pasynUser, const char *value, size_t nc, size_t *na){
  int function = pasynUser->reason;
  int addr = 0;
  asynStatus status = asynSuccess;
  static const char *functionName = "writeOctet";

  status = getAddress(pasynUser, &addr);
  if(status != asynSuccess){
    return(status);
  }
  status = (asynStatus)setStringParam(addr, function, (char *)value);
  if(status != asynSuccess){
    return(status);
  }

  if(function == FastCCDFirmwarePath || function == FastCCDClockPath ||
     function == FastCCDFCRICPath    || function == FastCCDBiasPath ){
    struct stat s;
    int _status = stat(value, &s);
    if(_status){
      setParamStatus(function, asynError);
    } else {
      setParamStatus(function, asynSuccess);
    }
  } else {
    // Call base class to handle strings
    status = ADDriver::writeOctet(pasynUser, value, nc, na);
  }

  if (status) {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
          "%s:%s: error, status=%d function=%d, value=%s\n",
          driverName, functionName, status, function, value);
  } else {
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
          "%s:%s: function=%d, value=%s\n",
          driverName, functionName, function, value);
  }


  /* Do callbacks so higher layers see any changes */
  callParamCallbacks();
 
  // Set the number of characters written
  *na = nc;
  return status;
}

void FastCCD::dataStatsTask(void)
{
  unsigned int status = 0;
  double timeout = 0.0;
  static const char *functionName = "dataStatsTask";

  while(1) {

    //Read timeout for polling freq.
    this->lock();
    timeout = dataStatsPollingPeriod;
    this->unlock();

    if (timeout != 0.0) {
      status = epicsEventWaitWithTimeout(dataStatsEvent, timeout);
    } else {
      status = epicsEventWait(dataStatsEvent);
    }              
  
    if (status == epicsEventWaitOK) {
      asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s: Got status event\n",
        driverName, functionName);
    }

    cin_data_stats_t stats;
    cin_data_compute_stats(&cin_data, &stats);
    setIntegerParam(FastCCDDroppedPck, (int)stats.dropped_packets);
    setIntegerParam(FastCCDBadPck, (int)stats.mallformed_packets);
    setIntegerParam(FastCCDLastFrame, stats.last_frame);
    setIntegerParam(FastCCDPacketBuffer, stats.packet_used);
    setIntegerParam(FastCCDFrameBuffer, stats.frame_used);
    
    this->lock();
    callParamCallbacks();
    this->unlock();
  }
}

/*
 * Do a single poll of the detector to get all the parameters
 */

void FastCCD::getCameraStatus(int first_run){

  cin_ctl_id_t id;
  int cin_status = 0;
  cin_ctl_pwr_mon_t pwr_value;
  int pwr = 0;
  int full = 0;

  if(first_run){
    cin_status |= cin_ctl_get_id(&cin_ctl, &id);
    if(!cin_status){
      setIntegerParam(FastCCDBaseBoardID, id.base_board_id);
      setIntegerParam(FastCCDBaseSerialNum, id.base_serial_no);
      setIntegerParam(FastCCDBaseFPGAVersion, id.base_fpga_ver);
      setIntegerParam(FastCCDFabBoardID, id.fabric_board_id);
      setIntegerParam(FastCCDFabSerialNum, id.fabric_serial_no);
      setIntegerParam(FastCCDFabFPGAVersion, id.fabric_fpga_ver);
      char buffer[50];
      sprintf(buffer, "0x%04X", id.fabric_fpga_ver);
      setStringParam(ADFirmwareVersion, (char *)buffer);

      setParamStatus(FastCCDBaseBoardID, asynSuccess);
      setParamStatus(FastCCDBaseSerialNum, asynSuccess);
      setParamStatus(FastCCDBaseFPGAVersion, asynSuccess);
      setParamStatus(FastCCDFabBoardID, asynSuccess);
      setParamStatus(FastCCDFabSerialNum, asynSuccess);
      setParamStatus(FastCCDFabFPGAVersion, asynSuccess);
    } else {
      setParamStatus(FastCCDBaseBoardID, asynDisconnected);
      setParamStatus(FastCCDBaseSerialNum, asynDisconnected);
      setParamStatus(FastCCDBaseFPGAVersion, asynDisconnected);
      setParamStatus(FastCCDFabBoardID, asynDisconnected);
      setParamStatus(FastCCDFabSerialNum, asynDisconnected);
      setParamStatus(FastCCDFabFPGAVersion, asynDisconnected);
    }
  }

  cin_status = cin_ctl_get_power_status(&cin_ctl, full, &pwr, &pwr_value);
  if(!cin_status){
    // Power Status
    if(pwr){
      setIntegerParam(FastCCDPower, 1);
      if(pwr == 2){
        setIntegerParam(FastCCDFPPower, 1);
      } else {
        setIntegerParam(FastCCDFPPower, 0);
      }
     
      // Voltage Values

      setDoubleParam(FastCCDVBus12V0, pwr_value.bus_12v0.v);
      setDoubleParam(FastCCDIBus12V0, pwr_value.bus_12v0.i);
      setParamStatus(FastCCDVBus12V0, asynSuccess);
      setParamStatus(FastCCDIBus12V0, asynSuccess);

      if(full){

        setDoubleParam(FastCCDVMgmt3v3, pwr_value.mgmt_3v3.v);
        setDoubleParam(FastCCDVMgmt2v5, pwr_value.mgmt_2v5.v);
        setDoubleParam(FastCCDVMgmt1v2, pwr_value.mgmt_1v2.v);
        setDoubleParam(FastCCDVEnet1v0, pwr_value.enet_1v0.v);
        setDoubleParam(FastCCDVS3E3v3,  pwr_value.s3e_3v3.v);
        setDoubleParam(FastCCDVGen3v3,  pwr_value.gen_3v3.v);
        setDoubleParam(FastCCDVGen2v5,  pwr_value.gen_2v5.v);
        setDoubleParam(FastCCDV60v9,    pwr_value.v6_0v9.v);
        setDoubleParam(FastCCDV61v0,    pwr_value.v6_1v0.v);
        setDoubleParam(FastCCDV62v5,    pwr_value.v6_2v5.v);
        setParamStatus(FastCCDVMgmt3v3, asynSuccess);
        setParamStatus(FastCCDVMgmt2v5, asynSuccess);
        setParamStatus(FastCCDVMgmt1v2, asynSuccess);
        setParamStatus(FastCCDVEnet1v0, asynSuccess);
        setParamStatus(FastCCDVS3E3v3,  asynSuccess);
        setParamStatus(FastCCDVGen3v3,  asynSuccess);
        setParamStatus(FastCCDVGen2v5,  asynSuccess);
        setParamStatus(FastCCDV60v9,    asynSuccess);
        setParamStatus(FastCCDV61v0,    asynSuccess);
        setParamStatus(FastCCDV62v5,    asynSuccess);
        
        // Current Values

        setDoubleParam(FastCCDIMgmt3v3, pwr_value.mgmt_3v3.i);
        setDoubleParam(FastCCDIMgmt2v5, pwr_value.mgmt_2v5.i);
        setDoubleParam(FastCCDIMgmt1v2, pwr_value.mgmt_1v2.i);
        setDoubleParam(FastCCDIEnet1v0, pwr_value.enet_1v0.i);
        setDoubleParam(FastCCDIS3E3v3,  pwr_value.s3e_3v3.i);
        setDoubleParam(FastCCDIGen3v3,  pwr_value.gen_3v3.i);
        setDoubleParam(FastCCDIGen2v5,  pwr_value.gen_2v5.i);
        setDoubleParam(FastCCDI60v9,    pwr_value.v6_0v9.i);
        setDoubleParam(FastCCDI61v0,    pwr_value.v6_1v0.i);
        setDoubleParam(FastCCDI62v5,    pwr_value.v6_2v5.i);
        setParamStatus(FastCCDIMgmt3v3, asynSuccess);
        setParamStatus(FastCCDIMgmt2v5, asynSuccess);
        setParamStatus(FastCCDIMgmt1v2, asynSuccess);
        setParamStatus(FastCCDIEnet1v0, asynSuccess);
        setParamStatus(FastCCDIS3E3v3,  asynSuccess);
        setParamStatus(FastCCDIGen3v3,  asynSuccess);
        setParamStatus(FastCCDIGen2v5,  asynSuccess);
        setParamStatus(FastCCDI60v9,    asynSuccess);
        setParamStatus(FastCCDI61v0,    asynSuccess);
        setParamStatus(FastCCDI62v5,    asynSuccess);

      }

      setDoubleParam(FastCCDVFp,      pwr_value.fp.v);
      setDoubleParam(FastCCDIFp,      pwr_value.fp.i);
      setParamStatus(FastCCDVFp,      asynSuccess);
      setParamStatus(FastCCDIFp,      asynSuccess);


    } else {
      setIntegerParam(FastCCDPower, 0);
      setIntegerParam(FastCCDFPPower, 0);
    }

    setParamStatus(FastCCDPower, asynSuccess);
    setParamStatus(FastCCDFPPower, asynSuccess);
  }

  if(cin_status || !pwr){

    setParamStatus(FastCCDVBus12V0, asynDisconnected);
    setParamStatus(FastCCDIBus12V0, asynDisconnected);
    setParamStatus(FastCCDVFp,      asynDisconnected);
    setParamStatus(FastCCDIFp,      asynDisconnected);

  }

  if(!full || cin_status || !pwr){
      
    // Voltage Values
    
    setParamStatus(FastCCDVMgmt3v3, asynDisconnected);
    setParamStatus(FastCCDVMgmt2v5, asynDisconnected);
    setParamStatus(FastCCDVMgmt1v2, asynDisconnected);
    setParamStatus(FastCCDVEnet1v0, asynDisconnected);
    setParamStatus(FastCCDVS3E3v3,  asynDisconnected);
    setParamStatus(FastCCDVGen3v3,  asynDisconnected);
    setParamStatus(FastCCDVGen2v5,  asynDisconnected);
    setParamStatus(FastCCDV60v9,    asynDisconnected);
    setParamStatus(FastCCDV61v0,    asynDisconnected);
    setParamStatus(FastCCDV62v5,    asynDisconnected);

    // Current Values

    setParamStatus(FastCCDIMgmt3v3, asynDisconnected);
    setParamStatus(FastCCDIMgmt2v5, asynDisconnected);
    setParamStatus(FastCCDIMgmt1v2, asynDisconnected);
    setParamStatus(FastCCDIEnet1v0, asynDisconnected);
    setParamStatus(FastCCDIS3E3v3,  asynDisconnected);
    setParamStatus(FastCCDIGen3v3,  asynDisconnected);
    setParamStatus(FastCCDIGen2v5,  asynDisconnected);
    setParamStatus(FastCCDI60v9,    asynDisconnected);
    setParamStatus(FastCCDI61v0,    asynDisconnected);
    setParamStatus(FastCCDI62v5,    asynDisconnected);

  }

  // Status

  uint16_t fpga_status, dcm_status;
  
  cin_status = cin_ctl_get_cfg_fpga_status(&cin_ctl, &fpga_status);
  if(!cin_status){
    setUIntDigitalParam(FastCCDFPGAStatus, fpga_status, 0xFFFF);
    setParamStatus(FastCCDFPGAStatus, asynSuccess);
  } else {
    setParamStatus(FastCCDFPGAStatus, asynDisconnected);
  }
  
  cin_status = cin_ctl_get_dcm_status(&cin_ctl, &dcm_status);
    if(!cin_status){
    setUIntDigitalParam(FastCCDDCMStatus, dcm_status, 0xFFFF);
    setParamStatus(FastCCDDCMStatus, asynSuccess);
  } else {
    setParamStatus(FastCCDDCMStatus, asynDisconnected);
  }

  /* Are we powered up and configured? */

  if(fpga_status & CIN_CTL_FPGA_STS_CFG){

    // Clock and Bias status
  
    int _val;
    cin_status = cin_ctl_get_camera_pwr(&cin_ctl, &_val);
    if(!cin_status){
      setIntegerParam(FastCCDCameraPower, _val);
      setParamStatus(FastCCDCameraPower, asynSuccess);
    } else {
      setParamStatus(FastCCDCameraPower, asynDisconnected);
    }

    cin_status = cin_ctl_get_clocks(&cin_ctl, &_val);
    if(!cin_status){
      setIntegerParam(FastCCDClocks, _val);
      setParamStatus(FastCCDClocks, asynSuccess);
    } else {
      setParamStatus(FastCCDClocks, asynDisconnected);
    }
 
    cin_status = cin_ctl_get_bias(&cin_ctl, &_val);
    if(!cin_status){
      setIntegerParam(FastCCDBias, _val);
      setParamStatus(FastCCDBias, asynSuccess);
    } else {
      setParamStatus(FastCCDBias, asynDisconnected);
    }

    // Get Mux Settings
    
    int mux;
    cin_status = cin_ctl_get_mux(&cin_ctl, &mux);
    if(!cin_status){
      setIntegerParam(FastCCDMux1, (mux & 0x000F));
      setIntegerParam(FastCCDMux2, (mux & 0x00F0) >> 4);
      setParamStatus(FastCCDMux1, asynSuccess);
      setParamStatus(FastCCDMux2, asynSuccess);
    } else {
      setParamStatus(FastCCDMux1, asynDisconnected);
      setParamStatus(FastCCDMux2, asynDisconnected);
    }


    // Get FCLK Settings

    int fclk;
    cin_status = cin_ctl_get_fclk(&cin_ctl, &fclk);
    if(!cin_status){
      setIntegerParam(FastCCDFclk, fclk);
      setParamStatus(FastCCDFclk, asynSuccess);
    } else {
      setParamStatus(FastCCDFclk, asynDisconnected);
    }

    // Can we get the timing settings
    // Are we triggering ?

    if(first_run){

      int mode;
      cin_status = cin_com_get_timing(&cin_ctl, &cin_data, &mode);
      if(cin_status == CIN_OK)
      {
        int _val1, _val2, _x, _y;
        cin_data_get_descramble_params(&cin_data, &_val1, &_val2, &_x, &_y);
        setIntegerParam(ADSizeX, _x);
        setIntegerParam(ADSizeY, _y);
        setIntegerParam(FastCCDOverscanCols, _val2);
        setParamStatus(ADSizeX, asynSuccess);
        setParamStatus(ADSizeY, asynSuccess);
        setParamStatus(FastCCDOverscanCols, asynSuccess);
        setIntegerParam(FastCCDTimingMode, mode);
        setParamStatus(FastCCDTimingMode, asynSuccess);
      } else {
        setParamStatus(FastCCDTimingMode, asynDisconnected);
        setParamStatus(ADSizeX, asynDisconnected);
        setParamStatus(ADSizeY, asynDisconnected);
        setParamStatus(FastCCDOverscanCols, asynDisconnected);
      }

      int trig;
      cin_status = cin_ctl_get_triggering(&cin_ctl, &trig);
      if(!cin_status)
      {
        if(trig)
        {
          setIntegerParam(ADAcquire, 1);
          setIntegerParam(ADStatus, ADStatusAcquire);
          float _exp, _cycle;
          cin_status = cin_ctl_get_exposure_time(&cin_ctl, &_exp);
          cin_status |= cin_ctl_get_cycle_time(&cin_ctl, &_cycle);
          setDoubleParam(ADAcquirePeriod, _cycle);
          setDoubleParam(ADAcquireTime, _exp);
          setParamStatus(ADAcquirePeriod, asynSuccess);
          setParamStatus(ADAcquireTime, asynSuccess);
        } else {
          setIntegerParam(ADAcquire, 0);
          setIntegerParam(ADStatus, ADStatusIdle);
        }
        setParamStatus(ADStatus, asynSuccess);
        setParamStatus(ADAcquire, asynSuccess);
      } else {
        setParamStatus(ADStatus, asynDisconnected);
        setParamStatus(ADAcquire, asynDisconnected);
        setParamStatus(ADAcquirePeriod, asynDisconnected);
        setParamStatus(ADAcquireTime, asynDisconnected);
      }
    }
    
    // Poll for the BIAS Settings
    
    float bias_voltage[CIN_CTL_NUM_BIAS];
    cin_status = cin_ctl_get_bias_voltages(&cin_ctl, bias_voltage, NULL);

    setDoubleParam(FastCCDBiasPosH, bias_voltage[CIN_CTL_BIAS_POSH]);
    setDoubleParam(FastCCDBiasNegH, bias_voltage[CIN_CTL_BIAS_NEGH]);
    setDoubleParam(FastCCDBiasPosRG, bias_voltage[CIN_CTL_BIAS_POSRG]);
    setDoubleParam(FastCCDBiasNegRG, bias_voltage[CIN_CTL_BIAS_NEGRG]);
    setDoubleParam(FastCCDBiasPosSW, bias_voltage[CIN_CTL_BIAS_POSSW]);
    setDoubleParam(FastCCDBiasNegSW, bias_voltage[CIN_CTL_BIAS_NEGSW]);
    setDoubleParam(FastCCDBiasPosV, bias_voltage[CIN_CTL_BIAS_POSV]);
    setDoubleParam(FastCCDBiasNegV, bias_voltage[CIN_CTL_BIAS_NEGV]);
    setDoubleParam(FastCCDBiasPosTG, bias_voltage[CIN_CTL_BIAS_POSTG]);
    setDoubleParam(FastCCDBiasNegTG, bias_voltage[CIN_CTL_BIAS_NEGTG]);
    setDoubleParam(FastCCDBiasPosVF, bias_voltage[CIN_CTL_BIAS_POSVF]);
    setDoubleParam(FastCCDBiasNegVF, bias_voltage[CIN_CTL_BIAS_NEGVF]);
    setDoubleParam(FastCCDBiasNEDGE, bias_voltage[CIN_CTL_BIAS_NEDGE]);
    setDoubleParam(FastCCDBiasOTG, bias_voltage[CIN_CTL_BIAS_OTG]);
    setDoubleParam(FastCCDBiasVDDR, bias_voltage[CIN_CTL_BIAS_VDDR]);
    setDoubleParam(FastCCDBiasVDDOut, bias_voltage[CIN_CTL_BIAS_VDD_OUT]);
    setDoubleParam(FastCCDBiasBufBase, bias_voltage[CIN_CTL_BIAS_BUF_BASE]);
    setDoubleParam(FastCCDBiasBufDelta, bias_voltage[CIN_CTL_BIAS_BUF_DELTA]);
    setDoubleParam(FastCCDBiasSpare1, bias_voltage[CIN_CTL_BIAS_SPARE1]);
    setDoubleParam(FastCCDBiasSpare2, bias_voltage[CIN_CTL_BIAS_SPARE2]);

    asynStatus _s = asynSuccess;
    if(cin_status){
      _s = asynDisconnected;
    }

    setParamStatus(FastCCDBiasPosH, _s);
    setParamStatus(FastCCDBiasNegH, _s);
    setParamStatus(FastCCDBiasPosRG, _s);
    setParamStatus(FastCCDBiasNegRG, _s);
    setParamStatus(FastCCDBiasPosSW, _s);
    setParamStatus(FastCCDBiasNegSW, _s);
    setParamStatus(FastCCDBiasPosV, _s);
    setParamStatus(FastCCDBiasNegV, _s);
    setParamStatus(FastCCDBiasPosTG, _s);
    setParamStatus(FastCCDBiasNegTG, _s);
    setParamStatus(FastCCDBiasPosVF, _s);
    setParamStatus(FastCCDBiasNegVF, _s);
    setParamStatus(FastCCDBiasNEDGE, _s);
    setParamStatus(FastCCDBiasOTG, _s);
    setParamStatus(FastCCDBiasVDDR, _s);
    setParamStatus(FastCCDBiasVDDOut, _s);
    setParamStatus(FastCCDBiasBufBase, _s);
    setParamStatus(FastCCDBiasBufDelta, _s);
    setParamStatus(FastCCDBiasSpare1, _s);
    setParamStatus(FastCCDBiasSpare2, _s);

  } else {
    // Set the Comm Error 
    
    setParamStatus(FastCCDBias, asynDisconnected);
    setParamStatus(FastCCDClocks, asynDisconnected);
    setParamStatus(FastCCDCameraPower, asynDisconnected);
    setParamStatus(FastCCDMux1, asynDisconnected);
    setParamStatus(FastCCDMux2, asynDisconnected);
    setParamStatus(FastCCDFclk, asynDisconnected);

    setParamStatus(FastCCDBiasPosH, asynDisconnected);
    setParamStatus(FastCCDBiasNegH, asynDisconnected);
    setParamStatus(FastCCDBiasPosRG, asynDisconnected);
    setParamStatus(FastCCDBiasNegRG, asynDisconnected);
    setParamStatus(FastCCDBiasPosSW, asynDisconnected);
    setParamStatus(FastCCDBiasNegSW, asynDisconnected);
    setParamStatus(FastCCDBiasPosV, asynDisconnected);
    setParamStatus(FastCCDBiasNegV, asynDisconnected);
    setParamStatus(FastCCDBiasPosTG, asynDisconnected);
    setParamStatus(FastCCDBiasNegTG, asynDisconnected);
    setParamStatus(FastCCDBiasPosVF, asynDisconnected);
    setParamStatus(FastCCDBiasNegVF, asynDisconnected);
    setParamStatus(FastCCDBiasNEDGE, asynDisconnected);
    setParamStatus(FastCCDBiasOTG, asynDisconnected);
    setParamStatus(FastCCDBiasVDDR, asynDisconnected);
    setParamStatus(FastCCDBiasVDDOut, asynDisconnected);
    setParamStatus(FastCCDBiasBufBase, asynDisconnected);
    setParamStatus(FastCCDBiasBufDelta, asynDisconnected);
    setParamStatus(FastCCDBiasSpare1, asynDisconnected);
    setParamStatus(FastCCDBiasSpare2, asynDisconnected);
  }
}

/**
 * Update status of detector. Meant to be run in own thread.
 */
void FastCCD::statusTask(void)
{
  unsigned int status = 0;
  double timeout = 0.0;
  static const char *functionName = "statusTask";
  int first = 1;

  while(1) {

    //Read timeout for polling freq.
    this->lock();
    timeout = statusPollingPeriod;
    this->unlock();

    if (timeout != 0.0) {
      status = epicsEventWaitWithTimeout(statusEvent, timeout);
    } else {
      status = epicsEventWait(statusEvent);
    }              
  
    if (status == epicsEventWaitOK) {
      asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
        "%s:%s: Got status event\n",
        driverName, functionName);
    }

    // Update the ticktock
    
    this->lock();
    setIntegerParam(FastCCDStatusHB, 1);
    callParamCallbacks();
    this->unlock();

    // Do a single poll of the detector
    getCameraStatus(first);
    if(first){
      first = 0;
    }

    /* Call the callbacks to update any changes */

    this->lock();
    setIntegerParam(FastCCDStatusHB, 0);
    callParamCallbacks();
    this->unlock();
        
  } //End of while loop

}

void timespec_diff(struct timespec *start, struct timespec *stop,
                   struct timespec *result)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }

    return;
}

void FastCCD::detectorWaitTask(void)
{
  unsigned int status = 0;
  static const char *functionName = "detectorWaitTask";
    
  while(1)
  {
    status = epicsEventWait(detectorWaitEvent);
    if (status == epicsEventWaitOK) {
      asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
                "%s:%s: Got status event\n",
                driverName, functionName);
    }

    double _p;
    getDoubleParam(ADAcquirePeriod, &_p);
    _p = _p * 2.5;
    for(;;)
    {
      struct timespec now, diff;
      clock_gettime(CLOCK_REALTIME, &now); 
      timespec_diff(&lastFrameTimestamp, &now, &diff);
      double _d = diff.tv_sec + ((double)(diff.tv_nsec) / 1000000000);
   
      usleep(1000);
   
     if((_d > _p) || (_d > 300))
     {
       break;
     }
    }
    
    this->lock();
    setIntegerParam(ADStatus, ADStatusIdle);
    setIntegerParam(ADAcquire, 0);
    callParamCallbacks();
    this->unlock();
  }
}

//C utility functions to tie in with EPICS

static void FastCCDStatusTaskC(void *drvPvt)
{
  FastCCD *pPvt = (FastCCD *)drvPvt;

  pPvt->statusTask();
}


static void FastCCDDataStatsTaskC(void *drvPvt)
{
  FastCCD *pPvt = (FastCCD *)drvPvt;

  pPvt->dataStatsTask();
}

static void FastCCDDetectorWaitTaskC(void *drvPvt)
{
  FastCCD *pPvt = (FastCCD *)drvPvt;

  pPvt->detectorWaitTask();
}

extern "C" {

/** IOC shell configuration command for debug output
  * \param[in] error (0 = no error output, 1 = error output to stderr)
  * \param[in] debug (0 = no debug output, 1 = debug output to stderr)
  */
void FastCCDDebug(int error, int debug)
{
  cin_set_debug_print(error);
  cin_set_error_print(debug);
}

/** IOC shell configuration command for FastCCD driver
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread
  * \param[in] stackSize The stack size for the asyn port driver thread
  * \param[in] packetBuffer The CINDATA packet buffer size
  * \param[in] imageBuffer The CINDATA image buffer size
  * \param[in] baseIP The base IP address
  * \param[in] fabricIP The fabric IP address
  * \param[in] fabricMAC The fabric MAC address
  */
int FastCCDConfig(const char *portName, int maxBuffers, size_t maxMemory, 
                  int priority, int stackSize, int packetBuffer, int imageBuffer,
				  const char *baseIP, const char *fabricIP, const char *fabricMAC)
{
  new FastCCD(portName, maxBuffers, maxMemory, priority, stackSize, packetBuffer, imageBuffer,
		      baseIP, fabricIP, fabricMAC);
  return(asynSuccess);
}

/* Code for iocsh registration */

/* FastCCDConfig */
static const iocshArg FastCCDConfigArg0 = {"Port name", iocshArgString};
static const iocshArg FastCCDConfigArg1 = {"maxBuffers", iocshArgInt};
static const iocshArg FastCCDConfigArg2 = {"maxMemory", iocshArgInt};
static const iocshArg FastCCDConfigArg3 = {"priority", iocshArgInt};
static const iocshArg FastCCDConfigArg4 = {"stackSize", iocshArgInt};
static const iocshArg FastCCDConfigArg5 = {"packetBuffer", iocshArgInt};
static const iocshArg FastCCDConfigArg6 = {"imageBuffer", iocshArgInt};
static const iocshArg FastCCDConfigArg7 = {"baseIP", iocshArgString};
static const iocshArg FastCCDConfigArg8 = {"fabricIP", iocshArgString};
static const iocshArg FastCCDConfigArg9 = {"fabricMAC", iocshArgString};
static const iocshArg * const FastCCDConfigArgs[] =  {&FastCCDConfigArg0,
                                                       &FastCCDConfigArg1,
                                                       &FastCCDConfigArg2,
                                                       &FastCCDConfigArg3,
                                                       &FastCCDConfigArg4,
                                                       &FastCCDConfigArg5,
                                                       &FastCCDConfigArg6,
                                                       &FastCCDConfigArg7,
                                                       &FastCCDConfigArg8,
                                                       &FastCCDConfigArg9};

static const iocshFuncDef configFastCCD = {"FastCCDConfig", 10, FastCCDConfigArgs};

static const iocshArg FastCCDDebugArg0 = {"error", iocshArgInt};
static const iocshArg FastCCDDebugArg1 = {"debug", iocshArgInt};
static const iocshArg * const FastCCDDebugArgs[] = {&FastCCDDebugArg0,
                                                    &FastCCDDebugArg1};
static const iocshFuncDef debugFastCCD = {"FastCCDDebug", 2, FastCCDDebugArgs};

static void configFastCCDCallFunc(const iocshArgBuf *args)
{
  FastCCDConfig(args[0].sval, args[1].ival, args[2].ival,
                args[3].ival, args[4].ival, args[5].ival,
                args[6].ival, args[7].sval, args[8].sval,
      				  args[9].sval);
}

static void debugFastCCDCallFunc(const iocshArgBuf *args)
{
  FastCCDDebug(args[0].ival, args[1].ival);
}

static void FastCCDRegister(void)
{
    iocshRegister(&configFastCCD, configFastCCDCallFunc);
    iocshRegister(&debugFastCCD, debugFastCCDCallFunc);
}

epicsExportRegistrar(FastCCDRegister);

} // extern "C"

