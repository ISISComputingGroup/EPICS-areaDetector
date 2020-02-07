/* GangServer.cpp
 *
 * Revamped PCO area detector driver.
 * The communication server class for the master of a ganged pair
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */

#include "GangServer.h"
#include "GangClient.h"
#include "TraceStream.h"
#include "Pco.h"
#include "GangConnection.h"
#include "epicsExport.h"
#include "iocsh.h"
#include <cstring>
#include <algorithm>
#include "TakeLock.h"
#include "FreeLock.h"

// Constants
const int GangServer::maxConnections = 3;

// Constructor.
GangServer::GangServer(Pco* pco, TraceStream* trace, int gangPortNumber)
	: SocketProtocol("GangServer", "", 40000000)
	, pco(pco)
	, trace(trace)
	, paramNumConnections(pco, "PCO_GANGSERV_CONNECTIONS", 0)
	, paramPositionX(pco, "PCO_GANGSERV_POSITIONX", 0)
	, paramPositionY(pco, "PCO_GANGSERV_POSITIONY", 0)
	, paramFullSizeX(pco, "PCO_GANGSERV_FULLSIZEX", 0)
	, paramFullSizeY(pco, "PCO_GANGSERV_FULLSIZEY", 0)
	, paramQueueSize(pco, "PCO_GANGSERV_QUEUESIZE", 0)
	, paramMissingPieces(pco, "PCO_GANGSERV_MISSINGPIECES", 0)
	, paramADSizeX(pco->paramADSizeX)
	, paramADSizeY(pco->paramADSizeY)
	, paramGangFunction(pco, "PCO_GANGSERV_FUNCTION", gangFunctionOff,
			new AsynParam::Notify<GangServer>(this, &GangServer::configure))
	, paramServerPort(pco, "PCO_GANGSERV_PORT", gangPortNumber)
	, paramNDDataType(pco->paramNDDataType)
{
	// Create the client connections
	pco->registerGangServer(this);
	for(int i=0; i<maxConnections; i++)
	{
		clients.push_back(new GangClient(pco, trace, this, i));
	}
	listen(gangPortNumber);
	*trace << "Gang server listening" << std::endl;
}

// Destructor
GangServer::~GangServer()
{
	TakeLock takeLock(pco);
	clearImageQueue(takeLock);
	std::vector<GangClient*>::iterator pos;
	for(pos=clients.begin(); pos!=clients.end(); ++pos)
	{
		delete *pos;
	}
	clients.clear();
}

// Clear out stashed NDArrays
void GangServer::clearImageQueue(TakeLock& takeLock)
{
	std::list<std::pair<int, NDArray*> >::iterator pos;
	for(pos=imageQueue.begin(); pos!=imageQueue.end(); ++pos)
	{
		pos->second->release();
	}
	imageQueue.clear();
	paramMissingPieces = 0;
}

// Accept a gang client connection
void GangServer::accepted(long long fd)
{
	GangClient* client = getFreeClient();
	if(client)
	{
		*trace << "Gang member connection accepted" << std::endl;
		TakeLock takeLock(pco);
		client->createConnection(takeLock, fd);
		paramNumConnections = countConnections();
		configure(takeLock);
	}
	else
	{
		*trace << "Gang member connection rejected" << std::endl;
	}
}

// A client has become disconnected
void GangServer::disconnected(TakeLock& takeLock, GangClient* client)
{
	*trace << "Gang member disconnected" << std::endl;
	paramNumConnections = countConnections();
}

// Return the first client that is not connected.
GangClient* GangServer::getFreeClient()
{
	GangClient* result = NULL;
	std::vector<GangClient*>::iterator pos;
	for(pos=clients.begin(); pos!=clients.end() && result==NULL; ++pos)
	{
		if(!(*pos)->isConnected())
		{
			result = *pos;
		}
	}
	return result;
}

// Return the number of active connections.
int GangServer::countConnections()
{
	int num=0;
	std::vector<GangClient*>::iterator pos;
	for(pos=clients.begin(); pos!=clients.end(); ++pos)
	{
		if((*pos)->isConnected())
		{
			num++;
		}
	}
	return num;
}

