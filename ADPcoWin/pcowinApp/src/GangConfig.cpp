/* GangConfig.cpp
 *
 * Revamped PCO area detector driver.
 * A class used to transfer configuration to clients
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */

#include "GangConfig.h"
#include "Pco.h"

// Constructor
GangConfig::GangConfig()
    : exposure(0.0)
    , acqPeriod(0.0)
    , expPerImage(0)
    , numImages(0)
    , imageMode(0)
    , triggerMode(0)
    , dataType(0)
{
}

// Destructor
GangConfig::~GangConfig()
{
}

// Return a pointer to the data
void* GangConfig::data()
{
    return this;
}

// Copy the configuration from the given PCO object
void GangConfig::fromPco(Pco* pco, TakeLock& takeLock)
{
    exposure = pco->paramADAcquireTime;
    acqPeriod = pco->paramADAcquirePeriod;
    expPerImage = pco->paramADNumExposures;
    numImages = pco->paramADNumImages;
    imageMode = pco->paramADImageMode;
    triggerMode= pco->paramADTriggerMode;
    dataType = pco->paramNDDataType;
}

// Copy the configuration to the given PCO object
void GangConfig::toPco(Pco* pco, TakeLock& takeLock)
{
    pco->paramADAcquireTime = exposure;
    pco->paramADAcquirePeriod = acqPeriod;
    pco->paramADNumExposures = expPerImage;
    pco->paramADNumImages = numImages;
    pco->paramADImageMode = imageMode;
    pco->paramADTriggerMode = triggerMode;
    pco->paramNDDataType = (NDDataType_t)dataType;
}
