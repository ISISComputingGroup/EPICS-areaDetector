/*
 * IntegerParam.h
 *
 * Use this class to represent an asyn parameter.
 *
 * Author:  Jonathan Thompson
 */

#include "IntegerParam.h"
#include "ADDriverEx.h"

// Constructor.  Create an integer parameter and initialise it.
IntegerParam::IntegerParam(ADDriverEx* driver, const char* name, int initialValue,
		AbstractNotify* notify, int list)
	: AsynParam(driver, name, asynParamInt32, notify, list)
{
	*this = initialValue;
}

// Constructor.  Create an integer parameter, no initialisation
IntegerParam::IntegerParam(ADDriverEx* driver, const char* name,
		AbstractNotify* notify, int list)
	: AsynParam(driver, name, asynParamInt32, notify, list)
{
}

// Constructor.  Use an existing parameter.
IntegerParam::IntegerParam(ADDriverEx* driver, int handle,
		AbstractNotify* notify, int list)
	: AsynParam(driver, handle, asynParamInt32, notify, list)
{
}

// Constructor.  Copy an existing parameter with possible notifier.
IntegerParam::IntegerParam(const IntegerParam& other, AbstractNotify* notify)
	: AsynParam(other, notify)
{
}

// Destructor.
IntegerParam::~IntegerParam()
{
}

// Assignment operator
IntegerParam& IntegerParam::operator=(int value)
{
	driver->setIntegerParam(handle, value);
	return *this;
}

// Int cast operator
IntegerParam::operator int() const
{
	int value;
	driver->getIntegerParam(handle, &value);
	return value;
}

