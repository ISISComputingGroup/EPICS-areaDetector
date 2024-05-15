/*
 * AsynParam.h
 *
 *  Created on: 22 Oct 2014
 *      Author: fgz73762
 */

#ifndef _ASYNPARAM_H_
#define _ASYNPARAM_H_

#include <string>
#include "asynParamType.h"
class TakeLock;
class ADDriverEx;

class AsynParam
{
public:
    class AbstractNotify
    {
    public:
        AbstractNotify() {}
        virtual void operator()(TakeLock& takeLock) = 0;
    };
    template<class Target>
    class Notify: public AbstractNotify
    {
    public:
        Notify(Target* target, void (Target::*fn)(TakeLock& takeLock)) :
            target(target), fn(fn) {}
        virtual void operator()(TakeLock& takeLock) {(target->*fn)(takeLock);}
    private:
        Target* target;
        void (Target::*fn)(TakeLock& takeLock);
    };
public:
    AsynParam(ADDriverEx* driver, const char* name, asynParamType paramType,
            AbstractNotify* notify, int list);
    AsynParam(ADDriverEx* driver, int handle, asynParamType paramType,
            AbstractNotify* notify, int list);
    AsynParam(const AsynParam& other, AbstractNotify* notify);
    virtual ~AsynParam();
    std::string const& getName() const;
    int getHandle() const;
    asynParamType getType() const;
    int getList() const;
    void notify(TakeLock& takeLock);
protected:
    std::string name;
    int handle;
    ADDriverEx* driver;
    asynParamType paramType;
    AbstractNotify* notifier;
    int list;
};

#endif /* _ASYNPARAM_H_ */
