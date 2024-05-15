/* PerformanceMonitor.h
 *
 * An object that records performance information
 * from the camera.
 *
 * Author:  Jonathan Thompson
 *
 */
#ifndef PerformanceMonitor_H_
#define PerformanceMonitor_H_

#include "IntegerParam.h"
#include "NDArray.h"
#include <map>
class TraceStream;
class Pco;
class TakeLock;

class PerformanceMonitor
{
public:
    enum Param {PERF_REBOOT=0, PERF_CONNECT, PERF_ARM, PERF_START, PERF_GOODFRAME, PERF_MISSINGFRAME,
        PERF_OUTOFARRAYS, PERF_INVALIDFRAME, PERF_FRAMESTATUSERROR, PERF_WAITFAULT, PERF_DRIVERERROR,
        PERF_CAPTUREERROR, PERF_POLLGETFRAME};
    PerformanceMonitor(Pco* pco, TraceStream* trace);
    virtual ~PerformanceMonitor();
    void count(TakeLock& takeLock, PerformanceMonitor::Param param, bool fault=true, int by=1);
    void clear(TakeLock& takeLock);
private:
    Pco* pco;
    TraceStream* trace;
    // Session counters
    IntegerParam paramCntGoodFrame;
    IntegerParam paramCntMissingFrame;      // Missing frame detected based on embedded image number
    IntegerParam paramCntOutOfArrays;       // Failed to allocate an NDArray
    IntegerParam paramCntInvalidFrame;      // The frame has a zero embedded timestamp
    IntegerParam paramCntFrameStatusError;  // PCO driver reports an error for a buffer status
    IntegerParam paramCntWaitFault;         // Windows wait for PCO event returned an error
    IntegerParam paramCntDriverError;       // The PCO driver library returned an error during acquisition
    IntegerParam paramCntCaptureError;      // Frame event received but buffer not released to us
    IntegerParam paramCntPollGetFrame;      // A frame was received by polling after the camera jammed up
    IntegerParam paramCntFault;             // Count of all faults
    // Accumulating counters (autosave restores them after a reboot)
    IntegerParam paramAccReboot;
    IntegerParam paramAccConnect;
    IntegerParam paramAccArm;
    IntegerParam paramAccStart;
    IntegerParam paramAccGoodFrame;
    IntegerParam paramAccMissingFrame;
    IntegerParam paramAccOutOfArrays;
    IntegerParam paramAccInvalidFrame;
    IntegerParam paramAccFrameStatusError;
    IntegerParam paramAccWaitFault;
    IntegerParam paramAccDriverError;
    IntegerParam paramAccCaptureError;
    IntegerParam paramAccPollGetFrame;
    IntegerParam paramAccFault;
    // Commands
    IntegerParam paramTestCount;
    IntegerParam paramReset;
    // The counter maps
    std::map<PerformanceMonitor::Param, IntegerParam*> session;
    std::map<PerformanceMonitor::Param, IntegerParam*> accumulating;
    // Handlers
    void onReset(TakeLock& takeLock);
    void onTestCount(TakeLock& takeLock);
};

#endif /* PerformanceMonitor_H_ */
