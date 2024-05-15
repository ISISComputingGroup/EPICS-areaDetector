/* GangServerConfig.h
 *
 * Revamped PCO area detector driver.
 * A class used to transfer of server configuration to the clients
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */

#ifndef GANGSERVERCONFIG_H_
#define GANGSERVERCONFIG_H_

#include "GangServer.h"
class GangConnection;
class Pco;

class GangServerConfig {
public:
    GangServerConfig();
    ~GangServerConfig();
    void toPco(Pco* pco, GangConnection* gangConnection, TakeLock& takeLock);
    void fromPco(Pco* pco, GangServer* gangServer, TakeLock& takeLock);
    void* data();
private:
    GangServer::GangFunction gangFunction;
};

#endif /* GANGSERVERCONFIG_H_ */
