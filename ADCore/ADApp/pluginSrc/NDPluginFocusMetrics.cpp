/*
 * NDPluginFocusMetrics.cpp
 *
 * Focus-metrics calculation plugin
 * Author: Gabriele Salvato
 *
 * Created October 19, 2014
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include <asynDriver.h>

#include <epicsExport.h>
#include "NDPluginDriver.h"
#include "NDPluginFocusMetrics.h"

static const char *driverName="NDPluginFocusMetrics";

/*
 * Focus Metrics Contrast Based. See: 
 * Xu et al.
 * Robust Automatic Focus Algorithm for Low Contrast Images Using a New Contrast Measure
 * Sensors 2011, 11, 8281 8294
*/
template <typename epicsType>
void 
NDPluginFocusMetrics::doComputeFocusMetricsT(NDArray *pArray, double *pFocusMetrics) {
    size_t n_rows, n_columns;
    epicsType *pData = (epicsType *)pArray->pData;
    NDArrayInfo arrayInfo;
    double gXY; 
    epicsType *pRow, *pRowM1, *pRowP1;

    this->unlock();
    *pFocusMetrics = 0.0;
    pArray->getInfo(&arrayInfo);
    n_rows    = arrayInfo.xSize;// Rows of the image
    n_columns = arrayInfo.ySize;// Columns of the image

    for(size_t j=1; j<n_rows-2; j++) {
      pRowM1 = pData+(j-1)*n_columns;
      pRow   = pData+j    *n_columns;
      pRowP1 = pData+(j+1)*n_columns;

      for(size_t k=1; k<n_columns-2; k++) {
        gXY  = fabs(static_cast<double>(*(pRow+k)-(*(pRow+k-1))));
        gXY += fabs(static_cast<double>(*(pRow+k)-(*(pRow+k+1))));
        gXY += fabs(static_cast<double>(*(pRow+k)-(*(pRowM1+k))));
        gXY += fabs(static_cast<double>(*(pRow+k)-(*(pRowP1+k))));
        *pFocusMetrics += gXY*gXY;
      }
    }
    this->lock();
}

int
NDPluginFocusMetrics::doComputeFocusMetrics(NDArray *pArray, double *pFocusMetrics) {

    switch(pArray->dataType) {
        case NDInt8:
            doComputeFocusMetricsT<epicsInt8>(pArray, pFocusMetrics);
            break;
        case NDUInt8:
            doComputeFocusMetricsT<epicsUInt8>(pArray, pFocusMetrics);
            break;
        case NDInt16:
            doComputeFocusMetricsT<epicsInt16>(pArray, pFocusMetrics);
            break;
        case NDUInt16:
            doComputeFocusMetricsT<epicsUInt16>(pArray, pFocusMetrics);
            break;
        case NDInt32:
            doComputeFocusMetricsT<epicsInt32>(pArray, pFocusMetrics);
            break;
        case NDUInt32:
            doComputeFocusMetricsT<epicsUInt32>(pArray, pFocusMetrics);
            break;
        case NDFloat32:
            doComputeFocusMetricsT<epicsFloat32>(pArray, pFocusMetrics);
            break;
        case NDFloat64:
            doComputeFocusMetricsT<epicsFloat64>(pArray, pFocusMetrics);
            break;
        default:
            return(ND_ERROR);
        break;
    }
    return(ND_SUCCESS);
}


/** Callback function that is called by the NDArray driver with new NDArray data.
  * Does image Focus Metrics calculation.
  * \param[in] pArray  The NDArray from the callback.
  */
void
NDPluginFocusMetrics::processCallbacks(NDArray *pArray) {
    /* 
	 * This function calculates the focus metrics of a given image.
     * It is called with the mutex already locked. 
	 * It unlocks it during long calculations when private
     * structures don't need to be protected.
     */
    double focusMetrics, *pFocusMetrics = &focusMetrics;
    int computeFocusMetrics;
    NDArrayInfo arrayInfo;
    static const char* functionName = "processCallbacks";

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);
    
    getIntegerParam(NDPluginFocusMetricsComputeFocus,  &computeFocusMetrics);

    if (computeFocusMetrics) {
        pArray->getInfo(&arrayInfo);
        doComputeFocusMetrics(pArray, pFocusMetrics);
        setDoubleParam(NDPluginFocusMetricsFocusValue, focusMetrics);
        printf("Focus Value= %f", focusMetrics);
        asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER,
            (char *)pArray->pData, arrayInfo.totalBytes,
            "%s:%s focus Value=%f",
            driverName, functionName, focusMetrics);
    }
    callParamCallbacks();
}


/** Called when asyn clients call pasynFloat64->write().
  * This function performs actions for some parameters.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus
NDPluginFocusMetrics::writeFloat64(asynUser *pasynUser, epicsFloat64 value) {
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;
    static const char *functionName = "writeFloat64";

    /* Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status = setDoubleParam(function, value);

    /* If this parameter belongs to a base class call its method */
    if (function < FIRST_NDPLUGIN_FOCUS_METRICS_PARAM) 
      status = NDPluginDriver::writeFloat64(pasynUser, value);

    /* Do callbacks so higher layers see any changes */
    callParamCallbacks();
    if (status) 
        asynPrint(pasynUser, ASYN_TRACE_ERROR, 
              "%s:%s: error, status=%d function=%d, value=%f\n", 
              driverName, functionName, status, function, value);
    else        
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
              "%s:%s: function=%d, value=%f\n", 
              driverName, functionName, function, value);
    return status;
}



/** Constructor for NDPluginFocusMetrics; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * After calling the base class constructor this method sets reasonable default values for all of the
  * parameters.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginFocusMetrics::NDPluginFocusMetrics(
                         const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_NDPLUGIN_FOCUS_METRICS_PARAMS, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   0, 1, priority, stackSize)
{
    //static const char *functionName = "NDPluginStats";
    
    /* Focus Metrics */
    createParam(NDPluginFocusMetricsComputeFocusString, asynParamInt32,      &NDPluginFocusMetricsComputeFocus);
    createParam(NDPluginFocusMetricsFocusMetricsString, asynParamFloat64,    &NDPluginFocusMetricsFocusValue);

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginFocusMetrics");

    /* Try to connect to the array port */
    connectToArrayPort();
}

/** Configuration command */
extern "C" int NDFocusMetricsConfigure(
                                 const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
    new NDPluginFocusMetrics(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                             maxBuffers, maxMemory, priority, stackSize);
    return(asynSuccess);
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg6 = { "maxMemory",iocshArgInt};
static const iocshArg initArg7 = { "priority",iocshArgInt};
static const iocshArg initArg8 = { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8};
                                            
static const iocshFuncDef initFuncDef = {"NDFocusMetricsConfigure", 9, initArgs};

static void initCallFunc(const iocshArgBuf *args)
{
    NDFocusMetricsConfigure(args[0].sval, args[1].ival, args[2].ival,
                            args[3].sval, args[4].ival, args[5].ival,
                            args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void NDFocusMetricsRegister(void)
{
    iocshRegister(&initFuncDef, initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDFocusMetricsRegister);
}
