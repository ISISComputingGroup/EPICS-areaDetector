#include "PcoException.h"
#include <sstream>

/**
 * Constructor
 * \param[in] errorCode The error code reported by the PCO library
 */
PcoException::PcoException(const char* function, int errorCode)
    : errorCode(errorCode)
    , function(function)
{
    std::stringstream s;
    s << "PCO error, " << this->function << " returned " << std::hex << this->errorCode;
    this->text = s.str();
}

/**
 * Destructor.
 */
PcoException::~PcoException() throw()
{
}

/**
 * Return a string representation of the exception
 */
const char* PcoException::what() const throw()
{
  return text.c_str();
}

