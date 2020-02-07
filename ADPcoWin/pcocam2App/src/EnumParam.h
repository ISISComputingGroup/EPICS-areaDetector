/*
 * EnumParam.h
 *
 * Use this class to represent an asyn integer parameter as an enumerated type.
 *
 * Author:  Jonathan Thompson
 */

#ifndef _ENUMPARAM_H_
#define _ENUMPARAM_H_

#include "IntegerParam.h"

template<class T>
class EnumParam: public IntegerParam
{
public:
	EnumParam(ADDriverEx* driver, const char* name, T initialValue,
			AbstractNotify* notify=NULL, int list=0)
		: IntegerParam(driver, name, (int)initialValue, notify, list)
		{}
	EnumParam(ADDriverEx* driver, const char* name,
			AbstractNotify* notify=NULL, int list=0)
		: IntegerParam(driver, name, notify, list)
		{}
	EnumParam(ADDriverEx* driver, int handle,
			AbstractNotify* notify=NULL, int list=0)
		: IntegerParam(driver, handle, notify, list)
		{}
	EnumParam(const IntegerParam& other, AbstractNotify* notify=NULL)
		: IntegerParam(other, notify)
		{}
	virtual ~EnumParam()
		{}
	EnumParam& operator=(T value)
		{
			IntegerParam::operator=((int)value);
			return *this;
		}
	operator T() const
		{
			return (T)IntegerParam::operator int();
		}
};

#endif /* _ENUMPARAM_H_ */
