#include "microsimulation.h"
#include <Rcpp.h>

using namespace std;

enum state_t {Healthy,Cancer,Death};

enum event_t {toOtherDeath, toCancer, toCancerDeath};

class SimplePerson : public cProcess 
{
public:
  state_t state;
  int id;
  SimplePerson(const int i = 0) : id(i) {};
  void init();
  virtual void handleMessage(const cMessage* msg);
  // static map<string, vector<double> > report;
};

map<string, vector<double> > report;

/** 
    Initialise a simulation run for an individual
 */
void SimplePerson::init() {
  state = Healthy;
  if (R::runif(0.0,1.0)<0.2) 
    scheduleAt(R::rweibull(10,65),toCancer);
  scheduleAt(R::rexp(80.0),toOtherDeath);
}

#define Reporting(name,value) report[name].push_back(value);

/** 
    Handle receiving self-messages
 */
void SimplePerson::handleMessage(const cMessage* msg) {

  double dwellTime, pDx;

  Reporting("id",id);
  Reporting("startTime",previousEventTime);
  Reporting("endtime", now());
  Reporting("state", state);
  Reporting("event", msg->kind);

  switch(msg->kind) {

  case toOtherDeath: 
    Sim::stop_simulation();
    break;
    
  case toCancerDeath: 
    Sim::stop_simulation();
    break;
    
  case toCancer:
    state = Cancer;
    scheduleAt(R::rweibull(3.0,20.0), toCancerDeath);
    break;
  
  default:
    REprintf("No valid kind of event\n");
    break;
    
  } // switch

} // handleMessage()


extern "C" {

  RcppExport SEXP callSimplePerson(SEXP parms) {
    // // set the seed
    // unsigned long seed[6] = {12345,12345,12345,12345,12345,12345};
    // RngStream_SetPackageSeed(seed);
    // // declare the random number generator(s)
    // Rng * rng = new Rng();
    // declare a person
    SimplePerson person;
    // loop over persons
    for (int i = 0; i < 100; i++) {
      person = SimplePerson(i);
      Sim::create_process(&person);
      Sim::run_simulation();
      Sim::clear();
      // rng->nextSubstream();
    }
    // clean up
    // delete rng;
    // output arguments to R
    return Rcpp::wrap(report);
    
  } // callSimplePerson()
  
} // extern "C"
