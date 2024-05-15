#ifndef EURESYS_FEATURE_H
#define EURESYS_FEATURE_H

#include <GenICamFeature.h>

#include "EGrabber.h"
using namespace Euresys;

class EuresysFeature : public GenICamFeature
{
public:
    EuresysFeature(GenICamFeatureSet *set, 
              std::string const & asynName, asynParamType asynType, int asynIndex,
              std::string const & featureName, GCFeatureType_t featureType);
    virtual bool isImplemented(void);
    virtual bool isAvailable(void);
    virtual bool isReadable(void);
    virtual bool isWritable(void);
    virtual epicsInt64 readInteger(void);
    virtual epicsInt64 readIntegerMin(void);
    virtual epicsInt64 readIntegerMax(void);
    virtual epicsInt64 readIncrement(void);
    virtual void writeInteger(epicsInt64 value);
    virtual bool readBoolean(void);
    virtual void writeBoolean (bool value);
    virtual double readDouble(void);
    virtual double readDoubleMin(void);
    virtual double readDoubleMax(void);
    virtual void writeDouble(double value);
    virtual int readEnumIndex(void);
    virtual void writeEnumIndex(int value);
    virtual std::string readEnumString(void);
    virtual void writeEnumString(std::string const & value);
    virtual void readEnumChoices(std::vector<std::string>& enumStrings, std::vector<int>& enumValues);
    virtual std::string readString(void);
    virtual void writeString(std::string const & value);
    virtual void writeCommand(void);

private:
    void reportError(const char *functionName, const char *errorSource, const char *errorWhat);
    asynUser *mAsynUser;
    EGRABBER_CALLBACK *mGrabber;
    std::string mFeatureName;
    bool mIsImplemented;

};

#endif
