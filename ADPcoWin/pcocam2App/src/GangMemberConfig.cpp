/* GangMemberConfig.cpp
 *
 * Revamped PCO area detector driver.
 * A class used to transfer configuration from clients
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */

#include "GangMemberConfig.h"
#include "Pco.h"
#include "GangConnection.h"
#include "GangClient.h"

// Constructor
GangMemberConfig::GangMemberConfig()
	: positionX(0)
	, positionY(0)
	, sizeX(0)
	, sizeY(0)
{
}

// Destructor
GangMemberConfig::~GangMemberConfig()
{
}

// Fill the config object from the PCO object
void GangMemberConfig::fromPco(Pco* pco, GangConnection* connection, TakeLock& takeLock)
{
	positionX = connection->paramPositionX;
	positionY = connection->paramPositionY;
	sizeX = pco->paramADSizeX;
	sizeY = pco->paramADSizeY;
}

// Transfer configuration information to the PCO object
void GangMemberConfig::toPco(Pco* pco, GangClient* client, TakeLock& takeLock)
{
	client->paramPositionX = positionX;
	client->paramPositionY = positionY;
	client->paramSizeX = sizeX;
	client->paramSizeY = sizeY;
}

// Return a pointer to the config data
void* GangMemberConfig::data()
{
	return this;
}
