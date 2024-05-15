/*
* socketprotocol.h
*
*  Created on: 16 Aug 2011
*      Author: fgz73762
*/


#ifndef SOCKETPROTOCOL_H_
#define SOCKETPROTOCOL_H_

#include <list>
#include <string>
#include "epicsThread.h"

/** A class that handles a socket based protocol.  The messages passed over the socket
* take the form:
*
*     <preamble:char[]> <tag:char> <parameter:int> <dataSize:size_t> <data>
*
* To use the class, override the following functions:
*    getDataBuffer:  Return a data buffer suitable for the data associated with the given tag.
*    receive:        Process the fully received message.
*    disconnected:   Indicates that the socket has disconnected. (optional)
* Call either of the two connect functions to establish the socket connection.  Call
* transmit to send messages to the peer.
*/
class SocketProtocol
{
public:
    // Function called by nested class
    void run();
private:
    /** Private nested class that contains the thread that handles
    * asynchronous reception from the socket.
    */
    class RxThread: public epicsThreadRunable
    {
    private:
        epicsThread thread;
        SocketProtocol* owner;
    public:
        RxThread(SocketProtocol* owner, const char* threadName);
        virtual ~RxThread() {}
        virtual void run() {this->owner->run();}
    };
private:
    enum {MAX_PREAMBLE_SIZE=32, MAX_HOST_NAME_SIZE=100};
    struct HeaderData
    {
        int tag;
        int parameter;
        size_t dataSize;
    };
private:
    RxThread rxThread;
    long long fd;
    enum {STATE_IDLE=0, STATE_LISTENDISC, STATE_LISTENCONN, STATE_SERVER,
        STATE_CLIENTDISC, STATE_CLIENTCONN} state;
    epicsEventId initialiseEventId;
    size_t maxDataSize;
    char preamble[MAX_PREAMBLE_SIZE];
    size_t preambleSize;
    std::string hostName;
    int tcpPort;
    char* txbuffer;
private:
    // Protocol variables
    char* buffer;
    size_t bufferSize;
    HeaderData headerData;
    char preambleData[MAX_PREAMBLE_SIZE];
    enum {RXSTATE_PREAMBLE=0, RXSTATE_HEADER=1, RXSTATE_DATA=2} rxState;
    size_t requiredSize;
public:
    SocketProtocol(const char* name, const char* preamble, size_t maxMessageSize);
    virtual ~SocketProtocol();
    void server(long long fd);
    void client(const char* hostName, int tcpPort);
    void listen(int tcpPort);
    void transmit(char tag, int parameter, void* data, size_t dataSize);
    virtual void connected() {}
    virtual void disconnected() {}
    virtual void accepted(long long fd) {}
    virtual void* getDataBuffer(char tag, int parameter, size_t dataSize) {return NULL;}
    virtual void receive(char tag, int parameter, void* data, size_t dataSize) {}
private:
    void resetProtocol();
    void handleProtocol(size_t n);
};


#endif /* SOCKETPROTOCOL_H_ */
