/*!
 * \file
 * 
 * \section svn Version Control
 * - $Revision$
 * - $Date$
 * - $Author$
 */

#include "uncoded.h"
#include "vectorutils.h"
#include <sstream>

namespace libcomm {

// initialization / de-allocation

template <class dbl>
void uncoded<dbl>::init()
   {
   assertalways(encoder);
   // Check that FSM is memoryless
   assertalways(encoder->mem_order() == 0);
   }

template <class dbl>
void uncoded<dbl>::free()
   {
   if (encoder != NULL)
      delete encoder;
   }

// constructor / destructor

template <class dbl>
uncoded<dbl>::uncoded() :
   encoder(NULL)
   {
   }

template <class dbl>
uncoded<dbl>::uncoded(const fsm& encoder, const int tau) :
   tau(tau)
   {
   uncoded<dbl>::encoder = dynamic_cast<fsm*> (encoder.clone());
   init();
   }

// internal codec operations

template <class dbl>
void uncoded<dbl>::resetpriors()
   {
   // Allocate space for prior input statistics
   libbase::allocate(rp, This::input_block_size(), This::num_inputs());
   // Initialize
   rp = 1.0;
   }

template <class dbl>
void uncoded<dbl>::setpriors(const array1vd_t& ptable)
   {
   // Encoder symbol space must be the same as modulation symbol space
   assertalways(ptable.size() > 0);
   assertalways(ptable(0).size() == This::num_inputs());
   // Confirm input sequence to be of the correct length
   assertalways(ptable.size() == This::input_block_size());
   // Copy the input statistics
   rp = ptable;
   }

template <class dbl>
void uncoded<dbl>::setreceiver(const array1vd_t& ptable)
   {
   // Encoder symbol space must be the same as modulation symbol space
   assertalways(ptable.size() > 0);
   assertalways(ptable(0).size() == This::num_outputs());
   // Confirm input sequence to be of the correct length
   assertalways(ptable.size() == This::output_block_size());
   // Copy the output statistics
   R = ptable;
   }

// encoding and decoding functions

template <class dbl>
void uncoded<dbl>::encode(const array1i_t& source, array1i_t& encoded)
   {
   assert(source.size() == This::input_block_size());
   // Inherit sizes
   const int k = enc_inputs();
   const int n = enc_outputs();
   // Initialise result vector
   encoded.init(This::output_block_size());
   // Encode source stream
   for (int t = 0; t < tau; t++)
      {
      // NOTE: do not replace with a copy constructor
      array1i_t ip;
      ip = source.extract(t * k, k);
      encoded.segment(t * n, n) = encoder->step(ip);
      }
   }

template <class dbl>
void uncoded<dbl>::softdecode(array1vd_t& ri)
   {
   // Inherit sizes
   const int k = enc_inputs();
   const int n = enc_outputs();
   const int S = This::num_inputs();
   // Initialize results to prior statistics
   ri = rp;
   // Consider each time-step
   for (int t = 0; t < tau; t++)
      {
      array1i_t ip(k), op(n);
      // Go through each possible input set
      ip = 0;
      for (int i = 0; i < k; i++)
         for (int x = 0; x < S; x++)
            {
            // Update input set
            ip(i) = x;
            // Determine corresponding output set
            op = encoder->step(ip);
            // Update probabilities at input according to those at output
            for (int ii = 0; ii < k; ii++)
               for (int oo = 0; oo < n; oo++)
                  ri(t * k + ii)(ip(ii)) *= R(t * n + oo)(op(oo));
            }
      }
   }

template <class dbl>
void uncoded<dbl>::softdecode(array1vd_t& ri, array1vd_t& ro)
   {
   softdecode(ri);
   // Inherit sizes
   const int k = enc_inputs();
   const int n = enc_outputs();
   const int S = This::num_inputs();
   // Allocate space for output results
   libbase::allocate(ro, This::output_block_size(), This::num_outputs());
   // Initialize
   ro = 1.0;
   // Consider each time-step
   for (int t = 0; t < tau; t++)
      {
      array1i_t ip(k), op(n);
      // Go through each possible input set
      ip = 0;
      for (int i = 0; i < k; i++)
         for (int x = 0; x < S; x++)
            {
            // Update input set
            ip(i) = x;
            // Determine corresponding output set
            op = encoder->step(ip);
            // Update probabilities at output according to those at input
            for (int ii = 0; ii < k; ii++)
               for (int oo = 0; oo < n; oo++)
                  ro(t * n + oo)(op(oo)) *= ri(t * k + ii)(ip(ii));
            }
      }
   }

// description output

template <class dbl>
std::string uncoded<dbl>::description() const
   {
   std::ostringstream sout;
   sout << "Uncoded/Repetition Code (" << This::output_bits() << ","
         << This::input_bits() << ") - ";
   sout << encoder->description();
   return sout.str();
   }

// object serialization - saving

template <class dbl>
std::ostream& uncoded<dbl>::serialize(std::ostream& sout) const
   {
   sout << encoder;
   sout << tau << "\n";
   return sout;
   }

// object serialization - loading

template <class dbl>
std::istream& uncoded<dbl>::serialize(std::istream& sin)
   {
   free();
   sin >> libbase::eatcomments >> encoder;
   sin >> libbase::eatcomments >> tau;
   init();
   return sin;
   }

} // end namespace

// Explicit Realizations

#include "logrealfast.h"

namespace libcomm {

using libbase::logrealfast;

using libbase::serializer;

template class uncoded<float> ;
template <>
const serializer uncoded<float>::shelper = serializer("codec",
      "uncoded<float>", uncoded<float>::create);

template class uncoded<double> ;
template <>
const serializer uncoded<double>::shelper = serializer("codec",
      "uncoded<double>", uncoded<double>::create);

template class uncoded<logrealfast> ;
template <>
const serializer uncoded<logrealfast>::shelper = serializer("codec",
      "uncoded<logrealfast>", uncoded<logrealfast>::create);

} // end namespace
