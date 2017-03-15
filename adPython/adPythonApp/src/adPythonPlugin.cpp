/* Nasty nasty hack so that Python.h is happy. */
#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE
#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 600
#include <Python.h>
#define PYTHON_USE_NUMPY
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include "numpy/ndarrayobject.h"
#include <stdio.h>
//#include <libgen.h>
#include <epicsTime.h>
#include "NDArray.h"
#include "adPythonPlugin.h"

// User plugin and adPythonPlugin.py working
#define GOOD 0
// User plugin not working
#define BAD 1
// adPythonPlugin.py not working
#define UGLY 2

// Some macros to set an error state and print a message
#define NoGood(errString, st) {                         \
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,    \
        "%s:%s: " errString "\n",                       \
        driverName, __func__);                          \
    this->pluginState = st;                             \
    setIntegerParam(adPythonState, this->pluginState);  \
    callParamCallbacks();                               \
    PyErr_Clear();                                      \
    return asynError;                                   \
}
#define Bad(errString) NoGood(errString, BAD)
#define Ugly(errString) NoGood(errString, UGLY)

// Used in error printing
const char *driverName = "adPythonPlugin";

// This holds the threadState of the thread that initialised python
// We need it to get a handle on the interpreter so create a threadState
// object for each port
static PyThreadState *mainThreadState = NULL;

adPythonPlugin::adPythonPlugin(const char *portNameArg, const char *filename,
                   const char *classname, int queueSize, int blockingCallbacks,
                   const char *NDArrayPort, int NDArrayAddr, int maxBuffers, 
                   size_t maxMemory, int priority, int stackSize)
    : NDPluginDriver(portNameArg, queueSize, blockingCallbacks,
                       NDArrayPort, NDArrayAddr, 1, NUM_ADPYTHONPLUGIN_PARAMS, 
                       maxBuffers, maxMemory,
                       asynGenericPointerMask|asynFloat64ArrayMask,
                       asynGenericPointerMask|asynFloat64ArrayMask,
                       ASYN_MULTIDEVICE, 1, priority, stackSize) 
{
    // Initialise some params
    this->pInstance = NULL;
    this->pParams = NULL;    
    this->pProcessArray = NULL;
    this->pParamChanged = NULL;
    this->pMakePyInst = NULL;
    this->pAttrs = NULL;
    this->pProcessArgs = NULL;
    this->nextParam = 0;
    this->pluginState = 0;
    this->pFileAttributes = new NDAttributeList;
   
    // Create the base class parameters (our python class may make some more)
    setStringParam(NDPluginDriverPluginType, driverName);
    createParam("ADPYTHON_FILENAME",   asynParamOctet,   &adPythonFilename);
    setStringParam(adPythonFilename,   filename);
    createParam("ADPYTHON_CLASSNAME",  asynParamOctet,   &adPythonClassname);
    setStringParam(adPythonClassname,  classname);
    createParam("ADPYTHON_LOAD",       asynParamInt32,   &adPythonLoad);
    createParam("ADPYTHON_TIME",       asynParamFloat64, &adPythonTime);    
    createParam("ADPYTHON_STATE",      asynParamInt32,   &adPythonState);        

    // First we tell python where to find adPythonPlugin.py
    char buffer[BIGBUFFER];
    snprintf(buffer, sizeof(buffer), "PYTHONPATH=%s", DATADIR);
    putenv(buffer);
    
    // Now we initialise python
    if (!Py_IsInitialized()) {
        PyEval_InitThreads();
        Py_Initialize();
    
        // Be sure to save thread state to release the GIL and give a handle
        // on the interpreter to this and other ports
        mainThreadState = PyEval_SaveThread();
    }
    
    // Create a thread state just for us
    this->threadState = PyThreadState_New(mainThreadState->interp);
    PyEval_RestoreThread(this->threadState);
    
    // Import our supporting library
    this->importAdPythonModule();

    // Try and make an instance of this
    if (this->makePyInst()) {
        // We failed, so our params are unusable
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s: User import failed, param lib unusable\n",
            driverName, __func__);
        this->pluginState = UGLY;
        setIntegerParam(adPythonState, this->pluginState);
        callParamCallbacks();
    } else {
        // Update param list from dict, also creating keys
        this->updateParamList(1);
    }
    
    // Release the GIL and finish
    this->threadState = PyEval_SaveThread();
}

