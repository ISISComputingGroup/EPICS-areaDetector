/* GangConnection.cpp
 *
 * Revamped PCO area detector driver.
 * The communication connection class for a ganged client
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */

#include "GangConnection.h"
#include "GangMemberConfig.h"
#include "GangServer.h"
#include "TraceStream.h"
#include "Pco.h"
#include "epicsExport.h"
#include "iocsh.h"
#include "NDArray.h"
#include "FreeLock.h"

// Constructor
// When used at the client end, the server is NULL
GangConnection::GangConnection(Pco* pco, TraceStream* trace, const char* serverIp,
		int serverPort)
	: SocketProtocol("GangConnection", "pco_gang", 40000000)
	, pco(pco)
	, trace(trace)
    , paramIsConnected(pco, "PCO_GANGCONN_CONNECTED", 0)
    , paramServerIp(pco, "PCO_GANGCONN_SERVERIP", serverIp)
    , paramServerPort(pco, "PCO_GANGCONN_SERVERPORT", serverPort)
    , paramPositionX(pco, "PCO_GANGCONN_POSITIONX", 0,
		new AsynParam::Notify<GangConnection>(this, &GangConnection::sendMemberConfig))
    , paramPositionY(pco, "PCO_GANGCONN_POSITIONY", 0,
    		new AsynParam::Notify<GangConnection>(this, &GangConnection::sendMemberConfig))
    , paramGangFunction(pco, "PCO_GANGCONN_FUNCTION", GangServer::gangFunctionOff)
	, paramADSizeX(pco->paramADSizeX,
			new AsynParam::Notify<GangConnection>(this, &GangConnection::sendMemberConfig))
	, paramADSizeY(pco->paramADSizeY,
			new AsynParam::Notify<GangConnection>(this, &GangConnection::sendMemberConfig))
{
	// Start the connection
	pco->registerGangConnection(this);
	client(serverIp, serverPort);
	*trace << "Gang client attempting connection" << std::endl;
}

// Destructor
GangConnection::~GangConnection()
{
}

// Send this member's configuration to the server
void GangConnection::sendMemberConfig(TakeLock& takeLock)
{
	GangMemberConfig config;
	config.fromPco(pco, this, takeLock);
	transmit('m', 0, config.data(), sizeof(GangMemberConfig));
}

// Send an image to the server
void GangConnection::sendImage(NDArray* image, int sequence)
{
	TakeLock takeLock(pco);
	if(paramGangFunction == GangServer::gangFunctionFull)
	{
		FreeLock freeLock(takeLock);
		NDArrayInfo arrayInfo;
		image->getInfo(&arrayInfo);
		transmit('i', sequence, image->pData, arrayInfo.totalBytes);
	}
}

// A message has been received from the peer.
void GangConnection::receive(char tag, int parameter, void* data, size_t dataSize)
{
	TakeLock takeLock(pco);
	switch(tag)
	{
	case 'a':
		*trace << "Gang client received arm" << std::endl;
		config.toPco(pco, takeLock);
        pco->post(pco->requestArm);
        break;
	case 'd':
		*trace << "Gang client received disarm" << std::endl;
        pco->post(pco->requestDisarm);
        break;
	case 's':
		*trace << "Gang client received start" << std::endl;
		config.toPco(pco, takeLock);
        pco->post(pco->requestAcquire);
		break;
	case 'x':
		*trace << "Gang client received stop" << std::endl;
        pco->post(pco->requestStop);
		break;
	case 'c':
		*trace << "Gang client received server config" << std::endl;
		serverConfig.toPco(pco, this, takeLock);
		break;
	}
}

// The connection has been made
void GangConnection::connected()
{
	*trace << "Gang client connected" << std::endl;
	TakeLock takeLock(pco);
	paramIsConnected = 1;
	sendMemberConfig(takeLock);
}

// This connection has broken
void GangConnection::disconnected()
{
	*trace << "Gang client disconnected" << std::endl;
	TakeLock takeLock(pco);
	paramIsConnected = 0;
}

// Get a buffer for the reception of a message data buffer
void* GangConnection::getDataBuffer(char tag, int parameter, size_t dataSize)
{
	void* result = NULL;
	switch(tag)
	{
	case 'a':
	case 's':
		if(dataSize == sizeof(GangConfig))
		{
			result = config.data();
		}
		break;
	case 'c':
		if(dataSize == sizeof(GangServerConfig))
		{
			result = serverConfig.data();
		}
		break;
	}
	return result;
}

// C entry point for iocinit
extern "C" int gangConnectionConfig(const char* portName, const char* gangServerIp,
		int gangPortNumber)
{
    Pco* pco = Pco::getPco(portName);
    if(pco != NULL)
    {
        new GangConnection(pco, &pco->gangTrace, gangServerIp, gangPortNumber);
    }
    else
    {
        printf("gangConnectionConfig: Pco \"%s\" not found\n", portName);
    }
    return asynSuccess;
}
static const iocshArg gangConnectionConfigArg0 = {"PCO Port Name", iocshArgString};
static const iocshArg gangConnectionConfigArg1 = {"Gang Server IP", iocshArgString};
static const iocshArg gangConnectionConfigArg2 = {"Gang Port Number", iocshArgInt};
static const iocshArg* const gangConnectionConfigArgs[] =
    {&gangConnectionConfigArg0, &gangConnectionConfigArg1, &gangConnectionConfigArg2};
static const iocshFuncDef configGangConnection =
    {"gangConnectionConfig", 3, gangConnectionConfigArgs};
static void configGangConnectionCallFunc(const iocshArgBuf *args)
{
    gangConnectionConfig(args[0].sval, args[1].sval, args[2].ival);
}

/** Register the functions */
static void gangConnectionRegister(void)
{
    iocshRegister(&configGangConnection, configGangConnectionCallFunc);
}

extern "C" { epicsExportRegistrar(gangConnectionRegister); }
