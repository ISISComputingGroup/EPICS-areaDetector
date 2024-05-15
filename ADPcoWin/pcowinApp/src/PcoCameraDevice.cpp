/* PcoCameraDevice.cpp
 *
 * Stores the firmware information for all devices in a camera.
 *
 * Author:  Benjamin Bradnick
 *
 */


#include "PcoCameraDevice.h"


PcoCameraDevice::PcoCameraDevice(
    const std::string deviceName, const int majorVer, const int minorVer, const int var) {

    name = deviceName;
    majorVersion = majorVer;
    minorVersion = minorVer;
    variant = var;

}


PcoCameraDevice::~PcoCameraDevice() {

}


std::string PcoCameraDevice::getName() {
    return name;
}

int PcoCameraDevice::getVariant() {
    return variant;
}

int PcoCameraDevice::getMajorVersion() {
    return majorVersion;
}

int PcoCameraDevice::getMinorVersion() {
    return minorVersion;
}

std::string PcoCameraDevice::getVersion() {
    std::stringstream ss;
    ss << majorVersion << "." << minorVersion;
    return ss.str();
}