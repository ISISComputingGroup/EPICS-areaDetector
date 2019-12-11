/*
 * IntegerParam.h
 *
 * Use this class to represent an asyn parameter.
 *
 * Author:  Jonathan Thompson
 */

#ifndef _INTEGERPARAM_H_
#define _INTEGERPARAM_H_

#include "AsynParam.h"

class IntegerParam: public AsynParam
{
public:
	IntegerParam(ADDriverEx* driver, const char* name, int initialValue,
			AbstractNotify* notify=NULL, int list=0);
	IntegerParam(ADDriverEx* driver, const char* name,
			AbstractNotify* notify=NULL, int list=0);
	IntegerParam(ADDriverEx* driver, int handle,
			AbstractNotify* notify=NULL, int list=0);
	IntegerParam(const IntegerParam& other, AbstractNotify* notify=NULL);
	virtual ~IntegerParam();
	IntegerParam& operator=(int value);
	operator int() const;
};

#endif /* _INTEGERPARAM_H_ */
