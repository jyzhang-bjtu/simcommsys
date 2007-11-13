#include "watermarkcode.h"
#include "timer.h"
#include <sstream>

namespace libcomm {

// internally-used functions

template <class real> int watermarkcode<real>::fill(int i, libbase::bitfield suffix, int w)
   {
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
   
template <class real> void watermarkcode<real>::createsequence(const int tau)
   {
   // creates 'tau' elements of 'n' bits each
   ws.init(tau);
   for(int i=0; i<tau; i++)
      ws(i) = r.ival(1<<n);
   }

// initialization / de-allocation

template <class real> void watermarkcode<real>::init()
   {
   using libbase::weight;
   using libbase::trace;
   // Create LUT with the lowest weight codewords
   lut.init(num_symbols());
   fill(0,"",n);
#ifndef NDEBUG
   // Display LUT when debugging
   trace << "LUT (k=" << k << ", n=" << n << "):\n";
   libbase::bitfield b;
   b.resize(n);
   for(int i=0; i<lut.size(); i++)
      {
      b = lut(i);
      trace << i << "\t" << b << "\t" << weight(b) << "\n";
      }
#endif
   // Compute the mean density
   libbase::vector<int> w = lut;
   w.apply(weight);
   f = w.sum()/double(n * w.size());
   trace << "Watermark code density = " << f << "\n";
   // Seed the watermark generator and clear the sequence
   r.seed(s);
   ws.init(0);
   // initialize the mpsk modulator & forward-backward algorithm
   mpsk::init(2);
   }

template <class real> void watermarkcode<real>::free()
   {
   }

// constructor / destructor

template <class real> watermarkcode<real>::watermarkcode() : mychan(0,0,0,0,0)
   {
   }

template <class real> watermarkcode<real>::watermarkcode(const int n, const int k, const int s, const int I, const int xmax, const bool varyPs, const bool varyPd, const bool varyPi) : mychan(I, xmax, varyPs, varyPd, varyPi)
   {
   // code parameters
   assert(n >= 1 && n <= 32);
   assert(k >= 1 && k <= n);
   watermarkcode::n = n;
   watermarkcode::k = k;
   watermarkcode::s = s;
   // decoder parameters
   assert(I > 0);
   assert(xmax > 0);
   watermarkcode::I = I;
   watermarkcode::xmax = xmax;
   // initialize everything else that depends on the above parameters
   init();
   }

// implementations of channel-specific metrics for fba

template <class real> double watermarkcode<real>::P(const int a, const int b)
   {
   const double Pd = mychan.get_pd();
   const double Pi = mychan.get_pi();
   const int m = b-a;
   if(m == -1)
      return Pd;
   else if(m >= 0)
      return pow(Pi,m)*(1-Pi)*(1-Pd);
   return 0;
   }
   
template <class real> double watermarkcode<real>::Q(const int a, const int b, const int i, const libbase::vector<sigspace>& s)
   {
   // 'a' and 'b' are redundant because 's' already contains the difference
   assert(s.size() == b-a+1);
   // 'tx' is a matrix of all possible transmitted symbols
   // we know exactly what was transmitted at this timestep
   libbase::matrix<sigspace> tx(1,1);
   const int word = i/n;
   const int bit  = i%n;
   tx(0,0) = mpsk::modulate((ws(word) >> bit) & 1);
   // compute the conditional probability
   libbase::matrix<double> ptable;
   mychan.receive(tx, s, ptable);
   return ptable(0,0);
   }
   
// encoding and decoding functions

template <class real> void watermarkcode<real>::modulate(const int N, const libbase::vector<int>& encoded, libbase::vector<sigspace>& tx)
   {
   // Inherit sizes
   const int q = 1<<k;
   const int tau = encoded.size();
   // We assume that each 'encoded' symbol can be fitted in an integral number of sparse vectors
   const int p = int(libbase::round(log(double(N))/log(double(q))));
   assert(N == pow(q, p));
   // Initialise result vector (one bit per sparse vector) and watermark sequence
   tx.init(n*p*tau);
   createsequence(p*tau);
   // Encode source stream
   // NOTE: we transmit the low-order bits first
   for(int i=0, ii=0; i<tau; i++)
      for(int j=0, x=encoded(i); j<p; j++, ii++, x >>= k)
         {
         const int s = lut(x & (q-1));    // sparse vector
         const int w = ws(ii);            // watermark vector
         for(int bit=0, t=s^w; bit<n; bit++, t >>= 1)
            tx(ii*n+j) = mpsk::modulate(t&1);
         }
   }

template <class real> void watermarkcode<real>::demodulate(const channel& chan, const libbase::vector<sigspace>& rx, libbase::matrix<double>& ptable)
   {
   using libbase::trace;
   // Inherit block size from last modulation step
   const int q = 1<<k;
   const int N = ws.size();
   assert(N > 0);
   // Set channel parameters used in FBA same as one being simulated
   mychan.set_eb(chan.get_eb());
   mychan.set_no(chan.get_no());
   const double Ps = mychan.get_ps();
   mychan.set_ps(Ps*(1-f) + (1-Ps)*f);
   // Initialize & perform forward-backward algorithm
   const int xmax = mychan.get_xmax();
   fba<real>::init(N*n, mychan.get_I(), xmax);
   fba<real>::prepare(rx);
   // Initialise result vector (one sparse symbol per timestep)
   ptable.init(N, q);
   // ptable(i,d) is the a posteriori probability of having transmitted symbol 'd' at time 'i'
   trace << "DEBUG (watermarkcode::demodulate): computing ptable...\n";
   for(int i=0; i<N; i++)
      {
      trace << libbase::pacifier(100*i/N);
      for(int d=0; d<q; d++)
         {
#ifndef NDEBUG
         libbase::timer t1;
#endif
         ptable(i,d) = 0;
         // In loop below skip out-of-bounds cases:
         // 1. received vector size: x2-x1+1 >= 0
         // 2. drift introduced in this section: abs(x2-x1) <= xmax
         // 3. first bit of received vector must exist: n*i+x1 >= 0
         // 4. last bit of received vector must exist: n*i+x2 < rx.size()
         for(int x1=max(-xmax,-n*i); x1<=xmax; x1++)
            for(int x2=max(-xmax,x1-1); x2<=min(min(xmax,x1+xmax),rx.size()-n*i-1); x2++)
               {
               // create received vector in consideration
               libbase::vector<sigspace> s(x2-x1+1);
               for(int j=n*i+x1, k=0; j<=n*i+x2; j++, k++)
                  s(k) = rx(j);
               // create the considered transmitted signal
               libbase::matrix<sigspace> tx(n,1);
               for(int j=0; j<n; j++)
                  tx(j,0) = mpsk::modulate(((ws(i)^d) >> j) & 1);
               // compute the conditional probability
               libbase::matrix<double> p;
               chan.receive(tx, s, p);
               // include the probability for this particular sequence
               const real F = fba<real>::getF(n*i,x1);
               const real B = fba<real>::getB(n*(i+1),x2);
               ptable(i,d) += p(0,0) * double(F * B);
               }
         }
      }
   trace << "DEBUG (watermarkcode::demodulate): ptable done.\n";
   }
   
// description output

template <class real> std::string watermarkcode<real>::description() const
   {
   std::ostringstream sout;
   sout << "Watermark Code (" << n << "," << k << "," << s << ",[" << mychan.description() << "])";
   return sout.str();
   }

// object serialization - saving

template <class real> std::ostream& watermarkcode<real>::serialize(std::ostream& sout) const
   {
   sout << n << "\n";
   sout << k << "\n";
   sout << s << "\n";
   mychan.serialize(sout);
   return sout;
   }

// object serialization - loading

template <class real> std::istream& watermarkcode<real>::serialize(std::istream& sin)
   {
   free();
   sin >> n;
   sin >> k;
   sin >> s;
   mychan.serialize(sin);
   init();
   return sin;
   }

}; // end namespace

// Explicit Realizations

#include "mpreal.h"
#include "mpgnu.h"
#include "logreal.h"
#include "logrealfast.h"

namespace libcomm {

using libbase::mpreal;
using libbase::mpgnu;
using libbase::logreal;
using libbase::logrealfast;

using libbase::serializer;
using libbase::vcs;

#define VERSION 1.25

template class watermarkcode<mpreal>;
template <> const serializer watermarkcode<mpreal>::shelper = serializer("modulator", "watermarkcode<mpreal>", watermarkcode<mpreal>::create);
template <> const vcs watermarkcode<mpreal>::version = vcs("Watermark Codec module (watermarkcode<mpreal>)", VERSION);

template class watermarkcode<mpgnu>;
template <> const serializer watermarkcode<mpgnu>::shelper = serializer("modulator", "watermarkcode<mpgnu>", watermarkcode<mpgnu>::create);
template <> const vcs watermarkcode<mpgnu>::version = vcs("Watermark Codec module (watermarkcode<mpgnu>)", VERSION);

template class watermarkcode<logreal>;
template <> const serializer watermarkcode<logreal>::shelper = serializer("modulator", "watermarkcode<logreal>", watermarkcode<logreal>::create);
template <> const vcs watermarkcode<logreal>::version = vcs("Watermark Codec module (watermarkcode<logreal>)", VERSION);

template class watermarkcode<logrealfast>;
template <> const serializer watermarkcode<logrealfast>::shelper = serializer("modulator", "watermarkcode<logrealfast>", watermarkcode<logrealfast>::create);
template <> const vcs watermarkcode<logrealfast>::version = vcs("Watermark Codec module (watermarkcode<logrealfast>)", VERSION);

}; // end namespace
