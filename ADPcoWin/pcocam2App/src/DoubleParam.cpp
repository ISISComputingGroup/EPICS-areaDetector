/*
 * DoubleParam.h
 *
 * Use this class to represent an asyn parameter.
 *
 * Author:  Jonathan Thompson
 */

#include "DoubleParam.h"
#include "ADDriverEx.h"

// Constructor.  Create an double parameter and initialise it.
DoubleParam::DoubleParam(ADDriverEx* driver, const char* name, double initialValue,
		AbstractNotify* notify, int list)
	: AsynParam(driver, name, asynParamFloat64, notify, list)
{
	*this = initialValue;
}

// Constructor.  Create an double parameter, no initialisation.
DoubleParam::DoubleParam(ADDriverEx* driver, const char* name,
		AbstractNotify* notify, int list)
	: AsynParam(driver, name, asynParamFloat64, notify, list)
{
}

// Constructor.  Use an existing parameter.
DoubleParam::DoubleParam(ADDriverEx* driver, int handle,
		AbstractNotify* notify, int list)
	: AsynParam(driver, handle, asynParamFloat64, notify, list)
{
}

// Constructor.  Copy an existing parameter with possible notifier.
DoubleParam::DoubleParam(const DoubleParam& other, AbstractNotify* notify)
	: AsynParam(other, notify)
{
}

// Destructor.
DoubleParam::~DoubleParam()
{
}

// Assignment operator
DoubleParam& DoubleParam::operator=(double value)
{
	driver->setDoubleParam(handle, value);
	return *this;
}

// Double cast operator
DoubleParam::operator double() const
{
	double value;
	driver->getDoubleParam(handle, &value);
	return value;
}
