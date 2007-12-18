#ifndef __montecarlo_h
#define __montecarlo_h

#include "config.h"

#include "timer.h"
#include "experiment.h"
#include "masterslave.h"

/*!
   \brief   Monte Carlo Estimator.
   \author  Johann Briffa

   \par Version Control:
   - $Revision$
   - $Date$
   - $Author$

   \version 1.10 (2 Sep 1999)
   added a hook for clients to know the number of frames simulated in a particular run.

   \version 1.11 (9 Sep 1999)
   changed min_passes to 128, since we observed that with 30 samples only, the distribution
   is still quite skewed.

   \version 1.12 (26 Oct 2001)
   added a virtual display function, to facilitate deriving from the class to produce a
   windowed GUI (by using custom display routines), and also added a virtual interrupt
   function to allow a derived class to stop the processing routine. Both functions are
   protected so they can only be called by the class itself or by derived classes.

   \version 1.13 (26 Oct 2001)
   added an empty creator object which doesn't initialise the simulator and doesn't bind
   it to a system - this is necessary for deriving classes from this. Also created separate
   initialisation and finalisation routines to allow re-use of the same montecarlo object.

   \version 1.14 (16 Nov 2001)
   added a virtual destructor.

   \version 1.15 (23 Feb 2002)
   added flushes to all end-of-line clog outputs, to clean up text user interface.

   \version 1.16 (6 Mar 2002)
   changed vcs version variable from a global to a static class variable.
   also changed use of iostream from global to std namespace.

   \version 1.17 (25 Mar 2002)
   changed min_passes to min_samples, to indicate that this is the number of minimum
   samples, not of minimum cycles, that are necessary to ensure a gaussian distribution.
   Note that in each pass, there will really be a number of samples, depending on how
   long each sample takes.

   \version 1.18 (27 Mar 2002)
   removed the timer display on clog for the estimate function.

   \version 1.20 (30 Oct 2006)
   - defined class and associated data within "libcomm" namespace.
   - removed use of "using namespace std", replacing by tighter "using" statements as needed.

   \version 1.30 (20 Apr 2007)
   - updated to use masterslave instead of cmpi class, to support socket-based model
   - names of private functions changed from child to slave, to reflect the model better

   \version 1.31 (24-25 Apr 2007)
   - added a timer element, used by the default display() routine, and also
    provided a public interface for use by clients.
   - updated slave initialization so that we also receive the code itself
   - refactored by splitting the slave init into two routines, one for the code
    and one for the SNR; this facilitates multiple simulations on the same code
   - updated to conform with the changes in masterslave 1.10, where the slave
    functions are no longer static, and the class is meant to be instantiated
    by derivation

   \version 1.32 (20 Nov 2007)
   - added timeout when waiting for an event from the slaves in estimate()
   - modified control structure in estimate() by removing the infinite loop when
    parallel execution is enabled; this means that each run of the outer loop does
    not necessarily mean that a new result set is available. This condition is
   versiond by a new variable within the loop.
   - modified ending-logic, so that:
    - all end conditions are clearly specified in the loop condition
    - a user interrupt now overrides everything, we don't even wait for slaves
      to finish any pending work
   - modified slave-handling logic, so that new work only gets posted if the sum
    of samples already aggregated and the number of pending slaves is less than
    the minimum threshold; this assumes that only one sample is returned per slave,
    and allows an earlier exit during fast result convergence in large clusters.
    Clearly this only applies when the actual accuracy has already been reached.

   \version 1.33 (26 Nov 2007)
   - optimized display routine, so that updates are rate-limited

   \version 1.34 (30 Nov 2007)
   - added error checking on assigning work to slaves

   \version 1.35 (17 Dec 2007)
   - added printing of seed when generated

   \version 1.40 (18 Dec 2007)
   - Updated according to the new definition of experiment::sample(), which only
     returns a single pass.
   - Extracted result accumulation to updateresults()
   - Extracted initialization of new slaves
   - Extracted getting idle slaves to work
   - Moved accumulation of results to a new function, separating the accumulation
     process from the actual updating of final results; modified updateresults()
     accordingly
   - Modified updateresults() so that it is no longer given the number of passes,
     but instead uses the (already updated) samplecount; also made this function
     const, as it should not be updating any class members.
   - Removed bail-out facility
   \todo Cause minimum granularity to be handled in slave_work, passing the
         requisite sample count and sum of squares accordingly.
*/

namespace libcomm {

class montecarlo : public libbase::masterslave {
   // constants
   static const int  min_samples;   // minimum number of samples to assume gaussian distribution
   // bound objects
   bool init;
   experiment *system;
   // internal variables
   double   cfactor;                   // factor dependent on confidence level
   double   accuracy;               // accuracy level required
   int      samplecount;            // number of samples taken to produce the result (updated by experiment module to allow for dynamic sampling)
   libbase::timer t;
   // slave processes
   void slave_getcode(void);
   void slave_getsnr(void);
   void slave_work(void);
   // their functors
   libbase::specificfunctor<montecarlo> *fgetcode, *fgetsnr, *fwork;
private:
   // helper functions
   void createfunctors(void);
   void destroyfunctors(void);
   // main estimator helper functions
   void accumulateresults(libbase::vector<double>& sum, libbase::vector<double>& sumsq, libbase::vector<double> est) const;
   double updateresults(libbase::vector<double>& result, libbase::vector<double>& tolerance, const libbase::vector<double>& sum, const libbase::vector<double>& sumsq) const;
   void initnewslaves(std::string systemstring);
   void workidleslaves(bool accuracy_reached);
protected:
   // overrideable user-interface functions
   virtual bool interrupt() { return false; };
   virtual void display(const int pass, const double cur_accuracy, const double cur_mean);
public:
   // constructor/destructor
   montecarlo(experiment *system);
   montecarlo();
   virtual ~montecarlo();
   // simulation initialization/finalization
   void initialise(experiment *system);
   void finalise();
   // simulation parameters
   void set_confidence(const double confidence);   // say, 0.95 => 95% probability
   void set_accuracy(const double accuracy);       // say, 0.10 => 10% of mean
   // simulation results
   int get_samplecount() { return samplecount; };  // returns the number of samples taken to produce the result
   // main process
   void estimate(libbase::vector<double>& result, libbase::vector<double>& tolerance);
   // information getters
   const libbase::timer& get_timer() { return t; }; 
};

}; // end namespace

#endif