/** Callback function that is called by the NDArray driver with new NDArray data
  * Calls the relevant user python code, stamps any outgoing array with
  * attributes from the code, then updates the internal param list from the user
  * param dict before returning any outgoing numpy array as an NDArray.
  * \param[in] pArray  The NDArray from the callback.
  * 
  * Called with this->lock taken
  */
void adPythonPlugin::processCallbacks(NDArray *pArray) {
    // First call the base class method
    NDPluginDriver::processCallbacks(pArray);

    // Make sure we are in a good state, otherwise do nothing
    if (this->pluginState != GOOD) return;

    // We have to modify our python dict to match our param list
    // Note: to avoid deadlocks we should always take locks in order:
    //  dictMutex, then GIL, then this->lock
    // so unlock here to preserve this order
    this->unlock();
    epicsMutexLock(this->dictMutex);

    // Make sure we're allowed to use the python API
    PyEval_RestoreThread(this->threadState);
    this->lock(); 

    // Store the time at the beginning of processing for profiling 
    epicsTimeStamp start, end;
    epicsTimeGetCurrent(&start);
       
    // Update the attribute dict
    this->updateAttrDict(pArray);        
       
    // Wrap the NDArray as a python tuple
    this->wrapArray(pArray);
       
    // Unlock for long call
    this->unlock();
    
    // Make the function call    
    PyObject *pValue = PyObject_CallObject(this->pProcessArray, this->pProcessArgs);

    // Lock back up
    this->lock();

    // interpret the return value of the python call
    this->interpretReturn(pValue);
    Py_XDECREF(pValue);

    // copy everything except the data, e.g. uniqueId and timeStamp, attributes
    if (this->pArrays[0]) this->pNDArrayPool->copy(pArray, this->pArrays[0], 0);

    // update param list, this will callParamCallbacks at the end
    this->updateParamList(0);    

    // update the attribute list
    this->updateAttrList();

    // release GIL and dict Mutex    
    this->threadState = PyEval_SaveThread();
    epicsMutexUnlock(this->dictMutex);
    
    // timestamp
    epicsTimeGetCurrent(&end);
    setDoubleParam(adPythonTime, epicsTimeDiffInSeconds(&end, &start)*1000);
    callParamCallbacks();    
    
    // Spit out the array
    if (this->pArrays[0]) {
        this->unlock();
        doCallbacksGenericPointer(this->pArrays[0], NDArrayData, 0);
        this->lock();    
    }
}

/** Called when asyn clients call pasynInt32->write().
  * This function does a write of a user parameter to the param list, then
  * updates the user param dict to match. It also reloads the user python lib
  * if requested.
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. 
  * 
  * Called with this->lock taken
  */
asynStatus adPythonPlugin::writeInt32(asynUser *pasynUser, epicsInt32 value) {
    int status = asynSuccess;
    int param = pasynUser->reason;
    if (param == adPythonLoad || 
            (this->nextParam && param >= adPythonUserParams[0])) {
        // We have to modify our python dict to match our param list
        // Note: to avoid deadlocks we should always take locks in order:
        //  dictMutex, then GIL, then this->lock
        // so unlock here to preserve this order
        this->unlock();
        epicsMutexLock(this->dictMutex);
        // Make sure we're allowed to use the python API
        PyEval_RestoreThread(this->threadState);
        // Now call the bast class to write the value to the param list
        this->lock();        
        status |= NDPluginDriver::writeInt32(pasynUser, value);               
        if (param == adPythonLoad) {
            // signal that we have started loading the python instance
            callParamCallbacks();
            // reload our python instance, this does callParamCallbacks for is
            status |= setIntegerParam(param, 0);
            status |= this->makePyInst();
        } else {
            // our param lib has changed, so update the dict and reprocess
            status |= this->updateParamDict();
        }
        // release GIL and dict Mutex
        this->threadState = PyEval_SaveThread();
        epicsMutexUnlock(this->dictMutex);    
    } else {
        status |= NDPluginDriver::writeInt32(pasynUser, value);
    }
    return (asynStatus) status;
}

