/* NDFileJPEG.cpp
 * Writes NDArrays to FITS files.
 *
 * Gabriele Salvato
 * March, 2014
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netcdf.h>
#include <fstream>

#include <epicsStdio.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDPluginFile.h"
#include "NDFileFITS.h"


static const char *driverName = "NDFileFITS";

/** Opens a FITS file.
  * \param[in] fileName The name of the file to open.
  * \param[in] openMode Mask defining how the file should be opened; bits are 
  *            NDFileModeRead, NDFileModeWrite, NDFileModeAppend, NDFileModeMultiple
  * \param[in] pArray A pointer to an NDArray; this is used to determine the array and attribute properties.
  */
asynStatus 
NDFileFITS::openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray) {
  static const char *functionName = "openFile";
  
  // We don't support reading yet
  if (openMode & NDFileModeRead) return(asynError);

  // We don't support opening an existing file for appending yet
  if (openMode & NDFileModeAppend) return(asynError);
  
  // At present only 16 bits unsigned images are valid.
  if (pArray->dataType != NDUInt16) return(asynError);

  //>>>>>>>>>>>>>>if (FITSType != 0) return(asynError);

  fitsStatus = 0;
  if(fits_create_file(&fitsFilePtr, fileName, &fitsStatus)) {// create new FITS file
    // get the error description
    fits_get_errstatus(fitsStatus, fits_status_str);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s error opening file %s error=%s\n",
      driverName, functionName, fileName, fits_status_str);
    fits_clear_errmsg();
    return (asynError);
  }

  return(asynSuccess);
}


/** Writes single NDArray to the FITS file.
  * \param[in] pArray Pointer to the NDArray to be written
  */
asynStatus 
NDFileFITS::writeFile(NDArray *pArray) {

  static const char *functionName = "writeFile";

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
            "%s:%s: %lu, %lu\n", 
            driverName, functionName, (unsigned long)pArray->dims[0].size, (unsigned long)pArray->dims[1].size);

  NDArrayInfo arrayInfo;
  pArray->getInfo(&arrayInfo);
  int nx = (int) arrayInfo.xSize;
  int ny = (int) arrayInfo.ySize;
  long naxis    = 2;         // 2-dimensional image
  long naxes[2] = { nx, ny };

  fitsStatus = 0;
  if(fits_create_img(fitsFilePtr, USHORT_IMG, naxis, naxes, &fitsStatus)) {
    fits_get_errstatus(fitsStatus, fits_status_str);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s error creating fits image: error=%s\n",
      driverName, functionName, fits_status_str);
    fits_clear_errmsg();
    return asynError;
  }
  
  if(fits_write_img(fitsFilePtr, CFITSIO_TUSHORT, 1, naxes[0]*naxes[1], pArray->pData, &fitsStatus)) {
    fits_get_errstatus(fitsStatus, fits_status_str);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s error writing fits image: error=%s\n",
      driverName, functionName, fits_status_str);
    fits_close_file(fitsFilePtr, &fitsStatus);
    fits_clear_errmsg();
    return asynError;
  }
  
  if(WriteKeys(fitsFilePtr, &fitsStatus)) {
    fits_get_errstatus(fitsStatus, fits_status_str);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s error writing fits image: error=%s\n",
      driverName, functionName, fits_status_str);
    fits_close_file(fitsFilePtr, &fitsStatus);
    return asynError;
  }
      
  return(asynSuccess);
}


/** Reads single NDArray from a FITS file; NOT CURRENTLY IMPLEMENTED.
  * \param[in] pArray Pointer to the NDArray to be read
  */
asynStatus 
NDFileFITS::readFile(NDArray **pArray) {
  //static const char *functionName = "readFile";
  return asynError;
}


/** Closes the FITS file. */
asynStatus 
NDFileFITS::closeFile() {
  static const char *functionName = "closeFile";
  if(fits_close_file(fitsFilePtr, &fitsStatus)) {
    fits_get_errstatus(fitsStatus, fits_status_str);
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s error closing fits image file: error=%s\n",
      driverName, functionName, fits_status_str);
    fits_clear_errmsg();
    return asynError;
  }
  return asynSuccess;
}


