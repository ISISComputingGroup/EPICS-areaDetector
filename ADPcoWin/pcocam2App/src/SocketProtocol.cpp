/*
 * socketprotocol.cpp
 *
 *  Created on: 16 Aug 2011
 *      Author: fgz73762
 */

#include "SocketProtocol.h"
#ifdef _WIN32
#include <winsock2.h>
#define socklen_t int
#define RECVSIZE int
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#define closesocket close
#define RECVSIZE size_t
#endif
#include <errno.h>
#include <cstring>
#include <stdio.h>
#include "epicsTime.h"
#include "epicsThread.h"
#include "epicsEvent.h"
#include "epicsMutex.h"
#include <iostream>

/** Receive thread constructor
 * \param[in] owner The owner socket protocol
 * \param[in] threadName A name to use for the thread
 */
SocketProtocol::RxThread::RxThread(SocketProtocol* owner, const char* threadName)
: thread(*this, threadName,
        epicsThreadGetStackSize(epicsThreadStackMedium))
{
    this->owner = owner;
    this->thread.start();
}

/** Constructor
 * \param[in] name A name that will be used for the thread
 * \param[in] preamble The preamble string that starts a message
 */
SocketProtocol::SocketProtocol(const char* name, const char* preamble, size_t maxMessageSize)
: rxThread(this, name)
, fd(0)
, state(STATE_IDLE)
, maxDataSize(0)
, tcpPort(0)
, buffer(NULL)
, bufferSize(0)
, rxState(RXSTATE_PREAMBLE)
, requiredSize(0)
{
    this->txbuffer = new char[maxMessageSize];
    /* An event for communicating initialisation state changes to the thread. */
    this->initialiseEventId = epicsEventCreate(epicsEventEmpty);
    if(this->initialiseEventId == 0)
    {
        printf("SocketProtocol::SocketProtocol: epicsEventCreate failure for connect event\n");
    }
    /* Initialise members */
    strncpy(this->preamble, preamble, MAX_PREAMBLE_SIZE);
    this->preambleSize = strnlen(this->preamble, MAX_PREAMBLE_SIZE);
    this->resetProtocol();
}

/** Destructor
 */
SocketProtocol::~SocketProtocol()
{
    // Close any open socket
    if(state == STATE_LISTENCONN || state == STATE_SERVER || state == STATE_CLIENTCONN)
    {
    	closesocket(this->fd);
    }
    delete this->txbuffer;
}

/** Initialise the protocol variables to their initial state
 */
void SocketProtocol::resetProtocol()
{
    // Return to preamble state
    this->rxState = RXSTATE_PREAMBLE;
    this->buffer = preambleData;
    this->requiredSize = this->preambleSize;
    this->bufferSize = 0;
}

/** The receive thread uses this function to receive data from the socket
 * if it is connected.
 */
