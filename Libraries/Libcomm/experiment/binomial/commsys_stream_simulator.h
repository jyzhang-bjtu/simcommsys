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

#ifndef __commsys_stream_simulator_h
#define __commsys_stream_simulator_h

#include "config.h"
#include "commsys_simulator.h"
#include <list>

namespace libcomm {

/*!
 * \brief   Communication System Simulator - for stream-oriented modulator.
 * \author  Johann Briffa
 *
 * \section svn Version Control
 * - $Revision$
 * - $Date$
 * - $Author$
 *
 * A variation on the regular commsys_simulator object, making use of the
 * stream-oriented modulator interface additions.
 *
 * This simulates stream transmission and reception by keeping track of the
 * previous and next frame information. Except for the first frame (where the
 * start position is known exactly) the a-priori information for start-of-frame
 * position is set to the posterior information of the end-of-frame from the
 * previous frame simulation. The a-priori end-of-frame information is set
 * according to the distribution provided by the channel.
 */
template <class S, class R>
class commsys_stream_simulator : public commsys_simulator<S, R> {
private:
   // Shorthand for class hierarchy
   typedef experiment Interface;
   typedef commsys_stream_simulator<S, R> This;
   typedef commsys_simulator<S, R> Base;
public:
   /*! \name Type definitions */
   //typedef libbase::vector<double> array1d_t;
   // @}

private:
   /*! \name User-defined parameters */
   int N; //!< maximum number of frames between resets (0=don't reset)
   // @}
   /*! \name Internally-used objects */
   std::list<libbase::vector<int> > source; //!< List of message sequences in order of transmission
   libbase::vector<S> received; //!< Received sequence as a stream
   libbase::vector<double> eof_post; //!< Centralized posterior probabilities at end-of-frame
   libbase::size_type<libbase::vector> offset; //!< Index offset for eof_post
   int estimated_drift; //!< Estimated drift in last decoded frame
   std::list<int> actual_drift; //!< Actual channel drift at end of each received frame
   int drift_error; //!< Cumulative error in channel drift estimation at end of last decoded frame
   commsys<S>* sys_enc; //!< Copy of the commsys object for encoder operations
   int frames_encoded; //!< Number of frames encoded since stream reset
   int frames_decoded; //!< Number of frames decoded since stream reset
   // @}

protected:
   /*! \name Setup functions */
   /*!
    * \brief Prepares to simulate a new sequence
    * This method clears the internal state and makes a copy of the base
    * commsys object to use for the transmission path.
    */
   void reset()
      {
      // clear internal state
      source.clear();
      received.init(0);
      eof_post.init(0);
      offset = libbase::size_type<libbase::vector>(0);
      estimated_drift = 0;
      // reset drift trackers
      actual_drift.clear();
      drift_error = 0;
      // Make a copy of the commsys object for transmitter operations
      delete sys_enc;
      if (this->sys)
         sys_enc = dynamic_cast<commsys<S>*> (this->sys->clone());
      else
         sys_enc = NULL;
      // reset counters
      frames_encoded = 0;
      frames_decoded = 0;
      }
   // @}

public:
   /*! \name Constructors / Destructors */
   commsys_stream_simulator(const commsys_stream_simulator<S, R>& c) :
      commsys_simulator<S, R> (c), N(c.N), source(c.source), received(
            c.received), eof_post(c.eof_post), offset(c.offset),
            estimated_drift(c.estimated_drift), actual_drift(c.actual_drift),
            drift_error(c.drift_error), frames_encoded(c.frames_encoded),
            frames_decoded(c.frames_decoded)
      {
      sys_enc = dynamic_cast<commsys<S>*> (c.sys_enc->clone());
      }
   commsys_stream_simulator() :
      N(0), sys_enc(NULL)
      {
      reset();
      }
   virtual ~commsys_stream_simulator()
      {
      delete sys_enc;
      }
   // @}

   /*! \name Communication System Setup */
   void seedfrom(libbase::random& r)
      {
      Base::seedfrom(r);
      reset();
      }
   void set_parameter(const double x)
      {
      Base::set_parameter(x);
      // we should already have a copy at this point
      assert(sys_enc);
      sys_enc->getchan()->set_parameter(x);
      }
   // @}

   // Experiment handling
   void sample(libbase::vector<double>& result);

   // Description
   std::string description() const;

   // Serialization Support
DECLARE_SERIALIZER(commsys_stream_simulator)
};

} // end namespace

#endif