/** Called when asyn clients call pasynFloat64->write().
  * This function does a write of a user parameter to the param list, then
  * updates the user param dict to match.
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. 
  * 
  * Called with this->lock taken
  */
asynStatus adPythonPlugin::writeFloat64(asynUser *pasynUser, 
                                        epicsFloat64 value) {
    int status = asynSuccess;
    int param = pasynUser->reason;
    if (this->nextParam && param >= adPythonUserParams[0]) {
        // We have to modify our python dict to match our param list
        // Note: to avoid deadlocks we should always take locks in order:
        //  dictMutex, then GIL, then this->lock
        // so unlock here to preserve this order
        this->unlock();
        epicsMutexLock(this->dictMutex);
        // Make sure we're allowed to use the python API
        PyEval_RestoreThread(this->threadState);
        // Now call the bast class to write the value to the param list
        this->lock();        
        status |= NDPluginDriver::writeFloat64(pasynUser, value);                
        // our param lib has changed, so update the dict and reprocess
        status |= this->updateParamDict();
        // release GIL and dict Mutex
        this->threadState = PyEval_SaveThread();
        epicsMutexUnlock(this->dictMutex);                        
    } else {
        status = NDPluginDriver::writeFloat64(pasynUser, value);
    }
    return (asynStatus) status;
}

/** Called when asyn clients call pasynOctet->write().
  * This function does a write of a user parameter to the param list, then
  * updates the user param dict to match.
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Address of the string to write.
  * \param[in] maxChars Max number of characters to write.
  * \param[out] nActual Number of characters actually written.
  * 
  * Called with this->lock taken
  */  
asynStatus adPythonPlugin::writeOctet(asynUser *pasynUser, const char *value, 
                                      size_t maxChars, size_t *nActual) {
    int status = asynSuccess;
    int param = pasynUser->reason;
    if (this->nextParam && param >= adPythonUserParams[0]) {
        // We have to modify our python dict to match our param list
        // Note: to avoid deadlocks we should always take locks in order:
        //  dictMutex, then GIL, then this->lock
        // so unlock here to preserve this order
        this->unlock();
        epicsMutexLock(this->dictMutex);
        // Make sure we're allowed to use the python API
        PyEval_RestoreThread(this->threadState);
        // Now call the bast class to write the value to the param list
        this->lock();        
        status |= NDPluginDriver::writeOctet(pasynUser, value, maxChars, nActual);                
        // our param lib has changed, so update the dict and reprocess
        status |= this->updateParamDict();
        // release GIL and dict Mutex
        this->threadState = PyEval_SaveThread();
        epicsMutexUnlock(this->dictMutex);       
    } else {
        status |= NDPluginDriver::writeOctet(pasynUser, value, maxChars, nActual);
    }
    return (asynStatus) status;
}

/** Called once when the module is initialised. Imports the python support
  * lib and creates dictMutex. If this fails it is not recoverable from.
  * 
  * Called with GIL taken, this->lock taken.
  */ 
asynStatus adPythonPlugin::importAdPythonModule() {
    // Create the epicsMutex for locking access to our param dict
    this->dictMutex = epicsMutexCreate();
    if (this->dictMutex == NULL) Ugly("epicsMutexCreate failure");
    
    // Import the adPython module
    PyObject *pAdPython = PyImport_ImportModule("adPythonPlugin");
    if (pAdPython == NULL) Ugly("Can't import adPythonPlugin");
    
    // Try and init numpy, needs to be done here as adPythonPlugin might have
    // to put it on our path
    _import_array();

    // Get the reference for the makePyInst python function
    this->pMakePyInst = PyObject_GetAttrString(pAdPython, "makePyInst");    
    Py_DECREF(pAdPython);
    if (this->pMakePyInst == NULL) Ugly("Can't get makePyInst ref");

    return asynSuccess;   
}

