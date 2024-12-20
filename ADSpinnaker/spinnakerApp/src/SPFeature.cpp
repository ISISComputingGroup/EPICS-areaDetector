// SPFeature.cpp
// Mark Rivers
// October 26, 2018

#include <SPFeature.h>
#include <ADSpinnaker.h>

#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

SPFeature::SPFeature(GenICamFeatureSet *set, 
                     std::string const & asynName, asynParamType asynType, int asynIndex,
                     std::string const & featureName, GCFeatureType_t featureType)
                     
         : GenICamFeature(set, asynName, asynType, asynIndex, featureName, featureType), mPBase(nullptr), mIsImplemented(false)
{
    try {
        ADSpinnaker *pDrv = (ADSpinnaker *) mSet->getPortDriver();
        mNodeName = featureName.c_str();
        INodeMap* nodeMap = pDrv->getNodeMap();
        if (nodeMap != NULL) {
            mPBase = (CNodePtr)nodeMap->GetNode(mNodeName);
            mIsImplemented = IsImplemented(mPBase);
        }
        else {
            mIsImplemented = false;
        }
    }
    catch (Spinnaker::Exception &e) {
        printf("SPProperty::SPProperty exception %s\n", e.what());
    }
}

bool SPFeature::isImplemented() { 
    return mIsImplemented; 
}

bool SPFeature::isAvailable() { 
    return IsAvailable(mPBase);
}

bool SPFeature::isReadable() { 
    return IsReadable(mPBase);
}

bool SPFeature::isWritable() { 
    return IsWritable(mPBase);
}

epicsInt64 SPFeature::readInteger() { 
    CIntegerPtr pNode = (CIntegerPtr)mPBase;
    return pNode->GetValue();
}

epicsInt64 SPFeature::readIntegerMin() {
    CIntegerPtr pNode = (CIntegerPtr)mPBase;
    return pNode->GetMin();
}

epicsInt64 SPFeature::readIntegerMax() {
    CIntegerPtr pNode = (CIntegerPtr)mPBase;
    return pNode->GetMax();
}

epicsInt64 SPFeature::readIncrement() { 
    CIntegerPtr pNode = (CIntegerPtr)mPBase;
    return pNode->GetInc();
}

void SPFeature::writeInteger(epicsInt64 value) { 
    CIntegerPtr pNode = (CIntegerPtr)mPBase;
    pNode->SetValue(value);
}

bool SPFeature::readBoolean() { 
    CBooleanPtr pNode = (CBooleanPtr)mPBase;
    return pNode->GetValue();
}

void SPFeature::writeBoolean(bool value) { 
    CBooleanPtr pNode = (CBooleanPtr)mPBase;
    pNode->SetValue(value);
}

double SPFeature::readDouble() { 
    CFloatPtr pNode = (CFloatPtr)mPBase;
    return pNode->GetValue();
}

void SPFeature::writeDouble(double value) { 
    CFloatPtr pNode = (CFloatPtr)mPBase;
    pNode->SetValue(value);
}

double SPFeature::readDoubleMin() {
    CFloatPtr pNode = (CFloatPtr)mPBase;
    return pNode->GetMin();
}

double SPFeature::readDoubleMax() {
    CFloatPtr pNode = (CFloatPtr)mPBase;
    return pNode->GetMax();
}

int SPFeature::readEnumIndex() { 
    CEnumerationPtr pNode = (CEnumerationPtr)mPBase;
    return (int)pNode->GetIntValue();
}

void SPFeature::writeEnumIndex(int value) { 
    CEnumerationPtr pNode = (CEnumerationPtr)mPBase;
    pNode->SetIntValue(value);
}

std::string SPFeature::readEnumString() { 
    CEnumerationPtr pNode = (CEnumerationPtr)mPBase;
    CEnumEntryPtr pEntry = pNode->GetCurrentEntry();
    gcstring value = pEntry->GetSymbolic();
    return value.c_str();
}

void SPFeature::writeEnumString(std::string const &value) { 
    CEnumerationPtr pNode = (CEnumerationPtr)mPBase;
//    gcstring str(value.c_str());
//    pNode->SetValue(str);
}

std::string SPFeature::readString() { 
    CStringPtr pNode = (CStringPtr)mPBase;
    return (pNode->GetValue()).c_str();
}

void SPFeature::writeString(std::string const & value) { 
    CStringPtr pNode = (CStringPtr)mPBase;
    gcstring str(value.c_str());
    pNode->SetValue(str);
}

void SPFeature::writeCommand() { 
    CCommandPtr pNode = (CCommandPtr)mPBase;
    pNode->Execute();
}

void SPFeature::readEnumChoices(std::vector<std::string>& enumStrings, std::vector<int>& enumValues) {
    CEnumerationPtr pNode = (CEnumerationPtr)mPBase;
    NodeList_t entries;
    pNode->GetEntries(entries);
    int numEnums = (int)entries.size();
    for (int i=0; i<numEnums; i++) {
        IEnumEntry *pEntry= dynamic_cast<IEnumEntry *>(entries[i]);
        if (IsAvailable(pEntry) && IsReadable(pEntry)) {
            std::string str = pEntry->GetSymbolic().c_str();
            enumStrings.push_back(str);
            enumValues.push_back((int)pEntry->GetValue());
        }
    }
}
