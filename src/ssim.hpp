// -*-C++-*-
//
//  This file is part of SSim, a simple discrete-event simulator.
//  See http://www.inf.usi.ch/carzaniga/ssim/
//  
//  Copyright (C) 1998-2004 University of Colorado
//  Copyright (C) 2012 Antonio Carzaniga
//  
//  Authors: Antonio Carzaniga <firstname.lastname@usi.ch>
//           See AUTHORS for full details.
//  
//  SSim is free software: you can redistribute it and/or modify it under
//  the terms of the GNU General Public License as published by the Free
//  Software Foundation, either version 3 of the License, or (at your
//  option) any later version.
//  
//  SSim is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with SSim.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef _ssim_hpp
#define _ssim_hpp

#include <vector>
#include <boost/function.hpp>
#include "heap.h"

/** \file ssim.h 
 *
 *  This header file defines the simulator API.
 **/

/** \namespace ssim
 *
 *  @brief name space for the Siena simulator.
 *
 *  This namespace groups all the types and functionalities associated
 *  with the Siena simulator.  These include:
 *
 *  <ol>
 *  <li>the simulator API
 *
 *  <li>the base classes for processes and events
 *
 *  <li>a few other utility classes
 *  </ol>
 **/
namespace ssim {

/** @brief version identifier for this ssim library
 **/
//const char *	Version;

/** @brief process identifier type
 **/
typedef int		ProcessId;

/** @brief no process will be identified by NULL_PROCESSID
 **/
const ProcessId		NULL_PROCESSID = -1;

/** @brief virtual time type
 *
 *  This type represents the basic time in the virtual (simulated)
 *  world.  Being defined as an integer type, virtual time is a
 *  discrete quantity.  The actual semantics of the time unit is
 *  determined by the simulated application.  In other words, a time
 *  interval of 1 may be interpreted as one second, one year, or any
 *  other time interval, depending on the semantics of the simulated
 *  application.
 *
 *  @see Sim::advance_delay(Time),
 *       Sim::signal_event(ProcessId, const Event*, Time),
 *       and Sim::self_signal_event(const Event*, Time).
 **/
typedef double		Time;

/** @brief beginning of time
 **/
const Time		INIT_TIME = 0;

/** @brief basic event in the simulation.
 *
 *  This base class represents a piece of information or a signal
 *  exchanged between two processes through the simulator.  Every
 *  simulated event must inherit from this class.  For example:
 *
 *  \code
 *  class TextMessage : public Event {
 *  public:
 *      char * message;
 *
 *      TextMessage(const char * m) {
 *          message = strdup(m);
 *      }

 *      ~TextMessage() {
 *          free(message);
 *      }
 *  };
 *  \endcode
 *
 *  @see Sim::signal_event(),
 *       Sim::self_signal_event(),
 *       Process::process_event(const Event*),
 *       TProcess::wait_for_event(Time).
 **/
class Event {
 public:
			Event(): refcount(0) {};
    virtual		~Event() {};

 private:
    mutable unsigned refcount;
    friend class SimImpl;	// this is an opaque implementation class
    friend class Sim;		// these need to be friends to manage refcount
};

  typedef boost::function<bool (const Event *)> EventPredicate;

/** @brief Virtual class (interface) representing processes running
 *  within the simulator.
 *
 *  A simulated process must implement this basic interface.
 **/
class Process {
 public:
    virtual		~Process() {};

    /** @brief action executed when the process is initialized.
     *
     *  This method is not a constructor.  It is rather an
     *  initialization step that is executed during the simulation
     *  when the process is created within the simulation through
     *  Sim::create_process.  This initialization is guaranteed to be
     *  executed before any event is signaled to the process.
     **/
    virtual void	init(void) {};