// Send the server configuration to the clients
void GangServer::configure(TakeLock& takeLock)
{
	GangServerConfig config;
	config.fromPco(pco, this, takeLock);
	std::vector<GangClient*>::iterator pos;
	for(pos=clients.begin(); pos!=clients.end(); ++pos)
	{
		if((*pos)->isConnected())
		{
			(*pos)->configure(&config);
		}
	}
}

// Return true if the server is in control of the clients
bool GangServer::inControl()
{
	return paramGangFunction != gangFunctionOff;
}

// Arm all the clients
void GangServer::arm()
{
	if(inControl())
	{
		TakeLock takeLock(pco);
		clearImageQueue(takeLock);
		determineImageSize(takeLock);
		GangConfig config;
		config.fromPco(pco, takeLock);
		std::vector<GangClient*>::iterator pos;
		for(pos=clients.begin(); pos!=clients.end(); ++pos)
		{
			if((*pos)->isToBeUsed(takeLock))
			{
				(*pos)->arm(&config);
			}
		}
	}
}

// Disarm all the clients
void GangServer::disarm()
{
	if(inControl())
	{
		TakeLock takeLock(pco);
		std::vector<GangClient*>::iterator pos;
		for(pos=clients.begin(); pos!=clients.end(); ++pos)
		{
			if((*pos)->isToBeUsed(takeLock))
			{
				(*pos)->disarm();
			}
		}
	}
}

// Start all the clients acquiring
void GangServer::start()
{
	if(inControl())
	{
		TakeLock takeLock(pco);
		clearImageQueue(takeLock);
		determineImageSize(takeLock);
		GangConfig config;
		config.fromPco(pco, takeLock);
		std::vector<GangClient*>::iterator pos;
		for(pos=clients.begin(); pos!=clients.end(); ++pos)
		{
			if((*pos)->isToBeUsed(takeLock))
			{
				(*pos)->start(&config);
			}
		}
	}
}

// Stop all the clients acquiring
void GangServer::stop()
{
	if(inControl())
	{
		TakeLock takeLock(pco);
		std::vector<GangClient*>::iterator pos;
		for(pos=clients.begin(); pos!=clients.end(); ++pos)
		{
			if((*pos)->isToBeUsed(takeLock))
			{
				(*pos)->stop();
			}
		}
	}
}

// Work out the size of the assembled image.
void GangServer::determineImageSize(TakeLock& takeLock)
{
	int x = 0;
	int y = 0;
	if(inControl())
	{
		// Where's my bit go?
		x = paramPositionX + paramADSizeX;
		y = paramPositionY + paramADSizeY;
		// Now account for all the client's bits
		std::vector<GangClient*>::iterator pos;
		for(pos=clients.begin(); pos!=clients.end(); ++pos)
		{
			if((*pos)->isToBeUsed(takeLock))
			{
				(*pos)->determineImageSize(takeLock, x, y);
			}
		}
	}
	// Report the results
	paramFullSizeX = x;
	paramFullSizeY = y;
}

// An image has been received on the server.
// Returns true if the image has been consumed.
// This is called in the state machine thread so
// we are allowed to check for completed frames here.
bool GangServer::imageReceived(int sequence, NDArray* image)
{
	bool result = false;
	TakeLock takeLock(pco);
	if(paramGangFunction == gangFunctionFull)
	{
		result = true;
		// Place the image in the queue
		imageQueue.push_back(std::pair<int,NDArray*>(sequence, image));
		// Forward any complete images
		makeCompleteImages(takeLock);
		// Update counters
		paramQueueSize = (int)imageQueue.size();
	}
	return result;
}

