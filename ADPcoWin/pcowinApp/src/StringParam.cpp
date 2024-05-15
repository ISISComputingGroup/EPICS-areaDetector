/*
 * IntegerParam.h
 *
 * Use this class to represent an asyn parameter.
 *
 * Author:  Jonathan Thompson
 */

#include "StringParam.h"
#include "ADDriverEx.h"

// Constructor.  Create a string parameter and initialise it
StringParam::StringParam(ADDriverEx* driver, const char* name, const char* initialValue,
        AbstractNotify* notify, int list)
    : AsynParam(driver, name, asynParamOctet, notify, list)
{
    *this = initialValue;
}

// Constructor.  Create a string parameter without initialisation
StringParam::StringParam(ADDriverEx* driver, const char* name,
        AbstractNotify* notify, int list)
    : AsynParam(driver, name, asynParamOctet, notify, list)
{
}

// Constructor.  Use an existing parameter.
StringParam::StringParam(ADDriverEx* driver, int handle,
        AbstractNotify* notify, int list)
    : AsynParam(driver, handle, asynParamOctet, notify, list)
{
}

// Constructor.  Copy an existing parameter with possible notifier.
StringParam::StringParam(const StringParam& other, AbstractNotify* notify)
    : AsynParam(other, notify)
{
}

// Destructor.
StringParam::~StringParam()
{
}

// Assignment operator
StringParam& StringParam::operator=(const std::string& value)
{
    driver->setStringParam(handle, value.c_str());
    return *this;
}

// String cast operator
StringParam::operator const std::string&()
{
    char buffer[maxStringSize];
    driver->getStringParam(handle, maxStringSize, buffer);
    value.assign(buffer, maxStringSize);
    return value;
}
