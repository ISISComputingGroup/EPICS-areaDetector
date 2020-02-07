/*
 * ParamManager.cpp
 *
 *  Created on: 22 Oct 2014
 *      Author: fgz73762
 */

#include "ADDriverEx.h"
#include "AsynParam.h"
#include "AsynException.h"
#include "TakeLock.h"
#include <sstream>

// Constructor.
ADDriverEx::ADDriverEx(const char* portName, int numAddresses, int maxBuffers, size_t maxMemory)
: ADDriver(portName,
		/*maxAddr=*/numAddresses, /*numParams=*/500, maxBuffers, maxMemory,
        asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask | asynInt16ArrayMask | asynEnumMask,
        asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask | asynInt16ArrayMask | asynEnumMask,
        ASYN_CANBLOCK,          // ASYN_CANBLOCK=0, ASYN_MULTIDEVICE=0
        1,          // autoConnect = 1
        -1,         // Default priority
        -1)         // Default stack size
, params(numAddresses)
, paramADMinX(this, ADMinX)
, paramADMinY(this, ADMinY)
, paramADSizeX(this, ADSizeX)
, paramADSizeY(this, ADSizeY)
, paramADBinX(this, ADBinX)
, paramADBinY(this, ADBinY)
, paramADMaxSizeX(this, ADMaxSizeX)
, paramADMaxSizeY(this, ADMaxSizeY)
, paramADNumExposures(this, ADNumExposures)
, paramNDArraySize(this, NDArraySize)
, paramADManufacturer(this, ADManufacturer)
, paramADModel(this, ADModel)
, paramADStatusMessage(this, ADStatusMessage)
, paramNDDataType(this, NDDataType)
, paramADAcquireTime(this, ADAcquireTime)
, paramADGain(this, ADGain)
, paramADTemperature(this, ADTemperature)
, paramADStatus(this, ADStatus)
, paramADTriggerMode(this, ADTriggerMode)
, paramADNumImages(this, ADNumImages)
, paramADImageMode(this, ADImageMode)
, paramADAcquirePeriod(this, ADAcquirePeriod)
, paramADReverseX(this, ADReverseX)
, paramADReverseY(this, ADReverseY)
, paramNDArrayCounter(this, NDArrayCounter)
, paramADAcquire(this, ADAcquire)
, paramNDArraySizeX(this, NDArraySizeX)
, paramNDArraySizeY(this, NDArraySizeY)
, paramADNumImagesCounter(this, ADNumImagesCounter)
, paramADNumExposuresCounter(this, ADNumExposuresCounter)
{
}

// Destructor.
ADDriverEx::~ADDriverEx()
{
}

// Deregister an asyn parameter
void ADDriverEx::deregisterParam(AsynParam* param)
{
	std::map<int, std::list<AsynParam*> >::iterator pos = params[param->getList()].find(param->getHandle());
	if(pos != params[param->getList()].end())
	{
		pos->second.remove(param);
	}
}

// Create (or connect to an existing) param
int ADDriverEx::makeParam(AsynParam* param)
{
	int handle;
	// Connect to an existing parameter first
	asynStatus ok = findParam(param->getList(), param->getName().c_str(), &handle);
	if(ok != asynSuccess)
	{
		// Create one
		ok = ADDriver::createParam(param->getList(), param->getName().c_str(), param->getType(), &handle);
		if(ok != asynSuccess)
		{
			std::stringstream str;
			str << "Unable to create parameter " << param->getName();
			throw AsynException(ok, str.str().c_str());
		}
	}
	// Remember the association
	addParameterToList(param, handle);
	return handle;
}

// Add the parameter to the appropriate list mapped by the given handle.
void ADDriverEx::addParameterToList(AsynParam* param, int handle)
{
	// Get or add an entry for this handle
	std::map<int, std::list<AsynParam*> >::iterator pos = params[param->getList()].find(handle);
	if(pos == params[param->getList()].end())
	{
		params[param->getList()][handle] = std::list<AsynParam*>();
		pos = params[param->getList()].find(handle);
	}
	// Add the parameter to the list
	pos->second.push_back(param);
}

// Attach to an existing parameter identified by its handle.  Return the name.
std::string ADDriverEx::attachParam(AsynParam* param)
{
	std::string result;
	// Get the parameter name
	const char* name;
	asynStatus ok = getParamName(param->getList(), param->getHandle(), &name);
	if(ok != asynSuccess)
	{
		std::stringstream str;
		str << "Unable to attach to parameter " << param->getHandle();
		throw AsynException(ok, str.str().c_str());
	}
	result = name;
	// Remember the association
	addParameterToList(param, param->getHandle());
	return result;
}

// Notify all the parameter objects attached to the parameter
// identified by the pasynUser that it has changed.
void ADDriverEx::notifyParameters(asynUser* pasynUser, TakeLock& takeLock)
{
	// Information
    int handle = pasynUser->reason;
    int list;
    pasynManager->getAddr(pasynUser, &list);
    if(list < 0)
    {
    	list = 0;
    }
    // Get the parameter objects representing this parameter
	std::map<int, std::list<AsynParam*> >::iterator paramPos = params[list].find(handle);
	if(paramPos != params[list].end())
	{
		// Call the notifications
		std::list<AsynParam*>::iterator pos;
		for(pos=paramPos->second.begin(); pos!=paramPos->second.end(); ++pos)
		{
			(*pos)->notify(takeLock);
		}
	}
}

// A 32 bit integer write to a parameter
asynStatus ADDriverEx::writeInt32(asynUser *pasynUser, int value)
{
    TakeLock takeLock(this, /*alreadyTaken=*/true);
    // Base class does most of the work including updating the parameters
    asynStatus status = ADDriver::writeInt32(pasynUser, value);

    // Send notifications
    notifyParameters(pasynUser, takeLock);
    return status;
}

// A 64 bit float write to a parameter
asynStatus ADDriverEx::writeFloat64(asynUser *pasynUser, double value)
{
    TakeLock takeLock(this, /*alreadyTaken=*/true);
    // Base class does most of the work including updating the parameters
    asynStatus status = ADDriver::writeFloat64(pasynUser, value);

    // Send notifications
    notifyParameters(pasynUser, takeLock);
    return status;
}

// An octet write to a string parameter
asynStatus ADDriverEx::writeOctet(asynUser *pasynUser, const char* value, size_t maxChars, size_t* nActual)
{
    TakeLock takeLock(this, /*alreadyTaken=*/true);
    // Base class does most of the work including updating the parameters
    asynStatus status = ADDriver::writeOctet(pasynUser, value, maxChars, nActual);

    // Send notifications
    notifyParameters(pasynUser, takeLock);
    return status;
}

