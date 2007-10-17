#ifndef __awgn_h
#define __awgn_h

#include "config.h"
#include "vcs.h"
#include "channel.h"
#include "itfunc.h"
#include "serializer.h"
#include <math.h>

/*
  Version 1.10 (15 Apr 1999)
  Changed the definition of set_snr to avoid using the pow() function.
  This was causing an unexplained SEGV with optimised code

  Version 1.11 (6 Mar 2002)
  changed vcs version variable from a global to a static class variable.

  Version 1.20 (13 Mar 2002)
  updated the system to conform with the completed serialization protocol (in conformance
  with channel 1.10), by adding the necessary name() function, and also by adding a static
  serializer member and initialize it with this class's name and the static constructor
  (adding that too). Also made the channel object a public base class, rather than a
  virtual public one, since this was affecting the transfer of virtual functions within
  the class (causing access violations). Also moved most functions into the implementation
  file rather than here.

  Version 1.30 (27 Mar 2002)
  changed descriptive output function to conform with channel 1.30.

  Version 1.40 (30 Oct 2006)
  * defined class and associated data within "libcomm" namespace.

  Version 1.50 (16 Oct 2007)
  changed class to conform with channel 1.50.

  Version 1.51 (16 Oct 2007)
  changed class to conform with channel 1.51.

  Version 1.52 (17 Oct 2007)
  changed class to conform with channel 1.52.
*/

namespace libcomm {

class awgn : public channel {
   static const libbase::vcs version;
   static const libbase::serializer shelper;
   static void* create() { return new awgn; };
   // channel paremeters
   double		sigma;
protected:
   // handle functions
   void compute_parameters(const double Eb, const double No);
   // channel handle functions
   sigspace corrupt(const sigspace& s);
public:
   // object handling
   channel *clone() const { return new awgn(*this); };
   const char* name() const { return shelper.name(); };

   // channel functions
   double pdf(const sigspace& tx, const sigspace& rx) const;

   // description output
   std::string description() const;
};

}; // end namespace

#endif

