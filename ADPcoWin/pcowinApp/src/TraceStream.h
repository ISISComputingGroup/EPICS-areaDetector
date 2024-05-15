
#ifndef _TRACESTREAM_H_
#define _TRACESTREAM_H_

#include <iostream>
#include <streambuf>
#include <cstdlib>
#include <locale>
#include <vector>
#include "asynDriver.h"
#include "epicsMutex.h"

class TraceBuf: public std::streambuf
{
public:
    TraceBuf(asynUser* user, int flag, size_t bufferSize);
    void vprintf(const char *pformat, va_list argptr);

protected:
    virtual std::streambuf::int_type overflow(std::streambuf::int_type ch);
    virtual int sync(); 

private:
    static const int  maxTraceBuff;

private:
    asynUser* user;
    int flag;
    std::vector<char> buffer;
    static epicsMutex mutex;

private:
    void writeTrace();
};

class TraceStream: public std::ostream
{
public:
    TraceStream(asynUser* user, int flag, size_t bufferSize=256);
    virtual ~TraceStream();
    void printf(const char *pformat, ...);

private:
    TraceBuf buffer;
};

#endif
