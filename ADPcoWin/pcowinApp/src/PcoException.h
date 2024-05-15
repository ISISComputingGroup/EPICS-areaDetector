
#ifndef _PCOEXCEPTION_H_
#define _PCOEXCEPTION_H_

#include <exception>
#include <string>

class PcoException: public std::exception
{
public:
    PcoException(const char* function, int errorCode);
    virtual ~PcoException() throw();
    virtual const char* what() const throw();
protected:
    int errorCode;
    std::string function;
    std::string text;
};

#endif
