// EuresysFeature.cpp
// Mark Rivers
// March 5, 2024

#include <ADEuresys.h>
#include <EuresysFeature.h>

static const char *driverName="EuresysFeature";

EuresysFeature::EuresysFeature(GenICamFeatureSet *set, 
                     std::string const & asynName, asynParamType asynType, int asynIndex,
                     std::string const & featureName, GCFeatureType_t featureType)
                     
         : GenICamFeature(set, asynName, asynType, asynIndex, featureName, featureType),
         mAsynUser(set->getUser()),
         mFeatureName(featureName)
{
    static const char *functionName = "EuresysFeature";

    ADEuresys *pDrv = (ADEuresys *) mSet->getPortDriver();
    mGrabber = pDrv->getGrabber();
    try {
        mIsImplemented = mGrabber->getInteger<RemoteModule>(query::implemented(mFeatureName)) ? true : false;
    }
    catch (std::exception &e) {
        asynPrint(mAsynUser, ASYN_TRACE_ERROR, "%s::%s query error for implemented=%s\n",
                  driverName, functionName, e.what());
    }
 }

void EuresysFeature::reportError(const char *functionName, const char *errorSource, const char *errorWhat) {
    asynPrint(mAsynUser, ASYN_TRACE_ERROR, "%s::%s feature=%s %s=%s\n",
              driverName, functionName, mFeatureName.c_str(), errorSource, errorWhat);
}
bool EuresysFeature::isImplemented() { 
    return mIsImplemented; 
}

bool EuresysFeature::isAvailable() {
    static const char *functionName = "isAvailable";
    bool value = false;
    if (!mIsImplemented) return value;
    try {
        value = mGrabber->getInteger<RemoteModule>(query::available(mFeatureName)) ? true : false;
    }
    catch (std::exception &e) {
        reportError(functionName, "query error for available", e.what());
    }
    return value;
 }

bool EuresysFeature::isReadable() {
    static const char *functionName = "isReadable";
    bool value = false; 
    if (!mIsImplemented) return value;
    try {
        value = mGrabber->getInteger<RemoteModule>(query::readable(mFeatureName)) ? true : false;
    }
    catch (std::exception &e) {
        reportError(functionName, "query error for readable", e.what());
    }
    return value;
}

bool EuresysFeature::isWritable() {
    static const char *functionName = "isWritable";
    bool value = false; 
    if (!mIsImplemented) return value;
    try {
        value = mGrabber->getInteger<RemoteModule>(query::writeable(mFeatureName)) ? true : false;
    }
    catch (std::exception &e) {
        reportError(functionName, "query error for writeable", e.what());
    }
    return value;
}

epicsInt64 EuresysFeature::readInteger() {
    static const char *functionName = "readInteger";
    epicsInt64 value = 0;
    try {
        value = mGrabber->getInteger<RemoteModule>(mFeatureName);
    }
    catch (std::exception &e) {
        reportError(functionName, "error calling getInteger", e.what());
    }
    return value;
}

epicsInt64 EuresysFeature::readIntegerMin() {
    static const char *functionName = "readIntegerMin";
    epicsInt64 value = 0;
    try {
        value = mGrabber->getInteger<RemoteModule>(mFeatureName+".Min");
    }
    catch (std::exception &e) {
        reportError(functionName, "error calling getInteger", e.what());
    }
    return value;
}

epicsInt64 EuresysFeature::readIntegerMax() {
    static const char *functionName = "readIntegerMax";
    epicsInt64 value = 0;
    try {
        value = mGrabber->getInteger<RemoteModule>(mFeatureName+".Max");
    }
    catch (std::exception &e) {
        reportError(functionName, "error calling getInteger", e.what());
    }
    return value;
}

epicsInt64 EuresysFeature::readIncrement() { 
    static const char *functionName = "readIncrement";
    epicsInt64 value = 0;
    try {
        value = mGrabber->getInteger<RemoteModule>(mFeatureName+".Inc");
    }
    catch (std::exception &e) {
        reportError(functionName, "error calling getInteger", e.what());
    }
    return value;
}

void EuresysFeature::writeInteger(epicsInt64 value) { 
    static const char *functionName = "writeInteger";
    try {
        mGrabber->setInteger<RemoteModule>(mFeatureName, value);
    }
    catch (std::exception &e) {
        reportError(functionName, "error calling setInteger", e.what());
    }
}

bool EuresysFeature::readBoolean() { 
    static const char *functionName = "readBoolean";
    epicsInt64 value = 0;
    try {
        value = mGrabber->getInteger<RemoteModule>(mFeatureName);
    }
    catch (std::exception &e) {
        reportError(functionName, "error calling getInteger", e.what());
    }
    return value ? true : false;
}

