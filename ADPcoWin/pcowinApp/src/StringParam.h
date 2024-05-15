/*
 * StringParam.h
 *
 * Use this class to represent an asyn parameter.
 *
 * Author:  Jonathan Thompson
 */

#ifndef _STRINGPARAM_H_
#define _STRINGPARAM_H_

#include "AsynParam.h"
class ADDriverEx;

class StringParam: public AsynParam
{
public:
    StringParam(ADDriverEx* driver, const char* name, const char* initialValue,
            AbstractNotify* notify=NULL, int list=0);
    StringParam(ADDriverEx* driver, const char* name,
            AbstractNotify* notify=NULL, int list=0);
    StringParam(ADDriverEx* driver, int handle,
            AbstractNotify* notify=NULL, int list=0);
    StringParam(const StringParam& other, AbstractNotify* notify=NULL);
    virtual ~StringParam();
    StringParam& operator=(const std::string& value);
    operator const std::string&();
private:
    enum {maxStringSize=1000};
    std::string value;
};

#endif /* _STRINGPARAM_H_ */
