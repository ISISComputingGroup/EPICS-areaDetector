/*
 * NdArrayRef.cpp
 *
 * See .h file for details
 *
 * Author:  Jonathan Thompson
 */

#include "NdArrayRef.h"
#include "asynPortDriver.h"
#include "NDArray.h"

// Default constructor
NdArrayRef::NdArrayRef()
    : array(NULL)
{
}

// Initialising constructor.  Use this to wrap the array
// pointer as soon as one is obtained.  Note that this
// NOT increment the reference counter.
NdArrayRef::NdArrayRef(NDArray* array)
    : array(array)
{
}

// Copy constructor.
NdArrayRef::NdArrayRef(const NdArrayRef& other)
    : array(NULL)
{
    *this = other;
}

// Destructor.  Release any array we hold.
NdArrayRef::~NdArrayRef()
{
    if(array)
    {
        array->release();
    }
}

// Assignment operator.
NdArrayRef& NdArrayRef::operator=(const NdArrayRef& other)
{
    if(array)
    {
        array->release();
    }
    array = other.array;
    array->reserve();
    return *this;
}

// Cast operator that allows the use of this object in place of
// a pointer to the NDArray.
NdArrayRef::operator NDArray*() const
{
    return array;
}