void SocketProtocol::run()
{
    long long res;
    struct sockaddr_in addr;
    socklen_t addrLen;
    struct hostent *host;
    int n;
    // Wait to allow other initialisation to happen otherwise we occasionally
    // get a segfault when we wait on the initialise event.
    epicsThreadSleep(2.0);
    // Prepare for reception of first preamble
    this->resetProtocol();
    while(true)
    {
        switch(this->state)
        {
        case STATE_IDLE:
            // Wait for an initialisation event
            epicsEventWait(this->initialiseEventId);
            break;
        case STATE_LISTENDISC:
            // Open the server port for listening
            this->fd = ::socket(AF_INET, SOCK_STREAM, 0);
            if(this->fd >= 0)
            {
                // Avoid problems with address already in use
                int reuse = 1;
                ::setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(int));
                int siz = 5000000;
                ::setsockopt(this->fd, SOL_SOCKET, SO_SNDBUF, (char*)&siz, sizeof(int));
                ::setsockopt(this->fd, SOL_SOCKET, SO_RCVBUF, (char*)&siz, sizeof(int));
                // Bind the socket
                addr.sin_family = AF_INET;
                addr.sin_addr.s_addr = INADDR_ANY;
                addr.sin_port = htons(this->tcpPort);
                res = ::bind(this->fd, (struct sockaddr*)&addr, sizeof(addr));
                if(res >= 0)
                {
                    // Socket opened
                    ::listen(this->fd, 5);
                    printf("Listening on port %d\n", this->tcpPort);
                    this->state = STATE_LISTENCONN;
                }
                else
                {
                    // Socket bind failed, wait a while before trying again
                    perror("Bind failed\n");
                    closesocket(this->fd);
                    epicsThreadSleep(2.0);
                }
            }
            else
            {
                // Socket open failed, wait a while before trying again
                printf("Socket failed\n");
                epicsThreadSleep(2.0);
            }
            break;
        case STATE_LISTENCONN:
            // Wait for a connection on the listening socket
            addrLen = sizeof(addr);
            res = ::accept(fd, (struct sockaddr*)&addr, &addrLen);
            if(res >= 0)
            {
                printf("Received connection\n");
                this->accepted(res);
            }
            break;
        case STATE_SERVER:
            // Try to receive the data we are currently expecting
            n = ::recv(this->fd, this->buffer+this->bufferSize, (RECVSIZE)(this->requiredSize-this->bufferSize), 0);
            if(n == 0)
            {
                printf("SocketProtocol::run: socket closed\n");
                // Socket closed
                this->state = STATE_IDLE;
                this->resetProtocol();
                closesocket(this->fd);
                this->disconnected();
            }
            else if(n < 0)
            {
                // Read error
                perror("SocketProtocol::run");
                // Close the socket and start again
                this->state = STATE_IDLE;
                this->resetProtocol();
                closesocket(this->fd);
                this->disconnected();
            }
            else
            {
                this->handleProtocol((size_t)n);
            }
            break;
        case STATE_CLIENTDISC:
            // Try to connect to the remote host
            host = ::gethostbyname(this->hostName.c_str());
            if(host != NULL)
            {
                //printf("Trying remote port at %s:%d\n", this->hostName.c_str(), this->tcpPort);
                this->fd = ::socket(AF_INET, SOCK_STREAM, 0);
                if(this->fd >= 0)
                {
                    memset((char*)&addr, 0, sizeof(addr));
                    addr.sin_family = AF_INET;
                    addr.sin_port = htons(this->tcpPort);
                    memmove((char*)&addr.sin_addr.s_addr, (char*)host->h_addr, host->h_length);
                    int siz = 5000000;
                    ::setsockopt(this->fd, SOL_SOCKET, SO_SNDBUF, (char*)&siz, sizeof(int));
                    ::setsockopt(this->fd, SOL_SOCKET, SO_RCVBUF, (char*)&siz, sizeof(int));
                    res = ::connect(this->fd, (struct sockaddr*)&addr, sizeof(addr));
                    if(res >= 0)
                    {
                        printf("Connected to remote port\n");
                        this->state = STATE_CLIENTCONN;
                        this->resetProtocol();
                        this->connected();
                    }
                    else
                    {
                        // Connection failed, wait a bit before trying again
                    	closesocket(this->fd);
                        epicsThreadSleep(5.0);
                    }
                }
                else
                {
                    // Socket open failed, wait a while before trying again
                    printf("Socket failed\n");
                    epicsThreadSleep(5.0);
                }
            }
            else
            {
                // Host by name failed, wait a while before trying again
                printf("Host by name failed\n");
                epicsThreadSleep(5.0);
            }
            break;
        case STATE_CLIENTCONN:
            // Try to receive the data we are currently expecting
            n = recv(this->fd, this->buffer+this->bufferSize, (RECVSIZE)(this->requiredSize-this->bufferSize), 0);
            if(n == 0)
            {
                // Socket closed
                this->state = STATE_CLIENTDISC;
                this->resetProtocol();
                closesocket(this->fd);
                this->disconnected();
            }
            else if(n < 0)
            {
                // Read error
                perror("SocketProtocol::run");
                // Close the socket and try again
                this->state = STATE_CLIENTDISC;
                this->resetProtocol();
                closesocket(this->fd);
                this->disconnected();
            }
            else
            {
                this->handleProtocol((size_t)n);
            }
            break;
        }
    }
}

/* Data has been received into the buffer, operate the protocol and decide
 * what to do next.
 */