void EuresysFeature::writeBoolean(bool bval) {
    static const char *functionName = "writeBoolean";
    epicsInt64 value = bval ? 1 : 0;
    try {
        mGrabber->setInteger<RemoteModule>(mFeatureName, value);
    }
    catch (std::exception &e) {
        reportError(functionName, "error calling setInteger", e.what());
    }
}

// The Mikrotron cameras use integer node types for ExposureTime and AcquisitionFrameRate but ADGenICam expects these to be float nodes.
// These double methods need to handle integers
double EuresysFeature::readDouble() {
    static const char *functionName = "readDouble";
    epicsFloat64 value = 0;
    try {
        value = mGrabber->getFloat<RemoteModule>(mFeatureName);
    }
    catch (std::exception &e) {
        reportError(functionName, "error calling getFloat", e.what());
    }
    return value;
}

void EuresysFeature::writeDouble(double value) { 
    static const char *functionName = "writeDouble";
    try {
        mGrabber->setFloat<RemoteModule>(mFeatureName, value);
    }
    catch (std::exception &e) {
        reportError(functionName, "error calling setFloat", e.what());
    }
}

double EuresysFeature::readDoubleMin() {
    static const char *functionName = "readDoubleMin";
    epicsFloat64 value = 0;
    try {
        value = mGrabber->getFloat<RemoteModule>(mFeatureName+".Min");
    }
    catch (std::exception &e) {
        reportError(functionName, "error calling getFloat", e.what());
    }
    return value;
}

double EuresysFeature::readDoubleMax() {
    static const char *functionName = "readDoubleMax";
    epicsFloat64 value = 0;
    try {
        value = mGrabber->getFloat<RemoteModule>(mFeatureName+".Max");
    }
    catch (std::exception &e) {
        reportError(functionName, "error calling getFloat", e.what());
    }
    return value;
}

int EuresysFeature::readEnumIndex() { 
    static const char *functionName = "readEnumIndex";
    epicsInt64 value=0;
    try {
        value = mGrabber->getInteger<RemoteModule>(mFeatureName);
    }
    catch (std::exception &e) {
        reportError(functionName, "error calling getInteger", e.what());
    }
    return (int) value;
}

void EuresysFeature::writeEnumIndex(int value) {
    static const char *functionName = "writeEnumIndex";
    try {
        mGrabber->setInteger<RemoteModule>(mFeatureName, (epicsInt64)value);
    }
    catch (std::exception &e) {
        reportError(functionName, "error calling setInteger", e.what());
    }
}

std::string EuresysFeature::readEnumString() { 
    static const char *functionName = "readEnumString";
    std::string value;
    try {
        value = mGrabber->getString<RemoteModule>(mFeatureName);
    }
    catch (std::exception &e) {
        reportError(functionName, "error calling getString", e.what());
    }
    return value;
}

void EuresysFeature::writeEnumString(std::string const &value) { 
}

std::string EuresysFeature::readString() { 
    static const char *functionName = "readString";
    std::string value;
    try {
        value = mGrabber->getString<RemoteModule>(mFeatureName);
    }
    catch (std::exception &e) {
        reportError(functionName, "error calling getString", e.what());
    }
    return value;
}

void EuresysFeature::writeString(std::string const & value) { 
    static const char *functionName = "writeString";
    try {
        mGrabber->setString<RemoteModule>(mFeatureName, value);
    }
    catch (std::exception &e) {
        reportError(functionName, "error calling setString", e.what());
    }
}

void EuresysFeature::writeCommand() {
    static const char *functionName = "writeCommand";
    try {
        mGrabber->execute<RemoteModule>(mFeatureName);
    }
    catch (std::exception &e) {
        reportError(functionName, "error calling execute", e.what());
    }
}

void EuresysFeature::readEnumChoices(std::vector<std::string>& enumStrings, std::vector<int>& enumValues) {
    static const char *functionName = "readEnumChoices";
    std::vector<std::string> strs;
    try {
        strs = mGrabber->getStringList<RemoteModule>(query::enumEntries(mFeatureName));
    }
    catch (std::exception &e) {
        reportError(functionName, "error calling getStrings", e.what());
    }
    enumStrings = strs;
    epicsInt64 ival=0;
    for (size_t i=0; i<strs.size(); i++) {
        try {
            ival = mGrabber->getInteger<RemoteModule>(mFeatureName+".Entry."+enumStrings[i]);
        }
        catch (std::exception &e) {
            reportError(functionName, "error calling getInteger", e.what());
        }
        enumValues.push_back((int)ival);
    }
}