    /** @brief action executed in response to an event signaled to
     *  this process.
     *
     *  The Event object signaled through this method should not be
     *  used outside this method, other than by signaling it to other
     *  processes through Sim::signal_event().  In fact, the Event
     *  pointer passed to this method should be considered invalid
     *  outside this method, as the simulator proceeds to de-allocate
     *  every event object that is not signaled to any other process.
     *
     *  <p>The implementation of this method may specify the duration
     *  of the actions associated with this response using the \link
     *  Sim::advance_delay(Time) advance_delay\endlink method.  By
     *  default, the duration of an action is 0.
     *
     *  <p>The implementation of this method may use the C++
     *  dynamic_cast feature to select an action on the basis of the
     *  actual type of the Event object.  Here's an example that
     *  illustrates this feature:
     *
     *  \code
     *  class Payment : public Event {
     *  public:
     *      int dollars;
     *      //...
     *  };
     *
     *  class Delivery : public Event {
     *  public:
     *      int count;
     *      string model;
     *      //...
     *  };
     *
     *  class GuitarShop : public Process {
     *      //...
     *      virtual void process_event(const Event * e) {
     *          const Payment * p;
     *          const Delivery * d;
     *
     *          if ((p = dynamic_cast<const Payment *>(e)) != 0) { 
     *              cout << "I got a payment for $" << p->dollars << endl;
     *          } else if ((d = dynamic_cast<const Delivery *>(e)) != 0) {
     *              cout << "I got a delivery of " << d->count 
     *                   << " " << d->model << " guitars" << endl;
     *          } else {
     *              cerr << "GuitarShop: error: unknown event." << endl;
     *          }
     *      }
     *  };
     *  \endcode
     *
     *  @see Sim::advance_delay(Time).
     **/
    virtual void	process_event(const Event * msg) {};

    /** @brief executed when the process is explicitly stopped.
     *
     *  A process is stopped by a call to
     *  Sim::stop_process(ProcessId).  This method is executed
     *  immediately after the process has processed all the events
     *  scheduled before the call to Sim::stop_process.
     **/
    virtual void	stop(void) {};
};

/** @brief utility Process class providing a utility interface with the
 *  simulator.
 *
 *  This is a sligtly more advanced Process class that provides 
 *  automatic management of its own process id.
 **/
class ProcessWithPId : public Process {
 public:
    /** @brief activates this process within the simulator.
     *
     *  Creates a simulator process with this process object.  The
     *  same ProcessWithPId can be activated only once.
     *
     *  @return the ProcessId of the created simulator process. Or
     *  NULL_PROCESSID if this process object is not associated with a
     *  simulation process.
     *
     *  @see Sim::create_process(Process*)
     **/
    ProcessId		activate() throw();

    /** @brief process id of this process.
     *
     *  @return the id of the simulation process with this objectk,
     *  or NULL_PROCESSID if no process is associated with this
     *  object.
     **/
    ProcessId		pid() const throw();

    ProcessWithPId() throw();

 private:
    ProcessId process_id;
};

/** @brief an error handler for simulation errors.
 *
 *  Simulation errors occur when an event is scheduled for a process
 *  that is either terminated or busy processing other events.  These
 *  conditions may or may not represent application errors.  In any
 *  case, the simulator delegates the handling of these conditions to
 *  an error handler, which is implemented by extending this class.
 *
 *  @see Sim::set_error_handler(SimErrorHandler *)
 **/ 
class SimErrorHandler {
public:
    virtual ~SimErrorHandler() {}

    /** @brief handles a clear operation.
     *
     *  This method is called by Sim::clear().  This enables
     *  any counters or other internal state of the error 
     *  handler to be reset as necessary.
     *
     *  @see Sim::clear()
     **/
    virtual void clear() throw() {}

    /** @brief handles busy-process conditions.
     *
     *  A busy-process condition occurs when a process is scheduled to
     *  process an event at a time when it is still busy processing
     *  other events.
     *
     *  This method is executed within the simulation in the context
     *  of the busy process.  This means that the handler may access
     *  the simulation's \link Sim::clock() current time\endlink as
     *  well as all the interface functions of the simulator.  In
     *  particular, this method may also signal events to the busy
     *  process (e.g., with \link Sim::self_signal_event(const Event*)
     *  self_signal_event()\endlink).
     *
     *  @param p is the id of the busy process
     *  @param e is the scheduled event (possibly NULL)
     **/ 
    virtual void handle_busy(ProcessId p, const Event * e) throw() {}