/** Called when the module is initialised and when the user clicks the ReadFile
  * button. Imports the user python file and makes an instance of the class
  * 
  * Called with dictMutex taken, GIL taken, this->lock taken.
  */ 
asynStatus adPythonPlugin::makePyInst() {     
    char filename[BIGBUFFER], classname[BIGBUFFER];
    
    // If adPython module has failed to load we can't do anything
    if (this->pluginState == UGLY) 
        Ugly("Can't load user python class as initial load already failed");
    
    // Get the filename from param lib
    if (getStringParam(adPythonFilename, BIGBUFFER, filename)) 
        Bad("Can't get filename");
    
    // Get the classname from param lib
    if (getStringParam(adPythonClassname, BIGBUFFER, classname))
        Bad("Can't get classname");

    // Make tuple for makePyInst
    PyObject *pArgs = Py_BuildValue("sss", this->portName, filename, classname);
    if (pArgs == NULL) Bad("Can't build tuple for makePyInst()");
           
    // Create instance of this class, freeing the old one if it exists
    Py_XDECREF(this->pInstance);
    this->pInstance = PyObject_CallObject(this->pMakePyInst, pArgs);
    Py_DECREF(pArgs);
    if (this->pInstance == NULL) Bad("Can't make instance of class");

    // Get the processArray function ref
    Py_XDECREF(this->pProcessArray);
    this->pProcessArray = PyObject_GetAttrString(this->pInstance, "_processArray");
    if (this->pProcessArray == NULL) Bad("Can't get processArray ref");
    
    // Get the paramChanged function ref
    Py_XDECREF(this->pParamChanged);
    this->pParamChanged = PyObject_GetAttrString(this->pInstance, "_paramChanged");
    if (this->pParamChanged == NULL) Bad("Can't get paramChanged ref");
    
    // Get the param dict ref
    Py_XDECREF(this->pParams);
    this->pParams = PyObject_GetAttrString(this->pInstance, "_params");
    if (this->pParams == NULL) Bad("Can't get _params ref");
    
    // Can reset state now
    this->pluginState = GOOD;
    setIntegerParam(adPythonState, this->pluginState);
    callParamCallbacks();
    return asynSuccess; 
}

/** Called by processCallbacks(). Creates a numpy array from an NDArray,
  * then wraps it and the pAttrs dictionary in a tuple ready to pass to the
  * user class.
  * \param[in] pArray NDArray structure to wrap
  * 
  * Called with dictMutex taken, GIL taken, this->lock taken.
  */ 
asynStatus adPythonPlugin::wrapArray(NDArray *pArray) {      
    // Return if we aren't good
    if (this->pluginState != GOOD) return asynError;

    // Return if no array to operate on
    if (pArray == NULL) return asynError;
        
    // Release the last produced array
    if (this->pArrays[0]) {
        this->pArrays[0]->release();    
        this->pArrays[0] = NULL;
    }
       
    // Create a dimension description for numpy, note that we reverse dims
    npy_intp npy_dims[ND_ARRAY_MAX_DIMS];
    for (int i=0; i<pArray->ndims; i++) {        
        npy_dims[i] = pArray->dims[pArray->ndims-i-1].size;
    }
    
    // Lookup the numpy format from the ad dataType of the array
    int npy_fmt;
    if (lookupNpyFormat(pArray->dataType, &npy_fmt))
        Bad("Can't lookup numpy format for dataType");
        
    // Wrap the existing data from the NDArray in a numpy array
    // When created, this is writable, but the python layer sets it readonly
    PyObject* pValue = PyArray_SimpleNewFromData(pArray->ndims, npy_dims, 
        npy_fmt, pArray->pData);   
    if (pValue == NULL) Bad("Cannot make numpy array");
    
    // Construct argument list, don't increment pValue so it is destroyed with
    // pProcessArgs
    Py_XDECREF(this->pProcessArgs);
    this->pProcessArgs = Py_BuildValue("(NO)", pValue, this->pAttrs);
    if (this->pProcessArgs == NULL) {
        Py_DECREF(pValue);    
        Bad("Cannot build tuple for processArray()");
    }
    return asynSuccess;
}

