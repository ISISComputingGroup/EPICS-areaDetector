/* GangServerConfig.cpp
 *
 * Revamped PCO area detector driver.
 * A class used to transfer configuration to the clients
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */

#include "GangServerConfig.h"
#include "Pco.h"
#include "GangServer.h"
#include "GangConnection.h"

// Constructor.
GangServerConfig::GangServerConfig()
    : gangFunction(GangServer::gangFunctionOff)
{
}

// Destructor.
GangServerConfig::~GangServerConfig()
{
}

// Return a pointer to the config data
void* GangServerConfig::data()
{
    return this;
}

// Get data from the PCO
void GangServerConfig::fromPco(Pco* pco, GangServer* gangServer, TakeLock& takeLock)
{
    gangFunction = gangServer->paramGangFunction;
}

// Write data to the PCO
void GangServerConfig::toPco(Pco* pco, GangConnection* gangConnection, TakeLock& takeLock)
{
    gangConnection->paramGangFunction = gangFunction;
}

