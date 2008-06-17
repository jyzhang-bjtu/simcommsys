#ifndef __experiment_h
#define __experiment_h

#include "config.h"
#include "vector.h"
#include "serializer.h"
#include "random.h"

#include <iostream>
#include <string>

namespace libcomm {

/*!
   \brief   Generic experiment.
   \author  Johann Briffa

   \par Version Control:
   - $Revision$
   - $Date$
   - $Author$
*/

class experiment {
private:
   /*! \name Internal variables */
   libbase::int64u   samplecount;               //!< Number of samples accumulated
   // @}

protected:
   /*! \name Result accumulator interface */
   /*!
      \brief Reset accumulated results
   */
   virtual void derived_reset() = 0;
   /*!
      \brief Add the given sample results to the accumulated set
      \param[in] result   Vector containing a set of results
   */
   virtual void derived_accumulate(const libbase::vector<double>& result) = 0;
   /*!
      \brief Add the complete state of results to the accumulated set
      \param[in] state Vector set of accumulated results 
   */
   virtual void accumulate_state(const libbase::vector<double>& state) = 0;
   // @}

public:
   /*! \name Constructors / Destructors */
   virtual ~experiment() {};
   // @}

   /*! \name Experiment parameter handling */
   //! Seeds any random generators from a pseudo-random sequence
   virtual void seedfrom(libbase::random& r) = 0;
   //! Set the system parameter at which we want to simulate
   virtual void set_parameter(double x) = 0;
   //! Get the system parameter at which we are simulating
   virtual double get_parameter() = 0;
   // @}

   /*! \name Experiment handling */
   /*!
      \brief Perform the experiment and return a single sample
      \param[out] result   The set of results for the experiment
   */
   virtual void sample(libbase::vector<double>& result) = 0;
   /*!
      \brief The number of elements making up a sample
      \note This getter is likely to be redundant, as the value may be
            easily obtained from the size of result in sample()
      \callergraph
   */
   virtual int count() const = 0;
   /*!
      \brief Title/description of result at index 'i'
   */
   virtual std::string result_description(int i) const = 0;
   /*!
      \brief Return the simulated event from the last sample
      \return An experiment-specific description of the last event
   */
   virtual libbase::vector<int> get_event() const = 0;
   // @}

   /*! \name Result accumulator interface */
   /*!
      \brief Reset accumulated results
   */
   void reset() { samplecount = 0; derived_reset(); };
   /*!
      \brief Add the given sample results to the accumulated set
      \param[in] result   Vector containing a set of results
   */
   void accumulate(const libbase::vector<double>& result)
      { samplecount++; derived_accumulate(result); };
   /*!
      \brief Add the complete state of results to the accumulated set
      \param[in] samplecount The number of samples in the accumulated set
      \param[in] state Vector set of accumulated results 
   */
   void accumulate_state(libbase::int64u samplecount, const libbase::vector<double>& state)
      { this->samplecount += samplecount; accumulate_state(state); };
   /*!
      \brief Get the complete state of accumulated results
      \param[out] state Vector set of accumulated results 
   */
   virtual void get_state(libbase::vector<double>& state) const = 0;
   /*!
      \brief Determine result estimate based on accumulated set
      \param[out] estimate Vector containing the set of estimates
      \param[out] stderror Vector containing the corresponding standard error
   */
   virtual void estimate(libbase::vector<double>& estimate, libbase::vector<double>& stderror) const = 0;
   /*!
      \brief The number of samples taken to produce the result
   */
   libbase::int64u get_samplecount() const { return samplecount; };
   /*!
      \brief The number of samples taken to produce result 'i'
   */
   libbase::int64u get_samplecount(int i) const { return get_samplecount() * get_multiplicity(i); };
   /*!
      \brief The number of elements/sample for result 'i'
      \param[in]  i  Result index
      \return        Population size per sample for given result index
   */
   virtual int get_multiplicity(int i) const = 0;
   // @}

   /*! \name Description */
   //! Human-readable experiment description
   virtual std::string description() const = 0;
   // @}

   // Serialization Support
   DECLARE_BASE_SERIALIZER(experiment)
};

/*!
   \brief   Experiment with normally distributed samples.
   \author  Johann Briffa

   \par Version Control:
   - $Revision$
   - $Date$
   - $Author$

   Implements the accumulator functions required by the experiment class,
   moved from previous implementation in montecarlo.
*/

class experiment_normal : public experiment {
   /*! \name Internal variables */
   libbase::vector<double> sum;     //!< Vector of result sums
   libbase::vector<double> sumsq;   //!< Vector of result sum-of-squares
   // @}

protected:
   // Accumulator functions
   void derived_reset();
   void derived_accumulate(const libbase::vector<double>& result);
   void accumulate_state(const libbase::vector<double>& state);
   // @}

public:
   // Accumulator functions
   void get_state(libbase::vector<double>& state) const;
   void estimate(libbase::vector<double>& estimate, libbase::vector<double>& stderror) const;
};

/*!
   \brief   Experiment for estimation of a binomial proportion.
   \author  Johann Briffa

   \par Version Control:
   - $Revision$
   - $Date$
   - $Author$

   Implements the accumulator functions required by the experiment class.
*/

class experiment_binomial : public experiment {
   /*! \name Internal variables */
   libbase::vector<double> sum;     //!< Vector of result sums
   // @}

protected:
   // Accumulator functions
   void derived_reset();
   void derived_accumulate(const libbase::vector<double>& result);
   void accumulate_state(const libbase::vector<double>& state);
   // @}

public:
   // Accumulator functions
   void get_state(libbase::vector<double>& state) const;
   void estimate(libbase::vector<double>& estimate, libbase::vector<double>& stderror) const;
};

}; // end namespace

#endif
