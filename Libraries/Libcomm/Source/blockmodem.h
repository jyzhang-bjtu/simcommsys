#ifndef __blockmodem_h
#define __blockmodem_h

#include "config.h"
#include "serializer.h"
#include "sigspace.h"
#include "vector.h"
#include "matrix.h"
#include "channel.h"
#include "blockprocess.h"
#include <iostream>
#include <string>

namespace libcomm {

/*!
   \brief   Common Blockwise Modulator Interface.
   \author  Johann Briffa

   \par Version Control:
   - $Revision$
   - $Date$
   - $Author$

   Class defines common interface for blockmodem classes.

   \todo Templatize with respect to the type used for the likelihood table

   \todo Separate block size definition from this class
*/

template <class S, template<class> class C=libbase::vector>
class basic_blockmodem : public blockprocess {
public:
   /*! \name Type definitions */
   typedef libbase::vector<double>     array1d_t;
   // @}
private:
   /*! \name User-defined parameters */
   int tau;    //!< Block size in symbols
   // @}

protected:
   /*! \name Interface with derived classes */
   //! Setup function, called from set_blocksize()
   virtual void setup() {};
   //! \copydoc modulate()
   virtual void domodulate(const int N, const C<int>& encoded, C<S>& tx) = 0;
   //! \copydoc demodulate()
   virtual void dodemodulate(const channel<S>& chan, const C<S>& rx, C<array1d_t>& ptable) = 0;
   // @}

public:
   /*! \name Constructors / Destructors */
   //! Default constructor
   basic_blockmodem() { tau = 0; };
   //! Virtual destructor
   virtual ~basic_blockmodem() {};
   // @}

   /*! \name Atomic modem operations */
   /*!
      \brief Modulate a single time-step
      \param   index Index into the symbol alphabet
      \return  Symbol corresponding to the given index
   */
   virtual const S modulate(const int index) const = 0;
   /*!
      \brief Demodulate a single time-step
      \param   signal   Received signal
      \return  Index corresponding symbol that is closest to the received signal
   */
   virtual const int demodulate(const S& signal) const = 0;
   /*! \copydoc modulate() */
   const S operator[](const int index) const { return modulate(index); };
   /*! \copydoc demodulate() */
   const int operator[](const S& signal) const { return demodulate(signal); };
   // @}

   /*! \name Vector modem operations */
   /*!
      \brief Modulate a sequence of time-steps
      \param[in]  N        The number of possible values of each encoded element
      \param[in]  encoded  Sequence of values to be modulated
      \param[out] tx       Sequence of symbols corresponding to the given input

      \todo Remove parameter N, replacing 'int' type for encoded vector with
            something that also encodes the number of symbols in the alphabet.

      \note This function is non-const, to support time-variant modulation
            schemes such as DM inner codes.
   */
   void modulate(const int N, const C<int>& encoded, C<S>& tx);
   /*!
      \brief Demodulate a sequence of time-steps
      \param[in]  chan     The channel model (used to obtain likelihoods)
      \param[in]  rx       Sequence of received symbols
      \param[out] ptable   Table of likelihoods of possible transmitted symbols

      \note \c ptable(i)(d) \c is the a posteriori probability of having
            transmitted symbol 'd' at time 'i'

      \note This function is non-const, to support time-variant modulation
            schemes such as DM inner codes.
   */
   void demodulate(const channel<S>& chan, const C<S>& rx, C<array1d_t>& ptable);
   // @}

   /*! \name Setup functions */
   //! Seeds any random generators from a pseudo-random sequence
   virtual void seedfrom(libbase::random& r) {};
   //! Sets input block size
   void set_blocksize(int tau) { assert(tau > 0); basic_blockmodem::tau = tau; setup(); };
   // @}

   /*! \name Informative functions */
   //! Symbol alphabet size at input
   virtual int num_symbols() const = 0;
   //! Gets input block size
   int input_block_size() const { return tau; };
   //! Gets output block size
   virtual int output_block_size() const { return tau; };
   // @}

