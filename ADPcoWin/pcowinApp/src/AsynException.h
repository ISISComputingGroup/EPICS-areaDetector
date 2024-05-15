/*
 * AsynException.h
 *
 *  Created on: 23 Oct 2014
 *      Author: fgz73762
 */

#ifndef PCOCAM2APP_SRC_ASYNEXCEPTION_H_
#define PCOCAM2APP_SRC_ASYNEXCEPTION_H_

#include <exception>
#include <string>
#include "asynDriver.h"

class AsynException: public std::exception
{
public:
    AsynException(asynStatus code, const char* description) throw();
    virtual ~AsynException() throw();
    virtual const char* what() const throw();
protected:
    std::string description;
};

#endif /* PCOCAM2APP_SRC_ASYNEXCEPTION_H_ */