void SocketProtocol::handleProtocol(size_t n)
{
    // Add in what has been received
    this->bufferSize += n;
    // Do we have all the data we are expecting
    if(this->bufferSize == this->requiredSize)
    {
        switch(this->rxState)
        {
        case RXSTATE_PREAMBLE:
            // Do we have a valid preamble?
            if(memcmp(this->buffer, this->preamble, this->preambleSize) == 0)
            {
                // Found a preamble, set up to receive the header
                this->buffer = (char*)&this->headerData;
                this->bufferSize = 0;
                this->requiredSize = sizeof(HeaderData);
                this->rxState = RXSTATE_HEADER;
            }
            else
            {
                // Drop the first character in the preamble and try again
                this->bufferSize--;
                ::memmove(this->buffer, this->buffer+1, this->bufferSize);
            }
            break;
        case RXSTATE_HEADER:
            // The header has been received, if the data length looks
            // good, receive the data.
            if(this->headerData.dataSize == 0)
            {
                // No data in the message
                this->receive(this->headerData.tag, this->headerData.parameter, NULL, 0);
                // Go back to looking for a preamble
                this->resetProtocol();
            }
            else
            {
                // Ask for a buffer for the data
                this->buffer = (char*)this->getDataBuffer(this->headerData.tag,
                        this->headerData.parameter, this->headerData.dataSize);
                if(buffer == NULL)
                {
                    // No buffer returned, go back to looking for a preamble
                    this->resetProtocol();
                }
                else
                {
                    // Receive the data part of the message
                    this->requiredSize = this->headerData.dataSize;
                    this->bufferSize = 0;
                    this->rxState = RXSTATE_DATA;
                }
            }
            break;
        case RXSTATE_DATA:
            // Message with data
            this->receive(this->headerData.tag, this->headerData.parameter, this->buffer, this->headerData.dataSize);
            // Go back to looking for a preamble
            this->resetProtocol();
            break;
        }
    }
}

/** Listen for incoming connections on the specified port
 * \param[in] tcpPort The TCP port
 */
void SocketProtocol::listen(int tcpPort)
{
    if(this->state == STATE_IDLE)
    {
        // Record connection information
        this->state = STATE_LISTENDISC;
        this->tcpPort = tcpPort;
        // Tell the receive thread
        epicsEventSignal(this->initialiseEventId);
    }
}

/** An incoming connection has been accepted on socket fd and can accept data
 * \param[in] fd The file descriptor of the accepted socket
 */
void SocketProtocol::server(long long fd)
{
    if(this->state == STATE_IDLE)
    {
        // Record connection information
        this->state = STATE_SERVER;
        this->fd = fd;
        // Tell the receive thread
        epicsEventSignal(this->initialiseEventId);
    }
}

/** Create a connection to a specified host and port
 * \param[in] hostName The name of the host to connect to
 * \param[in] tcpPort The TCP port to connect to on the host
 */
void SocketProtocol::client(const char* hostName, int tcpPort)
{
    if(this->state == STATE_IDLE)
    {
        // Record connection information
        this->state = STATE_CLIENTDISC;
        this->hostName.assign(hostName);
        this->tcpPort = tcpPort;
        // Tell the receive thread
        epicsEventSignal(this->initialiseEventId);
    }
}

/** Send a packet over the socket
 * \param[in] tag The tag character
 * \param[in] parameter An integer parameter
 * \param[in] data Pointer to a block of data (can be NULL if dataSize is 0)
 * \param[in] dataSize Number of bytes in the data
 */
void SocketProtocol::transmit(char tag, int parameter, void* data, size_t dataSize)
{
    if(this->state == STATE_SERVER || this->state == STATE_CLIENTCONN)
    {
        // Build a message
        memcpy(this->txbuffer, this->preamble, this->preambleSize);
        HeaderData headerData;
        headerData.tag = tag;
        headerData.parameter = parameter;
        headerData.dataSize = dataSize;
        memcpy(this->txbuffer+this->preambleSize, &headerData, sizeof(HeaderData));
        memcpy(this->txbuffer+this->preambleSize+sizeof(HeaderData), data, dataSize);
        if(::send(fd, this->txbuffer, (RECVSIZE)(this->preambleSize+sizeof(HeaderData)+dataSize), 0) < 0)
        {
            perror("SocketProtocol::transmit(header)");
        }
#if 0
        // Send the preamble
        if(::send(fd, this->preamble, this->preambleSize, 0) < 0)
        {
            perror("SocketProtocol::transmit(preamble)");
        }
        // Send the header
        HeaderData headerData;
        headerData.tag = tag;
        headerData.parameter = parameter;
        headerData.dataSize = dataSize;
        if(::send(fd, &headerData, sizeof(HeaderData), 0) < 0)
        {
            perror("SocketProtocol::transmit(header)");
        }
        // Send the data
        if(dataSize > 0)
        {
            if(::send(fd, data, dataSize, 0) < 0)
            {
                perror("SocketProtocol::transmit(data)");
            }
        }
#endif
    }
}


