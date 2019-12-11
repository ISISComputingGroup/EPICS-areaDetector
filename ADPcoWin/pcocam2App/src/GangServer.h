/* GangServer.h
 *
 * Revamped PCO area detector driver.
 * The communication server class for the master of a ganged pair
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */

#ifndef GANGSERVER_H_
#define GANGSERVER_H_

#include "SocketProtocol.h"
#include "IntegerParam.h"
#include "EnumParam.h"
#include "NDArray.h"
#include <vector>
#include <list>
class Pco;
class GangClient;
class TraceStream;
class GangServerConfig;
class TakeLock;

class GangServer : public SocketProtocol
{
friend class GangServerConfig;
public:
	GangServer(Pco* pco, TraceStream* trace, int gangPortNumber);
	virtual ~GangServer();
	virtual void accepted(long long fd);
	void disconnected(TakeLock& takeLock, GangClient* client);
	void arm();
	void disarm();
	void start();
	void stop();
	void configure(TakeLock& takeLock);
	bool imageReceived(int sequence, NDArray* image);
	void makeCompleteImages(TakeLock& takeLock);
	void insertImagePiece(NDArray* outImage, NDArray* inImage, int xPos, int yPos);
	enum GangFunction {gangFunctionOff=0, gangFunctionControl=1, gangFunctionFull=2};
	enum {imageTagMask=0x0f, imageTag=0xa0};
private:
	Pco* pco;
	std::vector<GangClient*> clients;
	TraceStream* trace;
	static const int maxConnections;
	IntegerParam paramNumConnections;
	IntegerParam paramPositionX;
	IntegerParam paramPositionY;
	IntegerParam paramFullSizeX;
	IntegerParam paramFullSizeY;
	IntegerParam paramQueueSize;
	IntegerParam paramMissingPieces;
	IntegerParam paramADSizeX;
	IntegerParam paramADSizeY;
	EnumParam<GangFunction> paramGangFunction;
	IntegerParam paramServerPort;
	EnumParam<NDDataType_t> paramNDDataType;
	std::list<std::pair<int, NDArray*> > imageQueue;
	GangClient* getFreeClient();
	int countConnections();
	bool inControl();
	void determineImageSize(TakeLock& takeLock);
	void clearImageQueue(TakeLock& takeLock);
};

#endif /* PCOCAM2APP_SRC_GANGSERVER_H_ */