asynStatus
NDFileFITS::WriteKeys(fitsfile *fitsFilePtr, int* fitsStatus) {
  *fitsStatus = 0;
  std::ifstream fHeader;
  char filePath[MAX_PATH] = {0};
  *fitsStatus = getStringParam(NDFITSFileHeaderFullPathname, sizeof(filePath), filePath); 

  if (*fitsStatus) return asynSuccess;
  fHeader.open(filePath, std::ios_base::in);
  if(fHeader.fail()) 
    return asynSuccess; // If the file does not exists there is nothing to add
  
  char lineBuf[256], keyword[FLEN_KEYWORD], value[FLEN_VALUE], comment[FLEN_COMMENT];
  char *pToken;
  char *context	= NULL;
  
  while (fHeader.getline(lineBuf, sizeof(lineBuf), '\n')) {
    if (strstr(lineBuf, "//")) continue;// It is a comment
    pToken= strtok_s(lineBuf, "\n\t ", &context);
    if (!pToken) continue;// It is an empty line
    strncpy_s(keyword, sizeof(keyword), pToken, _TRUNCATE);
    pToken= strtok_s(0, "\n\t ", &context);
    if (!pToken) continue;// No value specified.. skip entire line
    strncpy_s(value, sizeof(value), pToken, _TRUNCATE);
    pToken= strtok_s(0, "\n", &context);
    if (pToken)
      strncpy_s(comment, sizeof(comment), pToken, _TRUNCATE);
    else
      strncpy_s(comment, sizeof(comment), "", _TRUNCATE);
      
	  fits_update_key(fitsFilePtr, CFITSIO_TSTRING, keyword, value, comment, fitsStatus);
  }
  
  fits_close_file(fitsFilePtr, fitsStatus); // close the fits file
  fHeader.close();
  if(*fitsStatus) return asynError;
  return asynSuccess;  
}



/** Constructor for NDFileFITS; all parameters are simply passed to NDPluginFile::NDPluginFile.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when 
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDFileFITS::NDFileFITS(const char *portName, int queueSize, int blockingCallbacks,
                       const char *NDArrayPort, int NDArrayAddr,
                       int priority, int stackSize)
    /* Invoke the base class constructor.
     * We allocate 2 NDArrays of unlimited size in the NDArray pool.
     * This driver can block (because writing a file can be slow), and it is not multi-device.  
     * Set autoconnect to 1.  priority and stacksize can be 0, which will use defaults. */
    : NDPluginFile(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_NDFILE_FITS_PARAMS,
                   2, 0, asynGenericPointerMask, asynGenericPointerMask, 
                   ASYN_CANBLOCK, 1, priority, stackSize)
{
    //static const char *functionName = "NDFileFITS";
    createParam(NDFITSFileHeaderFullPathnameString,   asynParamOctet,   &NDFITSFileHeaderFullPathname);
    
    // Set the plugin type string
    setStringParam(NDPluginDriverPluginType, "NDFileFITS");
    
    setStringParam(NDFITSFileHeaderFullPathname, ".\\FitsHeaderParameters.txt"); 
}


// Configuration routine.  Called directly, or from the iocsh
extern "C" int NDFileFITSConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                   const char *NDArrayPort, int NDArrayAddr,
                                   int priority, int stackSize)
{
    new NDFileFITS(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                   priority, stackSize);
    return(asynSuccess);
}


// EPICS iocsh shell commands

static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArray Port",iocshArgString};
static const iocshArg initArg4 = { "NDArray Addr",iocshArgInt};
static const iocshArg initArg5 = { "priority",iocshArgInt};
static const iocshArg initArg6 = { "stack size",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6};
static const iocshFuncDef initFuncDef = {"NDFileFITSConfigure", 7, initArgs};
static void
initCallFunc(const iocshArgBuf *args) {
    NDFileFITSConfigure(args[0].sval, args[1].ival, args[2].ival, args[3].sval, args[4].ival, args[5].ival, args[6].ival);
}

extern "C" void 
NDFileFITSRegister(void) {
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDFileFITSRegister);
}
