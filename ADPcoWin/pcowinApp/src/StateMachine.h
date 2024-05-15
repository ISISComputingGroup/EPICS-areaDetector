/**
 * State machine template class
 */

#ifndef __STATE_MACHINE_H
#define __STATE_MACHINE_H

#include <string>
#include <map>
#include <list>
#include <iostream>
#include "epicsMessageQueue.h"
#include "epicsThread.h"
#include "epicsTimer.h"
#include "asynPortDriver.h"
class TraceStream;
class StringParam;

class StateMachine: public epicsThreadRunable
{
public:
    enum StateSelector {firstState=0, secondState, thirdState, fourthState};
    class AbstractAct
    {
    public:
        AbstractAct() {}
        virtual StateSelector operator()() = 0;
    };
    template<class Target>
    class Act: public AbstractAct
    {
    public:
        Act(Target* target, StateSelector (Target::*fn)()) :
            target(target), fn(fn) {}
        virtual StateSelector operator()() {return (target->*fn)();}
    private:
        Target* target;
        StateSelector (Target::*fn)();
    };
    class State
    {
    private:
        std::string name;
        int number;
    public:
        State(const char* name, int number) : name(name), number(number) {}
        State() : number(0) {}
        State(const State& other) {*this = other;}
        ~State() {}
        State& operator=(const State& other) {name=other.name; number=other.number; return *this;}
        bool operator<(const State& other) const {return number < other.number;}
        bool operator==(const State& other) const {return number == other.number;}
        operator int() const {return number;}
        friend std::ostream& operator<< (std::ostream& stream, const State& state) {stream << state.name << "[" << state.number << "]"; return stream;}
    };
    class Event
    {
    private:
        std::string name;
        int number;
    public:
        Event(const char* name, int number) : name(name), number(number) {}
        Event() : number(0) {}
        Event(const Event& other) {*this = other;}
        ~Event() {}
        Event& operator=(const Event& other) {name=other.name; number=other.number; return *this;}
        bool operator<(const Event& other) const {return number < other.number;}
        bool operator==(const Event& other) const {return number == other.number;}
        operator int() const {return number;}
        friend std::ostream& operator<< (std::ostream& stream, const Event& event) {stream << event.name << "[" << event.number << "]"; return stream;}
    };
    class Timer: public epicsTimerNotify
    {
    private:
        epicsTimer& timer;
        StateMachine* machine;
        const Event* expiryEvent;
    public:
        Timer(StateMachine* machine);
        virtual ~Timer();
        void start(double delay, const Event* expiryEvent);
        void stop();
        virtual expireStatus expire(const epicsTime& currentTime);
    };
private:
    class TransitionAct
    {
    private:
        AbstractAct* act;
        const State* s1;
        const State* s2;
        const State* s3;
        const State* s4;
    public: 
        TransitionAct(AbstractAct* act, const State* s1, const State* s2, const State* s3, const State* s4);
        ~TransitionAct();
        const State* execute() const;
    };
    class TransitionKey
    {
    private:
        const State* st;
        const Event* ev;
    public:
        TransitionKey(const State* st, const Event* ev);
        TransitionKey();
        TransitionKey(const TransitionKey& other);
        ~TransitionKey();
        TransitionKey& operator=(const TransitionKey& other);
        bool operator<(const TransitionKey& other) const;
    };
    std::map<TransitionKey, TransitionAct*> transitions;
    std::list<const State*> states;
    std::list<const Event*> events;
    const State* currentState;
    std::string name;
    TraceStream* tracer;
    asynPortDriver* portDriver;
    StringParam* paramRecord;
    enum {maxStateRecordLength=40};
    epicsMessageQueue requestQueue;
    epicsThread thread;
    epicsTimerQueueActive& timerQueue;
    Timer timer;
public:
    StateMachine(const char* name, asynPortDriver* portDriver,
            StringParam* paramRecord,
            TraceStream* tracer=NULL, int requestQueueCapacity=10);
    virtual ~StateMachine();
    void post(const Event* req);
    void startTimer(double delay, const Event* expiryEvent);
    void stopTimer();
    virtual void run();
    int pending();
    void clear();
    bool isState(const State* s);
    void transition(const State* initialState, const Event* event, AbstractAct* action,
            const State* firstState, const State* secondState=NULL,
            const State* thirdState=NULL, const State* fourthState=NULL);
    const State* state(const char* name);
    const Event* event(const char* name);
    void initialState(const State* state);
};

#endif

