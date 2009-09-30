#ifndef __bsid_h
#define __bsid_h

#include "config.h"
#include "channel.h"
#include "itfunc.h"
#include "serializer.h"
#include "multi_array.h"
#include <math.h>

namespace libcomm {

/*!
 * \brief   Binary substitution/insertion/deletion channel.
 * \author  Johann Briffa
 *
 * \section svn Version Control
 * - $Revision$
 * - $Date$
 * - $Author$
 */

class bsid : public channel<bool> {
public:
   /*! \name Type definitions */
   typedef boost::assignable_multi_array<double, 2> array2d_t;
   typedef libbase::vector<bool> array1b_t;
   typedef libbase::vector<double> array1d_t;
   typedef libbase::vector<array1d_t> array1vd_t;
   // @}
private:
   /*! \name User-defined parameters */
   bool biased; //!< Flag to indicate old-style bias against single-deletion
   bool varyPs; //!< Flag to indicate that \f$ P_s \f$ should change with parameter
   bool varyPd; //!< Flag to indicate that \f$ P_d \f$ should change with parameter
   bool varyPi; //!< Flag to indicate that \f$ P_i \f$ should change with parameter
   // @}
   /*! \name Channel-state parameters */
   double Ps; //!< Bit-substitution probability \f$ P_s \f$
   double Pd; //!< Bit-deletion probability \f$ P_d \f$
   double Pi; //!< Bit-insertion probability \f$ P_i \f$
   int N; //!< Block size in bits over which we want to synchronize
   // @}
   /*! \name Pre-computed parameters */
   int I; //!< Assumed limit for insertions between two time-steps
   int xmax; //!< Assumed maximum drift over a whole \c N -bit block
   array2d_t Rtable; //!< Receiver coefficient set for mu >= 0
   double Rval; //!< Receiver coefficient value for mu = -1
   // @}
public:
   /*! \name FBA decoder parameter computation */
   static int compute_I(int tau, double p);
   static int compute_xmax(int tau, double p, int I);
   static int compute_xmax(int tau, double p);
   static void compute_Rtable(array2d_t& Rtable, int xmax, double Ps,
         double Pd, double Pi);
   // @}
private:
   /*! \name Internal functions */
   void precompute();
   void init();
   // @}
protected:
   // Channel function overrides
   bool corrupt(const bool& s);
   double pdf(const bool& tx, const bool& rx) const;
public:
   /*! \name Constructors / Destructors */
   bsid(const bool varyPs = true, const bool varyPd = true, const bool varyPi =
         true, const bool biased = false);
   // @}

   /*! \name Channel parameter handling */
   void set_parameter(const double p);
   double get_parameter() const;
   // @}

   /*! \name Channel parameter setters */
   //! Set the bit-substitution probability
   void set_ps(const double Ps);
   //! Set the bit-deletion probability
   void set_pd(const double Pd);
   //! Set the bit-insertion probability
   void set_pi(const double Pi);
   //! Set the block size
   void set_blocksize(int N);
   // @}

   /*! \name Channel parameter getters */
   //! Get the current bit-substitution probability
   double get_ps() const
      {
      return Ps;
      }
   //! Get the current bit-deletion probability
   double get_pd() const
      {
      return Pd;
      }
   //! Get the current bit-insertion probability
   double get_pi() const
      {
      return Pi;
      }
   // @}

   // Channel functions
   void transmit(const array1b_t& tx, array1b_t& rx);
   void
         receive(const array1b_t& tx, const array1b_t& rx, array1vd_t& ptable) const;
   double receive(const array1b_t& tx, const array1b_t& rx) const;
   double receive(const bool& tx, const array1b_t& rx) const;

   // Description
   std::string description() const;

   // Serialization Support
DECLARE_SERIALIZER(bsid);
};

inline double bsid::pdf(const bool& tx, const bool& rx) const
   {
   return (tx != rx) ? Ps : 1 - Ps;
   }

inline double bsid::receive(const bool& tx, const array1b_t& rx) const
   {
   // Compute sizes
   const int mu = rx.size() - 1;
   // If this was a deletion, it's a fixed value
   if (mu < 0)
      return Rval;
   // Otherwise return result from table
   return Rtable[tx != rx(mu)][mu];
   }

} // end namespace

#endif