/*
 * DoubleParam.h
 *
 * Use this class to represent an asyn parameter.
 *
 * Author:  Jonathan Thompson
 */

#ifndef _DOUBLEPARAM_H_
#define _DOUBLEPARAM_H_

#include "AsynParam.h"

class DoubleParam: public AsynParam
{
public:
	DoubleParam(ADDriverEx* driver, const char* name, double initialValue,
			AbstractNotify* notify=NULL, int list=0);
	DoubleParam(ADDriverEx* driver, const char* name,
			AbstractNotify* notify=NULL, int list=0);
	DoubleParam(ADDriverEx* driver, int handle,
			AbstractNotify* notify=NULL, int list=0);
	DoubleParam(const DoubleParam& other, AbstractNotify* notify=NULL);
	virtual ~DoubleParam();
	DoubleParam& operator=(double value);
	operator double() const;
};

#endif /* _INTEGERPARAM_H_ */
