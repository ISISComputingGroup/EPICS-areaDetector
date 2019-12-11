/* TakeLock.cpp
 * See .h file header for description.
 *
 * Author:  Jonathan Thompson
 *
 */

#include "TakeLock.h"
#include "FreeLock.h"
#include "asynPortDriver.h"

/**
 * Constructor.  Use this to take the driver lock (or represent an already
 * taken lock if alreadyTaken is true).
 */
TakeLock::TakeLock(asynPortDriver* driver, bool alreadyTaken)
	: driver(driver)
	, mutex(NULL)
	, initiallyTaken(alreadyTaken)
{
	if(!alreadyTaken)
	{
		driver->lock();
	}
}

/**
 * Constructor.  Use this to take a lock that is represented by a FreeLock.
 */
TakeLock::TakeLock(FreeLock& freeLock)
	: driver(freeLock.driver)
	, mutex(freeLock.mutex)
	, initiallyTaken(false)
{
	if(driver != NULL)
	{
		driver->lock();
	}
	else
	{
		mutex->lock();
	}
}

/**
 * Constructor.  Use this to take an arbitary mutex.
 */
TakeLock::TakeLock(epicsMutex* mutex)
	: driver(NULL)
	, mutex(mutex)
	, initiallyTaken(false)
{
	mutex->lock();
}

/**
 * Destructor.  Call parameter call backs (with the lock taken) and
 * return the lock to its initial state.
 */
TakeLock::~TakeLock()
{
	if(driver != NULL)
	{
		driver->callParamCallbacks();
		if(!initiallyTaken)
		{
			driver->unlock();
		}
	}
	else
	{
		if(!initiallyTaken)
		{
			mutex->unlock();
		}
	}
}

