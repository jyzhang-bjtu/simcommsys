/*!
 * \file
 * 
 * \section svn Version Control
 * - $Revision$
 * - $Date$
 * - $Author$
 */

#include "fsm.h"

namespace libcomm {

using libbase::vector;

const int fsm::tail = -1;

// Helper functions

/*!
 * \brief Conversion from vector spaces to integer
 * \param[in] x Input in vector representation
 * \param[in] S Alphabet size for vector symbols
 * \return Value of \c x in integer representation
 *
 * Left-most register positions (ie. those closest to the input junction) are
 * represented by lower index positions, and get lower-order positions within
 * the integer representation.
 *
 * \todo check we are within the acceptable range for int representation
 */
int fsm::convert(const vector<int>& vec, int S)
   {
   int nu = vec.size();
   int val = 0;
   for (int i = nu - 1; i >= 0; i--)
      {
      val *= S;
      val += vec(i);
      }
   return val;
   }

/*!
 * \brief Conversion from integer to vector space
 * \param[in] x Input in integer representation
 * \param[in] nu Length of vector representation
 * \param[in] S Alphabet size for vector symbols
 * \return Value of \c x in vector representation
 *
 * Left-most register positions (ie. those closest to the input junction) are
 * represented by lower index positions, and get lower-order positions within
 * the integer representation.
 */
vector<int> fsm::convert(int val, int nu, int S)
   {
   vector<int> vec(nu);
   for (int i = 0; i < nu; i++)
      {
      vec(i) = val % S;
      val /= S;
      }
   assert(val == 0);
   return vec;
   }

// FSM state operations

void fsm::reset()
   {
   N = 0;
   }

void fsm::reset(libbase::vector<int> state)
   {
   N = 0;
   }

void fsm::resetcircular()
   {
   resetcircular(state(), N);
   }

// FSM operations

void fsm::advance(libbase::vector<int>& input)
   {
   N++;
   }

libbase::vector<int> fsm::step(libbase::vector<int>& input)
   {
   libbase::vector<int> op = output(input);
   advance(input);
   return op;
   }

} // end namespace