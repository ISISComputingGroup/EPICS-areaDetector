/* GangConfig.h
 *
 * Revamped PCO area detector driver.
 * A class used to transfer configuration to clients
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */

#ifndef GANGCONFIG_H_
#define GANGCONFIG_H_

class Pco;
class TakeLock;

class GangConfig {
public:
    GangConfig();
    ~GangConfig();
    void fromPco(Pco* pco, TakeLock& takeLock);
    void toPco(Pco* pco, TakeLock& takeLock);
    void *data();
private:
    double exposure;
    double acqPeriod;
    int expPerImage;
    int numImages;
    int imageMode;
    int triggerMode;
    int dataType;
};

#endif /* PCOCAM2APP_SRC_GANGCONFIG_H_ */
