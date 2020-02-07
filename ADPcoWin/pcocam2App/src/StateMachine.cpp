/**
 * State machine template class
 */

#include "StateMachine.h"
#include "TraceStream.h"
#include "StringParam.h"
#include "TakeLock.h"
#include <string>
#include <algorithm>

/**
 * Timer class constructor.
 * \param[in] machine The owning state machine
 * \param[in] queue The timer queue to place the timer on
 */
StateMachine::Timer::Timer(StateMachine* machine)
            : timer(machine->timerQueue.createTimer())
            , machine(machine)
            , expiryEvent(0)
{
}

/**
 * Timer class destructor.
 */
StateMachine::Timer::~Timer()
{
    this->timer.destroy();
}

/**
 * Start the timer.
 * \param[in] delay Time in seconds to timer expiry
 * \param[in] expiryEvent The event to post to the state machine on expiry
 */
void StateMachine::Timer::start(double delay, const Event* expiryEvent)
{
    this->expiryEvent = expiryEvent;
    this->timer.start(*this, delay);
}

/**
 * Stop the timer.  If the timer is already stopped, nothing happens.  Note
 * that this function won't remove any expiry events that may have already been
 * posted to the state machines message queue.
 */
void StateMachine::Timer::stop()
{
    this->timer.cancel();
}

/**
 * This function is called when the timer expires.  Post the expiry event
 * to the state machine's queue.  Note that state machine timers are never
 * restarted automatically.
 * \param[in] currentTime The current time
 * \return Indicate whether the timer should be restarted.
 */
StateMachine::Timer::expireStatus StateMachine::Timer::expire(const epicsTime& currentTime)
{
    this->machine->post(this->expiryEvent);
    return noRestart;
}

/* Transition action constructor. */
StateMachine::TransitionAct::TransitionAct(AbstractAct* act, const State* s1, const State* s2,
		const State* s3, const State* s4)
	: act(act), s1(s1), s2(s2), s3(s3), s4(s4)
{
}

/* Trasition action destructor */
StateMachine::TransitionAct::~TransitionAct()
{
	if(act)
	{
		delete act;
	}
}

/* Execute this transition action and return the next state. */
const StateMachine::State* StateMachine::TransitionAct::execute() const
{
	const State* result = s1;
	if(act)
	{
		switch((*act)())
		{
		case firstState:
			result = s1;
			break;
		case secondState:
			result = s2;
			break;
		case thirdState:
			result = s3;
			break;
		default:
			result = s4;
			break;
		}
	}
	return result;
}

/* Transition key constructor. */
StateMachine::TransitionKey::TransitionKey(const State* st, const Event* ev)
	: st(st), ev(ev)
{
}

/* Transition key default constructor. */
StateMachine::TransitionKey::TransitionKey()
	: st(NULL), ev(NULL)
{
}

/* Transition key copy constructor. */
StateMachine::TransitionKey::TransitionKey(const TransitionKey& other)
	: st(NULL), ev(NULL)
{
	*this = other;
}

/* Transition key destructor */
StateMachine::TransitionKey::~TransitionKey()
{
}

/* Transition key assignment operator */
StateMachine::TransitionKey& StateMachine::TransitionKey::operator=(const TransitionKey& other)
{
	st=other.st; 
	ev=other.ev; 
	return *this;
}

/* Transition key less than operator so this object can be used as a map key */
bool StateMachine::TransitionKey::operator<(const TransitionKey& other) const
{
	return *st == *other.st ? *ev < *other.ev : *st < *other.st;
}

/**
 * Constructor.
 * \param[in] name The name of the machine, used in trace messages.
 * \param[in] portUser An asynUser object to use when outputting trace.
 * \param[in] portDriver The asynPortDriver object that contains the state record.
 * \param[in] traceFlag The asyn trace flag to use when outputting trace.
 * \param[in] paramRecord The asyn string parameter to contain the state record.
 * \param[in] user Pointer to the state machine user that handles transitions.
 * \param[in] initial The initial state of the machine.
 * \param[in] stateNames An array of state name strings.
 * \param[in] eventNames An array of event name strings.
 * \param[in] requestQueueCapacity The size of the event queue.
 */
StateMachine::StateMachine(const char* name,
        asynPortDriver* portDriver,
        StringParam* paramRecord,
        TraceStream* tracer, int requestQueueCapacity)
    : currentState(NULL)
    , name(name)
    , tracer(tracer)
    , portDriver(portDriver)
    , paramRecord(paramRecord)
    , requestQueue(requestQueueCapacity, sizeof(const Event*))
    , thread(*this, name, epicsThreadGetStackSize(epicsThreadStackMedium))
    , timerQueue(epicsTimerQueueActive::allocate(true))
    , timer(this)
{
    // Start the thread
    this->thread.start();
}