// Make any complete images and pass them on
void GangServer::makeCompleteImages(TakeLock& takeLock)
{
	// Keep processing until we fail to do one
	bool doneOne=true;
	while(!imageQueue.empty() && doneOne)
	{
		doneOne = false;
		int sequence = imageQueue.front().first;
		// Do all the clients have this sequence?
		bool allPresent = true;
		bool missingPiece = false;
		std::vector<GangClient*>::iterator clientPos;
		for(clientPos=clients.begin(); clientPos!=clients.end(); ++clientPos)
		{
			if((*clientPos)->isToBeUsed(takeLock))
			{
				GangClient::SeqState has = (*clientPos)->hasSequence(sequence);
				allPresent = allPresent && has == GangClient::seqStateYes;
				missingPiece = missingPiece || has == GangClient::seqStateMissing;
			}
		}
		if(missingPiece)
		{
			// Part of this frame has gone missing, count and discard
			paramMissingPieces = paramMissingPieces + 1;
			imageQueue.front().second->release();
			imageQueue.pop_front();
		}
		else if(allPresent)
		{
			// Alloc an array
			NDArray* outImage = pco->allocArray(paramFullSizeX, paramFullSizeY, paramNDDataType);
			if(outImage)
			{
				NDArray* inImage = imageQueue.front().second;
				// Copy metadata
				outImage->uniqueId = inImage->uniqueId;
				outImage->timeStamp = inImage->timeStamp;
				outImage->pAttributeList->clear();
				inImage->pAttributeList->copy(outImage->pAttributeList);
				// Copy my piece into it and free
				insertImagePiece(outImage, inImage, paramPositionX, paramPositionY);
				inImage->release();
				imageQueue.pop_front();
				// Copy the client pieces into it
				for(clientPos=clients.begin(); clientPos!=clients.end(); ++clientPos)
				{
					if((*clientPos)->isToBeUsed(takeLock))
					{
						(*clientPos)->useImage(takeLock, sequence, outImage);
					}
				}
				// Update counters
				paramQueueSize = (int)imageQueue.size();
				// Pass it on
				{
					FreeLock freeLock(takeLock);
					pco->imageComplete(outImage);
				}
				// See if there is another one
				doneOne = true;
			}
		}
	}
}

// Copy the in image into the specified position in the out image
void GangServer::insertImagePiece(NDArray* outImage, NDArray* inImage, int xPos, int yPos)
{
	NDArrayInfo inInfo;
	inImage->getInfo(&inInfo);
	NDArrayInfo outInfo;
	outImage->getInfo(&outInfo);
	int inStride = inInfo.bytesPerElement * (int)inInfo.xSize;
	int inLength = std::min((int)outInfo.xSize-xPos, (int)inInfo.xSize) * inInfo.bytesPerElement;
	if(inLength > 0)
	{
		int outStride = outInfo.bytesPerElement * (int)outInfo.xSize;
		char* out = (char*)outImage->pData + yPos*outStride + xPos*outInfo.bytesPerElement;
		char* in = (char*)inImage->pData;
		for(size_t y=0; y<inInfo.ySize && (y+yPos)<outInfo.ySize; ++y)
		{
			if((out+inLength) > ((char*)outImage->pData+outInfo.totalBytes))
			{
				return;
			}
			else
			{
				memcpy(out, in, inLength);
			}
			out += outStride;
			in += inStride;
		}
	}
}

// C entry point for iocinit
extern "C" int gangServerConfig(const char* portName, int gangPortNumber)
{
    Pco* pco = Pco::getPco(portName);
    if(pco != NULL)
    {
        new GangServer(pco, &pco->gangTrace, gangPortNumber);
    }
    else
    {
        printf("gangServerConfig: Pco \"%s\" not found\n", portName);
    }
    return asynSuccess;
}
static const iocshArg gangServerConfigArg0 = {"PCO Port Name", iocshArgString};
static const iocshArg gangServerConfigArg1 = {"Gang Port Number", iocshArgInt};
static const iocshArg* const gangServerConfigArgs[] =
    {&gangServerConfigArg0, &gangServerConfigArg1};
static const iocshFuncDef configGangServer =
    {"gangServerConfig", 2, gangServerConfigArgs};
static void configGangServerCallFunc(const iocshArgBuf *args)
{
    gangServerConfig(args[0].sval, args[1].ival);
}

/** Register the functions */
static void gangServerRegister(void)
{
    iocshRegister(&configGangServer, configGangServerCallFunc);
}

extern "C" { epicsExportRegistrar(gangServerRegister); }
