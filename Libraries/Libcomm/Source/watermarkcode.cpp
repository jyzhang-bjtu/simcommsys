/*!
   \file

   \par Version Control:
   - $Revision$
   - $Date$
   - $Author$
*/

#include "watermarkcode.h"
#include "timer.h"
#include <sstream>

namespace libcomm {

// internally-used functions

/*!
   \brief Set up LUT with the lowest weight codewords
*/
template <class real> int watermarkcode<real>::fill(int i, libbase::bitfield suffix, int w)
   {
   // set up if this is the first (root) call
   if(i == 0 && w == -1)
      {
      assert(n >= 1 && n <= 32);
      assert(k >= 1 && k <= n);
      userspecified = false;
      lutname = "sequential";
      lut.init(num_symbols());
      suffix = "";
      w = n;
      }
   // stop here if we've reached the end
   if(i >= lut.size())
      return i;
   // otherwise, it all depends on the weight we're considering
   using libbase::bitfield;
   using libbase::trace;
   bitfield b;
   trace << "Starting fill with:\t" << suffix << "\t" << w << "\n";
   if(w == 0)
      lut(i++) = suffix;
   else
      {
      w--;
      if(suffix.size() == 0)
         i = fill(i,suffix,w);
      for(b="1"; b.size()+suffix.size()+w <= n; b=b+bitfield("0"))
         i = fill(i,b+suffix,w);
      }
   return i;
   }

//! Watermark sequence creator

template <class real> void watermarkcode<real>::createsequence(const int tau)
   {
   // creates 'tau' elements of 'n' bits each
   ws.init(tau);
   for(int i=0; i<tau; i++)
      ws(i) = r.ival(1<<n);
   }

//! Inform user if I or xmax have changed

template <class real> void watermarkcode<real>::checkforchanges(int I, int xmax)
   {
   static int last_I = 0;
   static int last_xmax = 0;
   if(last_I != I || last_xmax != xmax)
      {
      std::cerr << "Watermark Demodulation: I = " << I << ", xmax = " << xmax << ".\n";
      last_I = I;
      last_xmax = xmax;
      }
   }

// initialization / de-allocation

template <class real> void watermarkcode<real>::init()
   {
   using libbase::bitfield;
   using libbase::weight;
   using libbase::trace;
#ifndef NDEBUG
   // Display LUT when debugging
   trace << "LUT (k=" << k << ", n=" << n << "):\n";
   for(int i=0; i<lut.size(); i++)
      trace << i << "\t" << bitfield(lut(i),n) << "\t" << weight(lut(i)) << "\n";
#endif
   // Validate LUT
   assertalways(lut.size() == num_symbols());
   for(int i=0; i<lut.size(); i++)
      {
      // all entries should be within size
      assertalways(lut(i) >= 0 && lut(i) < (1<<n));
      // all entries should be distinct
      for(int j=0; j<i; j++)
         assertalways(lut(i) != lut(j));
      }
   // Compute the mean density
   libbase::vector<int> w = lut;
   w.apply(weight);
   f = w.sum()/double(n * w.size());
   trace << "Watermark code density = " << f << "\n";
   // Seed the watermark generator and clear the sequence
   r.seed(0);
   ws.init(0);
   }

// constructor / destructor

template <class real> watermarkcode<real>::watermarkcode() : mychan(1)
   {
   }

template <class real> watermarkcode<real>::watermarkcode(const int n, const int k, const int N, const bool varyPs, const bool varyPd, const bool varyPi) : mychan(N, varyPs, varyPd, varyPi)
   {
   // code parameters
   watermarkcode::n = n;
   watermarkcode::k = k;
   // initialize everything else that depends on the above parameters
   fill();
   init();
   }

// implementations of channel-specific metrics for fba

template <class real> real watermarkcode<real>::P(const int a, const int b)
   {
   const int m = b-a;
   return Ptable(m+1);
   }

template <class real> real watermarkcode<real>::Q(const int a, const int b, const int i, const libbase::vector<bool>& s)
   {
   // 'a' and 'b' are redundant because 's' already contains the difference
   assert(s.size() == b-a+1);
   // 'tx' is a matrix of all possible transmitted symbols
   // we know exactly what was transmitted at this timestep
   const int word = i/n;
   const int bit  = i%n;
   bool tx = ((ws(word) >> bit) & 1);
   // compute the conditional probability
   return mychan.receive(tx, s);
   }

// encoding and decoding functions

template <class real> void watermarkcode<real>::modulate(const int N, const libbase::vector<int>& encoded, libbase::vector<bool>& tx)
   {
   // Inherit sizes
   const int q = 1<<k;
   const int tau = encoded.size();
   // We assume that each 'encoded' symbol can be fitted in an integral number of sparse vectors
   const int p = int(round(log(double(N))/log(double(q))));
   assert(N == pow(q, p));
   // Initialise result vector (one bit per sparse vector) and watermark sequence
   tx.init(n*p*tau);
   createsequence(p*tau);
   // Encode source stream
   for(int i=0, ii=0; i<tau; i++)
      for(int j=0, x=encoded(i); j<p; j++, ii++, x >>= k)
         {
         const int s = lut(x & (q-1));    // sparse vector
         const int w = ws(ii);            // watermark vector
#ifndef NDEBUG
         if(tau < 10)
            {
            libbase::trace << "DEBUG (watermarkcode::modulate): word " << i << "\t";
            libbase::trace << "s = " << libbase::bitfield(s,n) << "\t";
            libbase::trace << "w = " << libbase::bitfield(w,n) << "\n";
            }
#endif
         // NOTE: we transmit the low-order bits first
         for(int bit=0, t=s^w; bit<n; bit++, t >>= 1)
            tx(ii*n+bit) = (t&1);
         }
   }

template <class real> void watermarkcode<real>::demodulate(const channel<bool>& chan, const libbase::vector<bool>& rx, libbase::matrix<double>& ptable)
   {
   using libbase::trace;
   // Inherit block size from last modulation step
   const int q = 1<<k;
   const int N = ws.size();
   const int tau = N*n;
   assert(N > 0);
   // Set channel parameters used in FBA same as one being simulated
   mychan.set_parameter(chan.get_parameter());
   const double Ps = mychan.get_ps();
   mychan.set_ps(Ps*(1-f) + (1-Ps)*f);
   // Determine required FBA parameter values
   const double Pd = mychan.get_pd();
   const int I = bsid::compute_I(tau, Pd);
   const int xmax = bsid::compute_xmax(tau, Pd, I);
   checkforchanges(I, xmax);
   // Pre-compute 'P' table
   bsid::compute_Ptable(Ptable, xmax, mychan.get_pd(), mychan.get_pi());
   // Initialize & perform forward-backward algorithm
   fba<real,bool>::init(tau, I, xmax);
   fba<real,bool>::prepare(rx);
   // Initialise result vector (one sparse symbol per timestep)
   ptable.init(N, q);
   libbase::matrix<real> p(N,q);
   p = real(0);
   // ptable(i,d) is the a posteriori probability of having transmitted symbol 'd' at time 'i'
   trace << "DEBUG (watermarkcode::demodulate): computing ptable...\n";
   for(int i=0; i<N; i++)
      {
      std::cerr << libbase::pacifier("WM Demodulate", i, N);
      // determine the strongest path at this point
      real threshold = 0;
      for(int x1=-xmax; x1<=xmax; x1++)
         if(fba<real,bool>::getF(n*i,x1) > threshold)
            threshold = fba<real,bool>::getF(n*i,x1);
      threshold *= 1e-6;
      for(int d=0; d<q; d++)
         {
         // create the considered transmitted sequence
         libbase::vector<bool> tx(n);
         for(int j=0, t=ws(i)^lut(d); j<n; j++, t >>= 1)
            tx(j) = (t&1);
         // In loop below skip out-of-bounds cases:
         // 1. first bit of received vector must exist: n*i+x1 >= 0
         // 2. last bit of received vector must exist: n*(i+1)+x2-1 <= rx.size()-1
         // 3. received vector size: x2-x1+n >= 0
         // 4. drift introduced in this section: -n <= x2-x1 <= n*I
         //    [first part corresponds to requirement 3]
         // 5. drift introduced in this section: -xmax <= x2-x1 <= xmax
         const int x1min = max(-xmax,-n*i);
         const int x1max = xmax;
         for(int x1=x1min; x1<=x1max; x1++)
            {
            const real F = fba<real,bool>::getF(n*i,x1);
            // ignore paths below a certain threshold
            if(F < threshold)
               continue;
            const int x2min = max(-xmax,x1-min(n,xmax));
            const int x2max = min(min(xmax,rx.size()-n*(i+1)),x1+min(n*I,xmax));
            for(int x2=x2min; x2<=x2max; x2++)
               {
               // compute the conditional probability
               const real P = chan.receive(tx, rx.extract(n*i+x1,x2-x1+n));
               const real B = fba<real,bool>::getB(n*(i+1),x2);
               // include the probability for this particular sequence
               p(i,d) += P * F * B;
               }
            }
         }
      }
   if(N > 0)
      std::cerr << libbase::pacifier("WM Demodulate", N, N);
   // normalize and copy results
   const real scale = p.max();
   // check for likely numerical underflow
   assert(scale != real(0));
   p /= scale;
   for(int i=0; i<N; i++)
      for(int d=0; d<q; d++)
         ptable(i,d) = p(i,d);
   trace << "DEBUG (watermarkcode::demodulate): ptable done.\n";
   }

// description output

template <class real> std::string watermarkcode<real>::description() const
   {
   std::ostringstream sout;
   sout << "Watermark Code (" << n << "/" << k << ", " << lutname << " codebook, [" << mychan.description() << "])";
   return sout.str();
   }

// object serialization - saving

template <class real> std::ostream& watermarkcode<real>::serialize(std::ostream& sout) const
   {
   sout << n << '\n';
   sout << k << '\n';
   sout << int(userspecified) << '\n';
   if(userspecified)
      {
      sout << lutname << '\n';
      assert(lut.size() == num_symbols());
      for(int i=0; i<lut.size(); i++)
         sout << libbase::bitfield(lut(i),n) << '\n';
      }
   mychan.serialize(sout);
   return sout;
   }

// object serialization - loading

template <class real> std::istream& watermarkcode<real>::serialize(std::istream& sin)
   {
   int temp;
   free();
   sin >> n;
   sin >> k;
   sin >> temp;
   userspecified = temp != 0;
   if(userspecified)
      {
      sin >> lutname;
      lut.init(num_symbols());
      libbase::bitfield b;
      for(int i=0; i<lut.size(); i++)
         {
         sin >> b;
         lut(i) = b;
         assertalways(n == b.size());
         }
      }
   else
      fill();
   mychan.serialize(sin);
   init();
   return sin;
   }

}; // end namespace

// Explicit Realizations

#include "logrealfast.h"

namespace libcomm {

using libbase::logrealfast;

using libbase::serializer;

template class watermarkcode<logrealfast>;
template <> const serializer watermarkcode<logrealfast>::shelper = serializer("modulator", "watermarkcode<logrealfast>", watermarkcode<logrealfast>::create);

}; // end namespace