/** Called by processCallbacks(). Creates an NDArray from the return value
  * of a python class.
  * \param[in] pValue PyObject which will be wrapped if it is a numpy array
  * 
  * Called with dictMutex taken, GIL taken, this->lock taken.
  */ 
asynStatus adPythonPlugin::interpretReturn(PyObject *pValue) {     
    // Return if we aren't good
    if (this->pluginState != GOOD) return asynError;

    // Check return value for existance    
    if (pValue == NULL) Bad("processArray() call failed");
           
    // If it wasn't an array, just return here
    if (!PyObject_IsInstance(pValue, (PyObject*) (&PyArray_Type))) {
        return asynSuccess;
    }
    
    // We must have an array, so find the dataType from it
    PyArrayObject *pArrayOb = (PyArrayObject *) pValue;
    NDDataType_t ad_fmt;
    if (lookupAdFormat(PyArray_TYPE(pArrayOb), &ad_fmt)) 
        Bad("Can't lookup numpy format for dataType");
    
    // Create a dimension description from numpy array, note the inverse order
    size_t ad_dims[ND_ARRAY_MAX_DIMS];
    for (int i=0; i<PyArray_NDIM(pArrayOb); i++) {
        ad_dims[i] = PyArray_DIMS(pArrayOb)[PyArray_NDIM(pArrayOb)-i-1];
    }
    
    /* Allocate the array */
    this->pArrays[0] = pNDArrayPool->alloc(PyArray_NDIM(pArrayOb), ad_dims, 
        ad_fmt, 0, NULL);
    if (this->pArrays[0] == NULL) Bad("Error allocating buffer");

    // TODO: could avoid this memcpy if we could pass an existing
    // buffer to NDArray *AND* have it call a user free function
    NDArrayInfo arrayInfo;    
    this->pArrays[0]->getInfo(&arrayInfo);
    memcpy(this->pArrays[0]->pData, PyArray_DATA(pArrayOb), arrayInfo.totalBytes);              
    return asynSuccess; 
}           

/** Called by writexxx(). Updates the python param dict with values from the
  * param list.
  * 
  * Called with dictMutex taken, GIL taken, this->lock taken.
  */ 
// TODO: we should make a python wrapper to the param list that handles the
// param getting and setting better. The only issue is that user python code is
// currently called without this->lock(). Probably not an issue to add it in,
// just needs thinking about
asynStatus adPythonPlugin::updateParamDict() { 
    // Return if we aren't all good
    if (this->pluginState != GOOD) return asynError;

    // Create param key list
    PyObject *pKeys = PyDict_Keys(this->pParams);
    if (pKeys == NULL) Bad("Can't get keys of _param dict\n");
    
    // Create a param of the correct type for each item
    for (Py_ssize_t i=0; i<PyList_Size(pKeys); i++) {
        int param;
        PyObject *key = PyList_GetItem(pKeys, i);
        PyObject *keyStr = PyObject_Str(key);
        char *paramStr = PyString_AsString(keyStr);
        if (findParam(paramStr, &param)) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: can't find param %s\n",
                driverName, __func__, paramStr);    
            continue;        
        }
        PyObject *pValue = PyDict_GetItem(this->pParams, key);
        if (PyFloat_Check(pValue)) {
            // get float param
            double value;
            getDoubleParam(param, &value);
            PyDict_SetItem(this->pParams, key, PyFloat_FromDouble(value));
        } else if (PyInt_Check(pValue)) {
            // get int param
            int value;
            getIntegerParam(param, &value);
            PyDict_SetItem(this->pParams, key, PyInt_FromLong(value));
        } else if (PyString_Check(pValue)) {
            // get string param
            char value[BIGBUFFER];
            getStringParam(param, BIGBUFFER, value);
            PyDict_SetItem(this->pParams, key, PyString_FromString(value));
        } else {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: param %s is not an int, float or string\n",
                driverName, __func__, paramStr);            
        }
    }
    Py_DECREF(pKeys);

    // call paramChanged method
    PyObject *pRet = PyObject_CallObject(this->pParamChanged, NULL);
    if (pRet == NULL) Bad("Calling paramChanged failed\n");
    
    // Update any parameters that may have changed in the paramChanged callback to python
    this->updateParamList(0);

    return asynSuccess;
}

