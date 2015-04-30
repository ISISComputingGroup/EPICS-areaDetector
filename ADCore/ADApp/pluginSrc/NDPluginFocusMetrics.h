#pragma once

#include <epicsTypes.h>
#include "NDPluginDriver.h"


#define NDPluginFocusMetricsComputeFocusString  "COMPUTE_FOCUS_METRICS"// (asynInt32,   r/w) Compute focus metrics ? 
#define NDPluginFocusMetricsFocusMetricsString  "FOCUS_METRICS"        // (asynFloat64, r/o) Focus Metrics Value


/** Calculates Focus Metrics
  */
class NDPluginFocusMetrics : public NDPluginDriver {
public:
    NDPluginFocusMetrics(const char *portName, int queueSize, int blockingCallbacks, 
                         const char *NDArrayPort, int NDArrayAddr,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    
    template <typename epicsType> void doComputeFocusMetricsT(NDArray *pArray, double *pFocusMetrics);
    int doComputeFocusMetrics(NDArray *pArray, double *pFocusMetrics);
   
protected:
    int NDPluginFocusMetricsComputeFocus;
    #define FIRST_NDPLUGIN_FOCUS_METRICS_PARAM NDPluginFocusMetricsComputeFocus
    int NDPluginFocusMetricsFocusValue;
    #define LAST_NDPLUGIN_FOCUS_METRICS_PARAM NDPluginFocusMetricsFocusValue
                                
private:
};
#define NUM_NDPLUGIN_FOCUS_METRICS_PARAMS ((int)(&LAST_NDPLUGIN_FOCUS_METRICS_PARAM - &FIRST_NDPLUGIN_FOCUS_METRICS_PARAM + 1))
