/*
 * ParamManager.h
 *
 *  Created on: 22 Oct 2014
 *      Author: fgz73762
 */

#ifndef _PARAMMANAGER_H_
#define _PARAMMANAGER_H_

#include "ADDriver.h"
#include "IntegerParam.h"
#include "StringParam.h"
#include "EnumParam.h"
#include "DoubleParam.h"
#include <map>
#include <list>
#include <string>
#include <vector>
class AsynParam;
class TakeLock;

class ADDriverEx: public ADDriver
{
public:
    ADDriverEx(const char* portName, int numAddresses, int maxBuffers, size_t maxMemory);
    virtual ~ADDriverEx();
    virtual asynStatus writeInt32(asynUser *pasynUser, int value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, double value);
    virtual asynStatus writeOctet(asynUser *pasynUser, const char* value, size_t maxChars, size_t* nActual);
    void deregisterParam(AsynParam* param);
    int makeParam(AsynParam* param);
    std::string attachParam(AsynParam* param);
protected:
    std::vector<std::map<int, std::list<AsynParam*> > > params;
    void notifyParameters(asynUser* pasynUser, TakeLock& takeLock);
    void addParameterToList(AsynParam* param, int handle);
public:
    IntegerParam paramADMinX;
    IntegerParam paramADMinY;
    IntegerParam paramADSizeX;
    IntegerParam paramADSizeY;
    IntegerParam paramADBinX;
    IntegerParam paramADBinY;
    IntegerParam paramADMaxSizeX;
    IntegerParam paramADMaxSizeY;
    IntegerParam paramADNumExposures;
    IntegerParam paramNDArraySize;
    StringParam paramADManufacturer;
    StringParam paramADModel;
    StringParam paramADStatusMessage;
    EnumParam<NDDataType_t> paramNDDataType;
    DoubleParam paramADAcquireTime;
    DoubleParam paramADTemperature;
    EnumParam<ADStatus_t> paramADStatus;
    IntegerParam paramADTriggerMode;
    IntegerParam paramADNumImages;
    IntegerParam paramADImageMode;
    DoubleParam paramADAcquirePeriod;
    IntegerParam paramNDArrayCounter;
    IntegerParam paramADAcquire;
    IntegerParam paramNDArraySizeX;
    IntegerParam paramNDArraySizeY;
    IntegerParam paramADNumImagesCounter;
    IntegerParam paramADNumExposuresCounter;
};

#endif /* _PARAMMANAGER_H_ */
