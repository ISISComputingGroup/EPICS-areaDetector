#ifndef ADPYTHONPLUGIN_H
#define ADPYTHONPLUGIN_H

#include "Python.h"
#include "NDPluginDriver.h"
#include <iocsh.h>
#include <epicsExport.h>

// Max number of user parameters in a subclass
#define NUSERPARAMS 100

// Number of characters in a big buffer
#define BIGBUFFER 10000

class adPythonPlugin : public NDPluginDriver {
public:
	adPythonPlugin(const char *portName, const char *filename,
                   const char *classname, int queueSize, int blockingCallbacks,
				   const char *NDArrayPort, int NDArrayAddr, int maxBuffers, size_t maxMemory,
				   int priority, int stackSize);
	~adPythonPlugin() {}
	/** This called once immediately after class instantiation */
	virtual void initThreads();
    /** This is called when the plugin gets a new array callback */
    virtual void processCallbacks(NDArray *pArray);
    /** This is when we get a new int value */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    /** This is when we get a new float value */
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    /** This is when we get a new string value */
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual);
        
protected:
    /** These are the values of our parameters */
    #define FIRST_ADPYTHONPLUGIN_PARAM adPythonFilename
    int adPythonFilename;
    int adPythonClassname;
    int adPythonLoad;
    int adPythonTime;
    int adPythonState;    
    #define LAST_ADPYTHONPLUGIN_PARAM adPythonState
    #define NUM_ADPYTHONPLUGIN_PARAMS (&LAST_ADPYTHONPLUGIN_PARAM - &FIRST_ADPYTHONPLUGIN_PARAM + 1 + NUSERPARAMS)
    int adPythonUserParams[NUSERPARAMS];

private:
    asynStatus importAdPythonModule();
    asynStatus makePyInst();
    asynStatus wrapArray(NDArray *pArray);  
    asynStatus interpretReturn(PyObject *pValue);
    asynStatus updateParamDict();
    asynStatus updateParamList(int atinit);
    asynStatus updateAttrDict(NDArray *pArray);
    asynStatus updateAttrList(NDArray *pArray);    
    asynStatus lookupNpyFormat(NDDataType_t ad_fmt, int *npy_fmt);
    asynStatus lookupAdFormat(int npy_fmt, NDDataType_t *ad_fmt);
    
    PyObject *pInstance, *pParams, *pProcessArray, *pParamChanged, *pMakePyInst, *pAttrs, *pProcessArgs;
    NDAttributeList *pFileAttributes;
    int nextParam, pluginState;
    epicsMutexId dictMutex;
    PyThreadState *threadState;
};

#endif
