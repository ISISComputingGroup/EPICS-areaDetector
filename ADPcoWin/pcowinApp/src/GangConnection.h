/* GangServer.h
 *
 * Revamped PCO area detector driver.
 * The communication connection class for a ganged pair
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */
#ifndef GANGCONNECTION_H_
#define GANGCONNECTION_H_

#include "SocketProtocol.h"
#include "GangConfig.h"
#include "GangServerConfig.h"
#include "GangServer.h"
#include "IntegerParam.h"
#include "EnumParam.h"
#include "StringParam.h"
#include "TakeLock.h"
class GangServer;
class TraceStream;
class Pco;
class GangMemberConfig;
class NDArray;

class GangConnection : public SocketProtocol
{
friend class GangMemberConfig;
friend class GangServerConfig;
public:
    GangConnection(Pco* pco, TraceStream* trace, const char* serverIp, int serverPort);
    virtual ~GangConnection();
    virtual void receive(char tag, int parameter, void* data, size_t dataSize);
    virtual void connected();
    virtual void disconnected();
    virtual void* getDataBuffer(char tag, int parameter, size_t dataSize);
    void sendMemberConfig(TakeLock& takeLock);
    void sendImage(NDArray* image, int sequence);
private:
    Pco* pco;
    TraceStream* trace;
    IntegerParam paramIsConnected;
    StringParam paramServerIp;
    IntegerParam paramServerPort;
    IntegerParam paramPositionX;
    IntegerParam paramPositionY;
    EnumParam<GangServer::GangFunction> paramGangFunction;
    IntegerParam paramADSizeX;
    IntegerParam paramADSizeY;
    GangConfig config;
    GangServerConfig serverConfig;
};

#endif /* PCOCAM2APP_SRC_GANGCONNECTION_H_ */