/** Called after any user python call (in makePyInst(), updateParamList() or
  * processCallbacks()). Updates the internal param list with values from
  * the python param dict.
  * \param[in] atinit If 1 then create params, otherwise just update their vals
  * 
  * Called with dictMutex taken, GIL taken, this->lock taken.
  */ 
asynStatus adPythonPlugin::updateParamList(int atinit) { 
    // Return if we aren't all good
    if (this->pluginState != GOOD) return asynError;
    
    // Create param key list
    PyObject *pKeys = PyDict_Keys(this->pParams);
    if (pKeys == NULL) Bad("Can't get keys of _param dict\n");
    
    // Create a param of the correct type for each item
    for (Py_ssize_t i=0; i<PyList_Size(pKeys); i++) {
        int param;
        PyObject *key = PyList_GetItem(pKeys, i);
        PyObject *keyStr = PyObject_Str(key);
        char *paramStr = PyString_AsString(keyStr);
        // If not at init, then find the param
        if (!atinit) {
            if (findParam(paramStr, &param)) {
                asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                    "%s:%s: can't find param %s\n",
                    driverName, __func__, paramStr);    
             continue;        
            }
        }
        PyObject *value = PyDict_GetItem(this->pParams, key);
        if (PyFloat_Check(value)) {
            if (atinit) {
                createParam(paramStr, asynParamFloat64, &adPythonUserParams[this->nextParam]);
                param = adPythonUserParams[this->nextParam++];
            }
            // set float param
            setDoubleParam(param, PyFloat_AsDouble(value));
        } else if (PyInt_Check(value)) {
            if (atinit) {
                createParam(paramStr, asynParamInt32, &adPythonUserParams[this->nextParam]);
                param = adPythonUserParams[this->nextParam++];
            }
            // set int param
            setIntegerParam(param, PyInt_AsLong(value));
        } else if (PyString_Check(value)) {            
            if (atinit) {
                createParam(paramStr, asynParamOctet, &adPythonUserParams[this->nextParam]);
                param = adPythonUserParams[this->nextParam++];
            }
            // set string param
            setStringParam(param, PyString_AsString(value));
        } else {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: param %s is not an int, float or string\n",
                driverName, __func__, paramStr);            
        }
    }
    Py_DECREF(pKeys);
    callParamCallbacks();
    return asynSuccess;
}

/** Called by processCallbacks(). Creates an attribute python dict and updates
  * it with values from the incoming attribute list of the array and from any
  * parameters defined by this plugin.
  * \param[in] pArray NDArray input object
  * 
  * Called with dictMutex taken, GIL taken, this->lock taken.
  */ 