    /** @brief handles terminated-process conditions.
     *
     *  A terminated-process condition occurs when a process is
     *  scheduled to process an event at a time when it has already
     *  terminated its execution.
     *
     *  This method is executed within the simulation in the context
     *  of the busy process.  This means that the handler may access
     *  the simulation's \link Sim::clock() current time\endlink as
     *  well as all the interface functions of the simulator.  In
     *  particular, this method may also signal events to the busy
     *  process (e.g., with \link Sim::self_signal_event(const Event*)
     *  self_signal_event()\endlink).
     *
     *  @param p is the id of the terminated process
     *  @param e is the scheduled event (possibly NULL)
     **/ 
    virtual void handle_terminated(ProcessId p, const Event * e) throw() {}
};

/** @brief a generic discrete-event sequential simulator
 *
 *  This class implements a generic discrete-event sequential
 *  simulator.  Sim maintains and executes a time-ordered schedule of
 *  actions (or discrete events).  
 *
 *  Notice that this class is designed to have only static members.
 *  It should therefore be seen and used more as a module than a
 *  class.  In practice, this means that Sim <em>does not define
 *  simulation objects</em>, but rather a single, static simulator.
 *  This design is based on practical considerations: a simulation
 *  object would allow one to handle multiple simulations within the
 *  same program at the same time, however that situation is neither
 *  common nor desirable.  Simulations tend to use a lot of memory and
 *  therefore having many of them is probably a bad idea.  Also,
 *  having multiple simulation objects would require every process
 *  object to maintain a reference to the corresponding simulation
 *  context (object), so that processes can access the virtual clock
 *  of their own simulation, their scheduler, etc.  This is both a
 *  waste of memory, for the many aditional references, and of CPU
 *  time for all the indirect calls to the simulation methods.
 **/ 
class Sim {
public:
    /** @brief creates a new process
     *
     *  Creates a new process with the given Process object.  This
     *  method schedules the execution of the \link Process::init()
     *  init\endlink method for the given object.
     *
     *  This method can be used safely within the simulation as well
     *  as outside the simulation.
     *
     *  @returns the process id of the new simulation process.
     *  @see Process::init()
     **/
    static ProcessId	create_process(Process *) throw();

    /** @brief stops the execution of a given process */
    static int		stop_process(ProcessId) throw();
    /** @brief stops the execution of the current process */
    static void		stop_process() throw();

   /** @brief clears out internal data structures
    *
    *  Resets the simulator making it available for a completely new
    *  simulation.  All scheduled actions are deleted together with
    *  the associated events.  All process identifiers returned by
    *  previoius invocations of \link create_process(Process*)
    *  create_process\endlink are invalidated by this operation.
    *  Notice however that it is the responsibility of the simulation
    *  programmer to delete process objects used in the simulation.
    **/
    static void		clear() throw();

    /** @brief signal an event to the current process immediately
     *
     *  Signal an event to \link this_process() this
     *  process\endlink.  The response is scheduled immediately (i.e.,
     *  at the \link Sim::clock() current time\endlink).
     *
     *  This method must be used within the simulation.  The effect of
     *  using this method outside the simulation is undefined.
     *
     *  @param e is the signaled event (possibly NULL)
     *
     *  @see signal_event(ProcessId, const Event *)
     *       and Process::process_event(const Event *).
     **/
    static void		self_signal_event(const Event * e) throw();

    /** @brief signal an event to the current process at the given time 
     *
     *  Signal a delayed event to the current process.  The response is
     *  scheduled with the given delay.
     *
     *  This method must be used within the simulation.  The effect of
     *  using this method outside the simulation is undefined.
     *
     *  @param e is the signaled event (possibly NULL)
     *
     *  @param delay is the delay from the \link Sim::clock() current
     *  time\endlink
     *
     *  @see signal_event() 
     *       and Process::process_event(const Event *).
     **/
    static void		self_signal_event(const Event * e, Time delay) throw();

    /** @brief signal an event to the given process immediately
     *
     *  Signal an event to the given process.  The response is
     *  scheduled immediately (i.e., at the \link Sim::clock() current
     *  time\endlink).
     *
     *  This method must be used within the simulation.  The effect of
     *  using this method outside the simulation is undefined.
     *
     *  @param p is the destination process. This must be a valid
     *  process id.  That is, a process id returned by 
     *  \link create_process(Process*) * create_process\endlink.  
     *  Using an invalid process id has undefined effects.
     *
     *  @param e is the signaled event (possibly NULL)
     *
     *  @see self_signal_event() 
     *       and Process::process_event(ProcessId, const Event *).
     **/
    static void		signal_event(ProcessId p, const Event * e) throw();

