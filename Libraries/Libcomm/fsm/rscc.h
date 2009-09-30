#ifndef __rscc_h
#define __rscc_h

#include "ccbfsm.h"
#include "serializer.h"

namespace libcomm {

/*!
 * \brief   Recursive Systematic Convolutional Coder.
 * \author  Johann Briffa
 *
 * \section svn Version Control
 * - $Revision$
 * - $Date$
 * - $Author$
 */

class rscc : public ccbfsm {
protected:
   libbase::vector<int> determineinput(libbase::vector<int> input) const;
   libbase::bitfield determinefeedin(libbase::vector<int> input) const;
   rscc()
      {
      }
public:
   /*! \name Constructors / Destructors */
   rscc(libbase::matrix<libbase::bitfield> const &generator) :
      ccbfsm(generator)
      {
      }
   rscc(rscc const &x) :
      ccbfsm(x)
      {
      }
   ~rscc()
      {
      }
   // @}

   // FSM state operations (getting and resetting)
   void resetcircular(libbase::vector<int> zerostate, int n);

   // Description
   std::string description() const;

   // Serialization Support
DECLARE_SERIALIZER(rscc);
};

} // end namespace

#endif