// TODO: as with the param dict, it probably makes sense to expose the param
// list directly to python, although it's less of a performance hit here as
// we don't often have many attributes compared to params  
asynStatus adPythonPlugin::updateAttrDict(NDArray *pArray) {
    // Return if we aren't all good
    if (this->pluginState != GOOD) return asynError;

    // Return if no array to operate on
    if (pArray == NULL) return asynError;
     
    // Make a blank dict for the attributes
    Py_XDECREF(this->pAttrs);
    this->pAttrs = PyDict_New();
    if (this->pAttrs == NULL) Bad("Cannot make attribute dict");     
     
    /* Construct an attribute list. We use a separate attribute list
     * from the one in pArray to avoid the need to copy the array.
     * First clear the list*/
    this->pFileAttributes->clear();
    
    /* Now get the current values of the attributes for this plugin */
    this->getAttributes(this->pFileAttributes);

    /* Now append the attributes from the array which are already up to date from
     * the driver and prior plugins */
    pArray->pAttributeList->copy(this->pFileAttributes);    
           
    // Fill it in
    NDAttribute *pAttr = this->pFileAttributes->next(NULL);
    while(pAttr != NULL) {    
        NDAttrDataType_t attrDataType;
        size_t attrDataSize;        
        PyObject *pObject = NULL;
        pAttr->getValueInfo(&attrDataType, &attrDataSize);
        void *value = calloc(1, attrDataSize);
        pAttr->getValue(attrDataType, value, attrDataSize);         
        /* If the attribute is a string, attrDataSize is the length of the 
         * string including the 0 terminator, otherwise it is the size in bytes 
         * of the specific data type
         */
        switch(attrDataType) {
            case(NDAttrInt8):
            case(NDAttrUInt8):
                pObject = PyInt_FromLong(*((short*) value));
                break;
            case(NDAttrInt16):
            case(NDAttrUInt16):
                pObject = PyInt_FromLong(*((int*) value));
                break;
            case(NDAttrInt32):
            case(NDAttrUInt32):
                pObject = PyInt_FromLong(*((long*) value)); 
                break;
            case(NDAttrFloat32):
                pObject = PyFloat_FromDouble(*((float*) value));
                break;
            case(NDAttrFloat64):
                pObject = PyFloat_FromDouble(*((double*) value));
                break;
            case(NDAttrString):
                pObject = PyString_FromString((char *) value);
                break;
            default:
                break;
        }
        free(value);
        if (pObject == NULL) {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: attribute %s could not be put in attribute dict\n",
                driverName, __func__, pAttr->getName());            
        } else {            
            PyDict_SetItemString(this->pAttrs, pAttr->getName(), pObject); 
            Py_DECREF(pObject); 
        }
        pAttr = this->pFileAttributes->next(pAttr);
    }        
    return asynSuccess;
}

/** Called by processCallbacks(). Updates the attribute list on the outgoing
  * NDArray with values from the attribute python dict modified by the user
  * python code.
  * 
  * Called with dictMutex taken, GIL taken, this->lock taken.
  */ 
asynStatus adPythonPlugin::updateAttrList() {
     // Return if we aren't all good
    if (this->pluginState != GOOD) return asynError;

    // Ignore and return if we did not get an a new array out
    if (this->pArrays[0] == NULL) return asynSuccess;

    // Return if we don't have an attribute dict to read from
    if (this->pAttrs == NULL) Bad("Attribute dict is null");

    // Create attr key list
    PyObject *pKeys = PyDict_Keys(this->pAttrs);
    if (pKeys == NULL) Bad("Can't get keys of attribute dict");
    
    // Create a param of the correct type for each item
    for (Py_ssize_t i=0; i<PyList_Size(pKeys); i++) {
        PyObject *key = PyList_GetItem(pKeys, i);
        PyObject *keyStr = PyObject_Str(key);
        char *paramStr = PyString_AsString(keyStr);
        PyObject *pValue = PyDict_GetItem(this->pAttrs, key);
        if (PyFloat_Check(pValue)) {
            double value = PyFloat_AsDouble(pValue);
            this->pArrays[0]->pAttributeList->add(paramStr, paramStr, NDAttrFloat64, &value);
        } else if (PyInt_Check(pValue)) {
            long value = PyInt_AsLong(pValue);
            this->pArrays[0]->pAttributeList->add(paramStr, paramStr, NDAttrInt32, &value);
        } else if (PyString_Check(pValue)) {     
            char *value = PyString_AsString(pValue);
            this->pArrays[0]->pAttributeList->add(paramStr, paramStr, NDAttrString, value);
        } else {
            asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                "%s:%s: param %s is not an int, float or string\n",
                driverName, __func__, paramStr);            
        }
    }
    Py_DECREF(pKeys);    
    return asynSuccess;
}

