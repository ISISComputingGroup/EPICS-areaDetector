/* PcoBuffer.h
 *
 * Representation of a buffer
 *
 * Author:  Giles Knap
 *          Jonathan Thompson
 *
 */
#include "PcoBuffer.h"

/**
 * Constructor.
 */
PcoBuffer::PcoBuffer(DllApi* api)
    : api(api)
{
    this->initialise();
}

/**
 * Destructor.
 */
PcoBuffer::~PcoBuffer()
{
    this->free();
}

/**
 * Initialise the buffer to its empty state
 */
void PcoBuffer::initialise()
{
    this->bufferNumber = 0;
    this->buffer = NULL;
    this->eventHandle = 0;
}

/**
 * Allocate memory for the buffer
 */
void PcoBuffer::allocate(unsigned long bufferSize)
{
    this->free();
    try
    {
        this->buffer = new unsigned short[bufferSize];
        this->api->allocateBuffer(this->camera, &this->bufferNumber,
                bufferSize, &this->buffer, &this->eventHandle);
    }
    catch(std::bad_alloc& e)
    {
        throw e;
    }
    catch(PcoException& e)
    {
        this->free();
        throw e;
    }
}

/**
 * Free memory associated with the buffer
 */
void PcoBuffer::free()
{
    if(this->buffer != NULL)
    {
        delete[] this->buffer;
        this->buffer = NULL;
        try
        {
            this->api->cancelImages(this->camera);
        }
        catch(PcoException& e)
        {
            this->errorTrace << "Failure: " << e << std::endl;
        }
    }
}
