/*!
 * \file
 * 
 * \section svn Version Control
 * - $Revision$
 * - $Date$
 * - $Author$
 */

#include "awgn.h"

namespace libcomm {

const libbase::serializer awgn::shelper("channel", "awgn", awgn::create);

// handle functions

void awgn::compute_parameters(const double Eb, const double No)
   {
   sigma = sqrt(Eb * No);
   }

// channel handle functions

sigspace awgn::corrupt(const sigspace& s)
   {
   const double x = r.gval(sigma);
   const double y = r.gval(sigma);
   return s + sigspace(x, y);
   }

double awgn::pdf(const sigspace& tx, const sigspace& rx) const
   {
   sigspace n = rx - tx;
   using libbase::gauss;
   return gauss(n.i() / sigma) * gauss(n.q() / sigma);
   }

// Description

std::string awgn::description() const
   {
   return "AWGN channel";
   }

// Serialization Support

std::ostream& awgn::serialize(std::ostream& sout) const
   {
   return sout;
   }

std::istream& awgn::serialize(std::istream& sin)
   {
   return sin;
   }

} // end namespace