/* The obligatory lookup table of ad datatype to numpy datatype */
struct pix_lookup {
    int npy_fmt;
    NDDataType_t ad_fmt;
};
static const struct pix_lookup pix_lookup[] = {
   { NPY_INT8,     NDInt8 },     /**< Signed 8-bit integer */
   { NPY_UINT8,    NDUInt8 },    /**< Unsigned 8-bit integer */
   { NPY_INT16,    NDInt16 },    /**< Signed 16-bit integer */
   { NPY_UINT16,   NDUInt16 },   /**< Unsigned 16-bit integer */
   { NPY_INT32,    NDInt32 },    /**< Signed 32-bit integer */
   { NPY_UINT16,   NDUInt32 },   /**< Unsigned 32-bit integer */
   { NPY_FLOAT32,  NDFloat32 },  /**< 32-bit float */
   { NPY_FLOAT64,  NDFloat64 }   /**< 64-bit float */
};

/** Lookup a numpy pixel format from an NDDataType */
asynStatus adPythonPlugin::lookupNpyFormat(NDDataType_t ad_fmt, int *npy_fmt) {
    const int N = sizeof(pix_lookup) / sizeof(struct pix_lookup);
    for (int i = 0; i < N; i ++)
        if (ad_fmt == pix_lookup[i].ad_fmt) {
            *npy_fmt = pix_lookup[i].npy_fmt;
            return asynSuccess;
        }
    return asynError;
}

/** Lookup an NDDataType from a numpy pixel format */
asynStatus adPythonPlugin::lookupAdFormat(int npy_fmt, NDDataType_t *ad_fmt) {
    const int N = sizeof(pix_lookup) / sizeof(struct pix_lookup);
    for (int i = 0; i < N; i ++)
        if (npy_fmt == pix_lookup[i].npy_fmt) {
            *ad_fmt = pix_lookup[i].ad_fmt;
            return asynSuccess;
        }
    return asynError;
}

/** Configuration routine.  Called directly, or from the iocsh function in NDFileEpics */
static int adPythonPluginConfigure(const char *portNameArg, const char *filename,
                   const char *classname, int queueSize, int blockingCallbacks,
                   const char *NDArrayPort, int NDArrayAddr, int maxBuffers, size_t maxMemory,
                   int priority, int stackSize) {
    // Stack Size must be a minimum of 2MB
    if (stackSize < 2097152) stackSize = 2097152;
    new adPythonPlugin(portNameArg, filename,
                   classname, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, maxBuffers, maxMemory,
                   priority, stackSize);
    return(asynSuccess);
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "filename",iocshArgString};
static const iocshArg initArg2 = { "classname",iocshArgString};
static const iocshArg initArg3 = { "frame queue size",iocshArgInt};
static const iocshArg initArg4 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg5 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg6 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg7 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg8 = { "maxMemory",iocshArgInt};
static const iocshArg initArg9 = { "priority",iocshArgInt};
static const iocshArg initArg10 = { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8,
                                            &initArg9,
                                            &initArg10};
static const iocshFuncDef initFuncDef = {"adPythonPluginConfigure",11,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    adPythonPluginConfigure(args[0].sval, args[1].sval, args[2].sval, 
                       args[3].ival, args[4].ival,
                       args[5].sval, args[6].ival, args[7].ival,
                       args[8].ival, args[9].ival, args[10].ival);
}

//extern "C" void adPythonPluginRegister(void);
static void adPythonPluginRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(adPythonPluginRegister);
}
