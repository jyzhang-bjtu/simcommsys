/*!
 * \file
 * 
 * Copyright (c) 2010 Johann A. Briffa
 * 
 * This file is part of SimCommSys.
 *
 * SimCommSys is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimCommSys is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SimCommSys.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * \section svn Version Control
 * - $Id$
 */

#ifndef __errors_levenshtein_h
#define __errors_levenshtein_h

#include "config.h"
#include "errors_hamming.h"

namespace libcomm {

/*!
 * \brief   CommSys Results - SER (Hamming & Levenshtein), FER.
 * \author  Johann Briffa
 *
 * \section svn Version Control
 * - $Revision$
 * - $Date$
 * - $Author$
 *
 * Implements error rate calculators for SER (using both Hamming and
 * Levenshtein distances) and FER.
 */
class errors_levenshtein : public errors_hamming {
public:
   /*! \name Public interface */
   void updateresults(libbase::vector<double>& result, const int i,
         const libbase::vector<int>& source,
         const libbase::vector<int>& decoded) const;
   /*! \copydoc experiment::count()
    * For each iteration, we count the number of symbol errors using
    * Hamming and Levenshtein metrics, as well as the number of frame errors
    */
   int count() const
      {
      return 3 * get_iter();
      }
   int get_multiplicity(int i) const;
   std::string result_description(int i) const;
   // @}
};

} // end namespace

#endif