    /** @brief signal an event to the given process at the given time 
     *
     *  Signal a delayed event to the current process.  The response is
     *  scheduled with the given delay.
     *
     *  This method must be used within the simulation.  The effect of
     *  using this method outside the simulation is undefined.
     *
     *  @param p is the destination process. This must be a valid
     *  process id.  That is, a process id returned by 
     *  \link create_process(Process*) * create_process\endlink.  
     *  Using an invalid process id has undefined effects.
     *
     *  @param e is the signaled event (possibly NULL)
     *
     *  @param d is the signal delay starting from the \link
     *  Sim::clock() current time\endlink
     *
     *  @see self_signal_event() 
     *       and Process::process_event(const Event *).
     **/
    static void		signal_event(ProcessId p, const Event * e, Time d) throw();

    /** @brief advance the execution time of the current process.
     *
     *  This method can be used to specify the duration of certain
     *  actions, or certain steps within the same action.  For example:
     *
     *  \code
     *  class SomeProcess : public Process {
     *      //...
     *      virtual void process_event(const Event * e) {
     *          //
     *          // suppose this response is called at (virtual) time X
     *
     *          // ...do something here...
     *
     *          // the above actions have a default duration of 0,
     *          // therefore the following event is scheduled at time X + 5
     *          //
     *          signal_event(e, p, 5);
     *     
     *          advance_delay(10);
     *
     *          // ...do something else here...
     *
     *          // advance_delay(10) specifies a duration of 10, therefore 
     *          // the following (NULL) event is scheduled at time X + 15;
     *          //
     *          signal_event(NULL, 5);
     *      }
     *  };
     *  \endcode
     *
     *  Notice that a simulation process correspond to a single
     *  logical thread.  This means that Process::process_event() and
     *  TProcess::main() are intended to process one event at a time.
     *  Because of this chosen semantics, the use of
     *  advance_delay(Time) may result in the current process missing
     *  some events.  Referring to the above example, the overall
     *  duration of the execution step defined by
     *  SimpleProcess::process_event() is 15 time units.  Therefore, a
     *  SimpleProcess executing its process_event at time X would miss
     *  every event scheduled for it between time X and X + 15.  A
     *  semantically identical situation would occur for a sequential
     *  process.  For example:
     *
     *  \code
     *  class SomeTProcess : public TProcess {
     *      //...
     *      virtual void main() {
     *          // ...
     *          const Event * e;
     *          e = wait_for_event();
     *
     *          // now suppose wait_for_event() returns a signal at time X
     *          // and we do something with this signal...
     *          // and this "something" costs us 20 time units
     *          advance_delay(20);
     *
     *          // now, our virtual clock is X + 20, so we have missed 
     *          // all the signals between X and X + 20
     *          e = wait_for_signal();
     *      }
     *  };
     *  \endcode
     *
     *  In this example, the process misses all the events signaled
     *  within 20 time units after the first event.  This is because
     *  the process is busy working on the first event.
     *
     *  The application may program a handler for missed events by
     *  programming and registering a SimErrorHandler object.
     *
     *  This method must be used within the simulation.  The effect of
     *  using this method outside the simulation is undefined.
     *
     *  @see Sim::clock().
     *  @see SimErrorHandler
     **/
    static void		advance_delay(Time) throw();
    
    /** @brief returns the current process
     *
     *  Process id of the process that is currently scheduled by the
     *  simulation.  This method can be used by a Process in \link
     *  Process::process_event(const Event *e) process_event\endlink
     *  or by a TProcess in \link TProcess::main() main\endlink to
     *  figure out its own process id.
     *
     *  Example:
     *  \code
     *  class SimpleProcess : public Process {
     *      void SimpleProcess::process_event(const Event *) {
     *          cout << "My process id is: " << Sim::this_pocess() << endl;
     *      }
     *  };
     *  \endcode
     *
     *  @return process id of the current process, or \link
     *  ssim::NULL_PROCESSID NULL_PROCESSID\endlink if called outside
     *  the simulation.
     **/
    static ProcessId	this_process() throw();

    /** @brief returns the current virtual time for the current process
     *  
     *  Current virtual time.
     *  
     *  Example:
     *  \code
     *  void LoopProcess::process_event(const Event * e) {
     *      cout << "Here I am at time:" << Sim::clock() << endl;
     *      self_signal_event(NULL, 20)
     *      cout << "I just scheduled myself again for time: " 
     *           << Sim::clock() + 20 << endl;
     *  }
     *  \endcode
     *
     *  @return current virtual time for the current process.
     *  @see advance_delay(Time)
     **/
    static Time		clock() throw();
    