/**
 * Destructor.
 */
StateMachine::~StateMachine()
{
    this->timerQueue.release();
    std::map<TransitionKey, TransitionAct*>::iterator pos;
    for(std::map<TransitionKey, TransitionAct*>::iterator pos=transitions.begin();
    		pos!=transitions.end(); ++pos)
    {
    	delete pos->second;
    }
	transitions.clear();
    for(std::list<const State*>::iterator pos=states.begin();
    		pos!=states.end(); ++pos)
    {
    	delete *pos;
    }
    states.clear();
    for(std::list<const Event*>::iterator pos=events.begin();
    		pos!=events.end(); ++pos)
    {
    	delete *pos;
    }
    events.clear();
}

/**
 * Define a state transition.
 */
void StateMachine::transition(const State* st, const Event* ev, AbstractAct* act,
		const State* s1, const State* s2, const State* s3, const State* s4)
{
	// The transition
	TransitionAct* action = new TransitionAct(act, s1, s2, s3, s4);
	TransitionKey key(st, ev);
	// Is one already in the map with this st/ev pair?
	std::map<TransitionKey, TransitionAct*>::iterator pos = transitions.find(key);
	if(pos != transitions.end())
	{
		// Yes, remove it.
		delete pos->second;
		transitions.erase(pos);
	}
	// Add in this transition
	transitions[key] = action;
}

/**
 * Define a state
 */
const StateMachine::State* StateMachine::state(const char* name)
{
	states.push_back(new State(name, (int)states.size()));
	return states.back();
}

/**
 * Define an event
 */
const StateMachine::Event* StateMachine::event(const char* name)
{
	events.push_back(new Event(name, (int)events.size()));
	return events.back();
}

/**
 * Set the state machine's initial state
 */
void StateMachine::initialState(const State* state)
{
	currentState = state;
}


/**
 * Post an event to the request queue.
 * \param[in] req The event to post.
 */
void StateMachine::post(const Event* req)
{
    if(tracer != NULL)
    {
    	*(tracer) << name << ": post request = " << *req << std::endl;
    }
    requestQueue.trySend(&req, sizeof(const Event*));
}

/**
 * Start the timer.
 * \param[in] delay Time in seconds to timer expiry
 * \param[in] expiryEvent The event to post to the state machine on expiry
 */
void StateMachine::startTimer(double delay, const Event* expiryEvent)
{
    this->timer.stop();
    this->timer.start(delay, expiryEvent);
}

/**
 * Stop the timer/
 */
void StateMachine::stopTimer()
{
    this->timer.stop();
}

/**
 * The function that is run by the thread.  Runs forever processing
 * events from the message queue when available.
 */
void StateMachine::run()
{
    while(true)
    {
        const Event* event;
        if(requestQueue.receive(&event, sizeof(const Event*)) == sizeof(const Event*))
        {
            // Get the event processed
			TransitionKey key(currentState, event);
			std::map<TransitionKey, TransitionAct*>::iterator pos = transitions.find(key);
			const State* nextState = this->currentState;
			if(pos != transitions.end())
			{
				nextState = pos->second->execute();
			}
            // Do the trace
            if(tracer != NULL)
            {
            	(*tracer) << name << ": " << *currentState << "--" << *event << "--> " <<
            			*nextState << std::endl;
            }
            // Update the state record
            {
            	TakeLock takeLock(portDriver);
            	std::string record = *paramRecord;
            	record += (char)(int)*nextState;
            	size_t startPos = record.size() - maxStateRecordLength;
            	startPos = std::max<size_t>(startPos, 0);
            	*paramRecord = record.substr(startPos, maxStateRecordLength);
            }
            // Change the state
            currentState = nextState;
        }
    }
}

/**
 * Return the number of events on the queue.
 */
 int StateMachine::pending()
 {
     return requestQueue.pending();
 }

 /**
  * Clear the event queue.
  */
 void StateMachine::clear()
{
    const Event* event;
    while(requestQueue.tryReceive(&event, sizeof(const Event*)) == sizeof(const Event*))
    {
        // Just discard messages on the queue
    }
}

 /**
 * Returns true if the state machine is currently in the given state.
 */
bool StateMachine::isState(const State* s)
{
    return this->currentState == s;
}

