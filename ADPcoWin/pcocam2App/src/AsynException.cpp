/*
 * AsynException.cpp
 *
 *  Created on: 23 Oct 2014
 *      Author: fgz73762
 */

#include "AsynException.h"
#include <sstream>

// Constructor
AsynException::AsynException(asynStatus code, const char* description) throw()
{
	std::stringstream str;
	str << description << " asynStatus=" << code << std::endl;
	this->description = str.str();
}

// Destructor
AsynException::~AsynException() throw()
{
}

// Return a description of the exception
const char* AsynException::what() const throw()
{
	return description.c_str();
}
