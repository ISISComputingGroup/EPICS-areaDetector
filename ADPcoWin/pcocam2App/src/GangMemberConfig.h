/* GangMemberConfig.h
 *
 * Revamped PCO area detector driver.
 * A class used to transfer configuration from clients
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */

#ifndef GANGMEMBERCONFIG_H_
#define GANGMEMBERCONFIG_H_

class GangConnection;
class GangClient;
class Pco;
class TakeLock;

class GangMemberConfig {
public:
	GangMemberConfig();
	~GangMemberConfig();
	void fromPco(Pco* pco, GangConnection* connection, TakeLock& takeLock);
	void toPco(Pco* pco, GangClient* client, TakeLock& takeLock);
	void* data();
private:
	int positionX;
	int positionY;
	int sizeX;
	int sizeY;
};

#endif /* PCOCAM2APP_SRC_GANGMEMBERCONFIG_H_ */
