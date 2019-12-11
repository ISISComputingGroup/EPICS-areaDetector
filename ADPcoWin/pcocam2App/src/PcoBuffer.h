/* Pixium.h
 *
 * Revamped pixium area detector driver.
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */
#ifndef PCOBUFFER_H_
#define PCOBUFFER_H_

#include "DllApi.h"

class PcoBuffer:
{
// Construction
public:
    PcoBuffer(DllApi* api);
    virtual ~PcoBuffer();

// Data
protected:
    short bufferNumber;
    unsigned short* buffer;
    DllApi::Handle eventHandle;
    DllApi* api;

// API
public:
	void initialise();
};

#endif
