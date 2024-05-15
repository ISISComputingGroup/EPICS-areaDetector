/* GangServer.h
 *
 * Revamped PCO area detector driver.
 * The communication connection class for a ganged pair
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */

#include "GangClient.h"
#include "TraceStream.h"
#include "Pco.h"
#include "GangServer.h"
#include "GangConfig.h"
#include "GangServerConfig.h"
#include "TakeLock.h"
#include "FreeLock.h"
#include <sstream>

// Connection class constructor
GangClient::Connection::Connection(GangClient* owner)
    : SocketProtocol("GangClient", "pco_gang", 40000000)
    , owner(owner)
{
}

// Make the asyn name from a string and an index number
std::string GangClient::makeParamName(std::string name, int index)
{
    std::stringstream str;
    str << name << index;
    return str.str();
}

// Constructor
// When used at the client end, the server is NULL
GangClient::GangClient(Pco* pco, TraceStream* trace, GangServer* gangServer,
        int index)
    : pco(pco)
    , trace(trace)
    , gangServer(gangServer)
    , index(index)
    , connection(NULL)
    , paramConnected(pco, makeParamName("PCO_GANGSERV_CONNECTED", index).c_str(), 0)
    , paramUse(pco, makeParamName("PCO_GANGSERV_USE", index).c_str(), 0)
    , paramPositionX(pco, makeParamName("PCO_GANGSERV_POSITIONX", index).c_str(), 0)
    , paramPositionY(pco, makeParamName("PCO_GANGSERV_POSITIONY", index).c_str(), 0)
    , paramSizeX(pco, makeParamName("PCO_GANGSERV_SIZEX", index).c_str(), 0)
    , paramSizeY(pco, makeParamName("PCO_GANGSERV_SIZEY", index).c_str(), 0)
    , paramQueueSize(pco, makeParamName("PCO_GANGSERV_QUEUESIZE", index).c_str(), 0)
    , paramNDDataType(pco->paramNDDataType)
    , image(NULL)
{
}

// Destructor
GangClient::~GangClient()
{
    clearImageQueue();
    if(connection)
    {
        delete connection;
    }
}

// Clear out stashed NDArrays
void GangClient::clearImageQueue()
{
    if(image)
    {
        image->release();
    }
    image = NULL;
    std::list<std::pair<int, NDArray*> >::iterator pos;
    for(pos=imageQueue.begin(); pos!=imageQueue.end(); ++pos)
    {
        pos->second->release();
    }
    imageQueue.clear();
}

// A message has been received from the peer.
void GangClient::receive(char tag, int parameter, void* data, size_t dataSize)
{
    TakeLock takeLock(pco);
    switch(tag)
    {
    case 'm':
        gangMemberConfig.toPco(pco, this, takeLock);
        break;
    case 'i':
        // Place the image in the queue
        imageQueue.push_back(std::pair<int,NDArray*>(parameter, image));
        image = NULL;
        // Get the main thread to forward any complete images
        pco->post(pco->requestMakeImages);
        // Update counters
        paramQueueSize = (int)imageQueue.size();
        break;
    }
}

// Return an indicator if the client has an image with the given sequence number.
// Any frames older than the given number are discarded.
// Returns seqStateNo if it does not
//         seqStateYes if it does
//         seqStateMissing if it has a newer frame
GangClient::SeqState GangClient::hasSequence(int s)
{
    GangClient::SeqState result = seqStateNo;
    // Discard older frames
    while(!imageQueue.empty() && (s - imageQueue.front().first) > 0)
    {
        imageQueue.front().second->release();
        imageQueue.pop_front();
    }
    if(imageQueue.empty())
    {
        result = seqStateNo;
    }
    else if(imageQueue.front().first == s)
    {
        result = seqStateYes;
    }
    else
    {
        result = seqStateMissing;
    }
    return result;
}

// This connection has broken
void GangClient::disconnected()
{
    if(connection)
    {
        TakeLock takeLock(pco);
        paramConnected = 0;
        gangServer->disconnected(takeLock, this);
        delete connection;
        connection = NULL;
    }
}

// Get a buffer for the reception of a message data buffer
void* GangClient::getDataBuffer(char tag, int parameter, size_t dataSize)
{
    void* result = NULL;
    switch(tag)
    {
    case 'm':
        result = gangMemberConfig.data();
        break;
    case 'i':
        {
            TakeLock takeLock(pco);
            if(image)
            {
                image->release();
            }
            image = pco->allocArray(paramSizeX, paramSizeY, paramNDDataType);
            if(image)
            {
                NDArrayInfo arrayInfo;
                image->getInfo(&arrayInfo);
                if(arrayInfo.totalBytes >= dataSize)
                {
                    result = image->pData;
                }
            }
        }
        break;
    }
    return result;
}

// Return true if this connection is up
bool GangClient::isConnected()
{
    TakeLock takeLock(pco);
    return paramConnected != 0;
}

// Return true if this connection is up and the client is to be used
bool GangClient::isToBeUsed(TakeLock& takeLock)
{
    return paramConnected != 0 && paramUse != 0;
}

// Create the connection object
void GangClient::createConnection(TakeLock& takeLock, long long fd)
{
    if(connection)
    {
        delete connection;
    }
    connection = new Connection(this);
    connection->server(fd);
    paramConnected = 1;
}

// Send the arm message to the client
void GangClient::arm(GangConfig* config)
{
    clearImageQueue();
    connection->transmit('a', 0, config->data(), sizeof(GangConfig));
}

// Send the disarm message to the client
void GangClient::disarm()
{
    connection->transmit('d', 0, NULL, 0);
}

// Send the start message to the client
void GangClient::start(GangConfig* config)
{
    clearImageQueue();
    connection->transmit('s', 0, config->data(), sizeof(GangConfig));
}

// Send the stop message to the client
void GangClient::stop()
{
    connection->transmit('x', 0, NULL, 0);
}

// Send server configuration to the client
void GangClient::configure(GangServerConfig* config)
{
    connection->transmit('c', 0, config->data(), sizeof(GangServerConfig));
}

// Adjust the given full image size so that this
// client bit fits.
void GangClient::determineImageSize(TakeLock& takeLock, int& fullSizeX, int& fullSizeY)
{
    if(paramPositionX + paramSizeX > fullSizeX)
    {
        fullSizeX = paramPositionX + paramSizeX;
    }
    if(paramPositionY + paramSizeY > fullSizeY)
    {
        fullSizeY = paramPositionY + paramSizeY;
    }
}

// Insert the image with the sequence number into the out image at the
// appropriate place.  Remove the image from the queue.
// TODO: An improvement can be made by not assuming the image is at the head.
void GangClient::useImage(TakeLock& takeLock, int sequence, NDArray* outImage)
{
    if(!imageQueue.empty())
    {
        // Copy and free the image
        {
            FreeLock freeLock(takeLock);
            NDArray* inImage = imageQueue.front().second;
            gangServer->insertImagePiece(outImage, inImage, paramPositionX, paramPositionY);
            inImage->release();
            imageQueue.pop_front();
        }
        // Update counters
        paramQueueSize = (int)imageQueue.size();
    }
}
