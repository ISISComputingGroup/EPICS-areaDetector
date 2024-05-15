/* PcoCameraDevice.h
 *
 * Stores the firmware information for all devices in a camera.
 *
 * Author:  Benjamin Bradnick
 *
 */


#ifndef PCO_CAMERA_DEVICE_H_
#define PCO_CAMERA_DEVICE_H_


#include <string>
#include <sstream>


class PcoCameraDevice {

public:
    PcoCameraDevice(
        const std::string deviceName, const int majorVersion, const int minorVersion, const int variant);
    ~PcoCameraDevice();

    std::string getName();
    int getVariant();
    int getMajorVersion();
    int getMinorVersion();
    std::string getVersion();

private:

    std::string name;
    int variant;
    int majorVersion;
    int minorVersion;

};


#endif