   /*! \name Description */
   //! Description output
   virtual std::string description() const = 0;
   // @}
};

/*!
   \brief   Blockwise Modulator Base.
   \author  Johann Briffa

   \par Version Control:
   - $Revision$
   - $Date$
   - $Author$
*/

template <class S>
class blockmodem : public basic_blockmodem<S> {
   // Serialization Support
   DECLARE_BASE_SERIALIZER(blockmodem);
};

/*!
   \brief   Signal-Space Modulator Specialization.
   \author  Johann Briffa

   \par Version Control:
   - $Revision$
   - $Date$
   - $Author$

   \version 1.00 (24 Jan 2008)
   - Elements specific to the signal-space channel moved to this implementation
     derived from the abstract class.
*/

template <>
class blockmodem<sigspace> : public basic_blockmodem<sigspace> {
public:
   /*! \name Informative functions */
   //! Average energy per symbol
   virtual double energy() const = 0;
   //! Average energy per bit
   double bit_energy() const { return energy()/log2(num_symbols()); };
   //! Modulation rate (spectral efficiency) in bits/unit energy
   double rate() const { return 1.0/bit_energy(); };
   // @}

   // Serialization Support
   DECLARE_BASE_SERIALIZER(blockmodem);
};

/*!
   \brief   Q-ary Blockwise Modulator.
   \author  Johann Briffa

   \par Version Control:
   - $Revision$
   - $Date$
   - $Author$

   Specific implementation of q-ary channel modulation.

   \note Template argument class must provide a method elements() that returns
         the field size.

   \todo Merge modulate and demodulate between this function and lut_modulator
*/

template <class G>
class direct_blockmodem : public blockmodem<G> {
public:
   /*! \name Type definitions */
   typedef libbase::vector<int>        array1i_t;
   typedef libbase::vector<G>          array1g_t;
   typedef libbase::vector<double>     array1d_t;
   typedef libbase::vector<array1d_t>  array1vd_t;
   // @}
protected:
   // Interface with derived classes
   void domodulate(const int N, const array1i_t& encoded, array1g_t& tx);
   void dodemodulate(const channel<G>& chan, const array1g_t& rx, array1vd_t& ptable);

public:
   // Atomic modem operations
   const G modulate(const int index) const { assert(index >= 0 && index < num_symbols()); return G(index); };
   const int demodulate(const G& signal) const { return signal; };

   // Informative functions
   int num_symbols() const { return G::elements(); };

   // Description
   std::string description() const;

   // Serialization Support
   DECLARE_SERIALIZER(direct_blockmodem);
};

/*!
   \brief   Binary Blockwise Modulator.
   \author  Johann Briffa

   \par Version Control:
   - $Revision$
   - $Date$
   - $Author$

   Specific implementation of binary channel modulation.
*/

template <>
class direct_blockmodem<bool> : public blockmodem<bool> {
public:
   /*! \name Type definitions */
   typedef libbase::vector<int>        array1i_t;
   typedef libbase::vector<bool>       array1b_t;
   typedef libbase::vector<double>     array1d_t;
   typedef libbase::vector<array1d_t>  array1vd_t;
   // @}
protected:
   // Interface with derived classes
   void domodulate(const int N, const array1i_t& encoded, array1b_t& tx);
   void dodemodulate(const channel<bool>& chan, const array1b_t& rx, array1vd_t& ptable);

public:
   // Atomic modem operations
   const bool modulate(const int index) const { assert(index >= 0 && index <= 1); return index & 1; };
   const int demodulate(const bool& signal) const { return signal; };

   // Informative functions
   int num_symbols() const { return 2; };

   // Description
   std::string description() const;

   // Serialization Support
   DECLARE_SERIALIZER(direct_blockmodem);
};

}; // end namespace

#endif
