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
};

map<string, vector<double> > report;

/** 
    Initialise a simulation run for an individual
 */
void SimplePerson::init() {
  state = Healthy;
  scheduleAt(R::rweibull(8.0,85.0),toOtherDeath);
  scheduleAt(R::rweibull(3.0,90.0),toCancer);
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
    if (R::runif(0.0,1.0) < 0.5)
      scheduleAt(now() + R::rweibull(2.0,10.0), toCancerDeath);
    break;
  
  default:
    REprintf("No valid kind of event\n");
    break;
    
  } // switch

} // handleMessage()


RcppExport SEXP callSimplePerson(SEXP parms) {
  SimplePerson person;
  Rcpp::RNGScope scope;
  Rcpp::List parmsl(parms);
  int n = Rcpp::as<int>(parmsl["n"]);
  for (int i = 0; i < n; i++) {
    person = SimplePerson(i);
    Sim::create_process(&person);
    Sim::run_simulation();
    Sim::clear();
  }
  return Rcpp::wrap(report);
} 
 