    /** @brief starts execution of the simulation */
    static void		run_simulation();
    /** @brief stops execution of the simulation */
    static void		stop_simulation() throw();

    /** @brief stops the execution of the simulation at the given time
     *
     *  This method sets the absolute (virtual) time at which the
     *  simulation will terminate.  This method can be used to limit
     *  the duration of a simulation even in the presence of
     *  schedulable actions.  When called with the (default)
     *  INIT_TIME, the simulation is set for normal termination, that
     *  is, the simulation terminates in the absence of schedulable
     *  actions.
     *
     *  @see stop_simulation()
     **/
    static void		set_stop_time(Time t = INIT_TIME) throw();

    /** @brief  registers a handler for simulation errors.
     *
     *  An error handler is a callback object that handles all
     *  simulation errors.
     *
     *  @see SimErrorHandler
     **/
    static void		set_error_handler(SimErrorHandler *) throw();
    static void remove_event(EventPredicate pred) throw();
};


  // IMPLEMENTATION

  //const char * Version = VERSION;

// these are the "private" static variables and types of the Sim class
//
  inline Time			&stop_time() { static Time stop_time_ = INIT_TIME; return stop_time_; }
  inline Time			&current_time() { static Time current_time_ = INIT_TIME; return current_time_; }

  inline ProcessId		&current_process() {static ProcessId current_process_ = NULL_PROCESSID; return current_process_; }

  inline bool			&running() { static bool running_ = false; return running_; }

  inline SimErrorHandler 	*error_handler() {static SimErrorHandler * error_handler_ = 0; return error_handler_; }

enum ActionType { 
    A_Event, 
    A_Init, 
    A_Stop 
};
    
struct Action {
    Time time;
    ActionType type;
    ProcessId pid;
    const Event * event;

    Action(Time t, ActionType at, ProcessId p, const Event * e = 0) throw()
	: time(t), type(at), pid(p), event(e) {};

    bool operator < (const Action & a) const throw() {
	return time < a.time;
    }
};

typedef heap<Action>	a_table_t;

  inline a_table_t &actions() {static a_table_t actions_; return actions_; }

struct PDescr {
    Process * 	process;
    bool terminated;
    Time available_at;

