/*
 * AsynParam.cpp
 *
 * Base class for all asyn parameter types.  Handles creation of the parameter
 * and registering with the driver.
 *
 *  Created on: 22 Oct 2014
 *      Author: fgz73762
 */

#include "AsynParam.h"
#include "ADDriverEx.h"

// Constructor.  Parameter identified by name, create if doesn't exist.
AsynParam::AsynParam(ADDriverEx* driver, const char* name, asynParamType paramType,
		AbstractNotify* notify, int list)
	: name(name)
	, handle(0)
	, driver(driver)
	, paramType(paramType)
	, notifier(notify)
	, list(list)
{
	handle = driver->makeParam(this);
}

// Constructor.  Parameter identified by a handle.  Attach to already existing.
AsynParam::AsynParam(ADDriverEx* driver, int handle, asynParamType paramType,
		AbstractNotify* notify, int list)
	: name("")
	, handle(handle)
	, driver(driver)
	, paramType(paramType)
	, notifier(notify)
	, list(list)
{
	name = driver->attachParam(this);
}

// Constructor.  Copy of existing parameter, possibly with a notifier.
AsynParam::AsynParam(const AsynParam& other, AbstractNotify* notify)
	: name(other.name)
	, handle(other.handle)
	, driver(other.driver)
	, paramType(other.paramType)
	, notifier(notify)
	, list(other.list)
{
	driver->attachParam(this);
}

// Destructor
AsynParam::~AsynParam()
{
	driver->deregisterParam(this);
}

// Return the name
std::string const& AsynParam::getName() const
{
	return name;
}

// Return the handle
int AsynParam::getHandle() const
{
	return handle;
}

// Return the type
asynParamType AsynParam::getType() const
{
	return paramType;
}

// Return the list
int AsynParam::getList() const
{
	return list;
}

// Call the change notification function
void AsynParam::notify(TakeLock& takeLock)
{
	if(notifier)
	{
		(*notifier)(takeLock);
	}
}
