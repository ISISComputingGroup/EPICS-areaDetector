
#ifndef _NDARRAYEXCEPTION_H_
#define _NDARRAYEXCEPTION_H_

#include <exception>
#include <string>

class NDArrayException: public std::exception
{
public:
    NDArrayException(const char* reason);
    virtual ~NDArrayException() throw();
    virtual const char* what() const throw();
protected:
    std::string reason;
    std::string text;
};

#endif
