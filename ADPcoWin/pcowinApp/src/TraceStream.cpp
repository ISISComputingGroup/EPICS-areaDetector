#include "TraceStream.h"
#include <stdio.h>
#include <stdarg.h>
#include "epicsStdio.h"

/** A mutex that prevents trace messages from
 * multiple threads getting mixed.
 */
epicsMutex TraceBuf::mutex;
const int TraceBuf::maxTraceBuff = 160;

/**
 * Buffer constructor.
 * \param[in] user An asynUser pointer to send trace for
 * \param[in] flag A bitmask that controls when this trace is output
 * \param[in] bufferSize The size of the output buffer
 */
TraceBuf::TraceBuf(asynUser* user, int flag, size_t bufferSize)
    : user(user)
    , flag(flag)
    , buffer(bufferSize+2)
{
    char* base = &buffer.front();
    setp(base, base+buffer.size()-2);
}

/**
 * formatted print to provide an 'old school' way of calling trace
 * called via TraceStream::printf
 *
 * \param[in] pformat defines the format of the output
 * \param[in] argptr list of arguments
 */
void TraceBuf::vprintf(const char *pformat, va_list argptr)
{
    char message[maxTraceBuff];

    if(pasynTrace->getTraceMask((user))&(flag))
    {
        TraceBuf::mutex.lock();
        epicsVsnprintf(message, maxTraceBuff, pformat, argptr);
        asynPrint(user, flag, "%s", message);
        TraceBuf::mutex.unlock();
    }
}

/**
 * Called when the output buffer overflows.  The character is placed at the
 * end of the buffer and it is then written out.  Placing the character at
 * the end is safe because we have deliberately made the buffer bigger than
 * we admit to.
 * \param[in] ch The character that doesn't fit into the buffer
 * \return Returns the character if successfully handled, eof if not.
 */
std::streambuf::int_type TraceBuf::overflow(std::streambuf::int_type ch)
{
    std::streambuf::int_type result = traits_type::eof();
    if(ch != traits_type::eof())
    {
        *pptr() = ch;
        pbump(1);
        writeTrace();
        result = ch;
    }
    return result;
}

/**
 * Called when the buffer should be written out, usually in response to the
 * std::endl manipulator.
 * \return Returns 0 for success
 */
int TraceBuf::sync()
{
    writeTrace();
    return 0;
}

/**
 * A function that writes the buffer to the trace output.  A null character is
 * placed at the end of the buffer as the %*s format specifier does not appear
 * to work.  This is safe because we don't admit to the full size of the buffer
 * so there is always space.
 */
void TraceBuf::writeTrace()
{
    // How many characters to output
    size_t n = pptr() - pbase();
    // Set the pointer back to the beginning
    setp(pbase(), epptr());
    //pbump(-n);
    // Tack a null on the end
    pbase()[n] = '\0';
    // Give the text to asyn.
    TraceBuf::mutex.lock();
    asynPrint(user, flag, "%s", pbase());
    TraceBuf::mutex.unlock();
}

/**
 * Constructor.  Make one of these for switch flag you use.
 * \param[in] user The asnUser to write trace for
 * \param[in] flag A bitmask that controls when this trace is output
 * \param[in] bufferSize The size of the output buffer
 */
TraceStream::TraceStream(asynUser* user, int flag, size_t bufferSize)
    : std::ostream(&buffer)
    , buffer(user, flag, bufferSize)
{
}

/**
 * formatted print to provide an 'old school' way of calling trace
 * Note that this has the advantage of not doing the work of
 * converting parameters to strings if the relevant trace bit is not
 * set.
 *
 * \param[in] pformat defines the format of the output
 */
void TraceStream::printf(const char *pformat, ...)
{
    va_list argptr;
    va_start(argptr, pformat);

    buffer.vprintf(pformat, argptr);

    va_end(argptr);
}

/**
 * Destructor.
 */
TraceStream::~TraceStream()
{
}