    PDescr(Process * p) 
	: process(p), terminated(false), available_at(INIT_TIME) {}
};

typedef std::vector<PDescr> PsTable;
  inline PsTable &processes() {static PsTable processes_; return processes_; }

class SimImpl {
public:
    static void schedule(Time t, ActionType i, ProcessId p, 
			 const Event * e = 0) throw() {
	if (e != 0) { 
	    ++(e->refcount); 
	}
	actions().insert(Action(current_time() + t, i, p, e ));
    }
    static void schedule_now(ActionType i, ProcessId p, 
			     const Event * e = 0) throw() {
	if (e != 0) { 
	    ++(e->refcount); 
	}
	actions().insert(Action(current_time(), i, p, e ));
    }
};

inline ProcessId Sim::create_process(Process * p) throw() {
    processes().push_back(PDescr(p));
    ProcessId newpid = processes().size() - 1;
    SimImpl::schedule_now(A_Init, newpid);
    return newpid;
}

inline void Sim::clear() throw() {
    running() = false;
    current_time() = INIT_TIME;
    current_process() = NULL_PROCESSID;
    processes().clear();
    if (error_handler()) error_handler()->clear();
    for(a_table_t::iterator a = actions().begin(); a != actions().end(); ++a) {
	const Event * e = (*a).event;
	if (e != 0 && --(e->refcount) == 0) 
	    delete(e);
    }
    actions().clear();
}

typedef a_table_t::iterator ForwardIterator;

inline void Sim::remove_event(EventPredicate pred) throw() {
  ForwardIterator first = actions().begin();
  ForwardIterator last = actions().end();
  ForwardIterator result = first;
  while (first != last) {
    if ((*first).type != A_Event) {
      *result = *first;
      ++result;
    } else {
      const Event * e = (*first).event;
      if (e != 0 && !pred(e)) {
	*result = *first;
      ++result;
      }
    }
    ++first;
  }
  actions().erase(result, actions().end());
}

//
// this is the simulator main loop.
//
  inline bool &lock() {static bool lock_ = false; return lock_; }
inline void Sim::run_simulation() {
    //
    // prevents anyone from re-entering the main loop.  Note that this
    // isn't meant to be thread-safe, it works if some process calls
    // Sim::run_simulation() within their process_event() function.
    //
    if (lock()) return;
    lock() = true;
    running() = true;

    //
    // while there is at least a scheduled action
    //
    while (running() && !actions().empty()) {
	//
	// I'm purposely excluding any kind of checks in this version
	// of the simulator.  
	//
	// I should say something like this:
	// assert(current_time() <= (*a).first);
	//
	Action action = actions().pop_first();
	current_time() = action.time;
	if (stop_time() != INIT_TIME && current_time() > stop_time())
	    break;
	current_process() = action.pid;
	//
	// right now I don't check if current_process is indeed a
	// valid process.  Keep in mind that this is the heart of the
	// simulator main loop, therefore efficiency is crucial.
	// Perhaps I should check.  This is somehow a design choice.
	//
	PDescr & pd = processes()[current_process()];

	if (pd.terminated) {
	    if (error_handler()) 
		error_handler()->handle_terminated(current_process(), 
						 action.event);
	} else if (current_time() < pd.available_at) {
	    if (error_handler()) 
		error_handler()->handle_busy(current_process(), action.event);
	} else {
	    switch (action.type) {
	    case A_Event:
		pd.process->process_event(action.event);
		break;
	    case A_Init: 
		pd.process->init(); 
		break;
	    case A_Stop: 
		pd.process->stop();
		//
		// here we must use processes()[current_process()] instead
		// of pd since pd.process->stop() might have added or
		// removed processes, and therefore resized the
		// processes vector, rendering pd invalid
		//
		processes()[current_process()].terminated = true;
		break;
	    default:
		//
		// add paranoia checks/logging here?
		//
		break;
	    }
	    // here we must use processes()[current_process()] instead of
	    // pd.  Same reason as above. the "processes" vector might
	    // have been modified and, as a consequence, resized.  So,
	    // pd may no longer be considered a valid reference.
	    //
	    processes()[current_process()].available_at = current_time();
	}

	if (action.event != 0)
	    if (--(action.event->refcount) == 0) 
		delete(action.event);
    }
    lock() = false;
    running() = false;
}

inline void Sim::set_stop_time(Time t) throw() {
    stop_time() = t;
}

inline void Sim::stop_process() throw() {
    SimImpl::schedule_now(A_Stop, current_process()); 
}

inline int Sim::stop_process(ProcessId pid) throw() {
    if (processes()[pid].terminated) return -1;
    SimImpl::schedule_now(A_Stop, pid); 
    return 0;
}

inline void Sim::stop_simulation() throw() {
    running() = false;
}

inline void Sim::advance_delay(Time delay) throw() {
    if (!running()) return;
    current_time() += delay;
}

inline ProcessId Sim::this_process() throw() {
    return current_process();
}

inline Time Sim::clock() throw() {
    return current_time();
}

inline void Sim::self_signal_event(const Event * e) throw() {
    SimImpl::schedule_now(A_Event, current_process(), e);
}

inline void Sim::self_signal_event(const Event * e, Time d) throw() {
    SimImpl::schedule(d, A_Event, current_process(), e);
}

inline void Sim::signal_event(ProcessId pid, const Event * e) throw() {
    SimImpl::schedule_now(A_Event, pid, e);
}

inline void Sim::signal_event(ProcessId pid, const Event * e, Time d) throw() {
    SimImpl::schedule(d, A_Event, pid, e);
}

inline void Sim::set_error_handler(SimErrorHandler * eh) throw() {
  *error_handler() = *eh;
}

inline ProcessId ProcessWithPId::activate() throw() {
    if (process_id == NULL_PROCESSID) {
	return process_id = Sim::create_process(this);
    } else {
	return NULL_PROCESSID;
    }
}

inline ProcessWithPId::ProcessWithPId() throw(): process_id(NULL_PROCESSID) {}

inline ProcessId ProcessWithPId::pid() const throw() {
    return process_id;
}

} // end namespace ssim

#endif /* _ssim_h */

