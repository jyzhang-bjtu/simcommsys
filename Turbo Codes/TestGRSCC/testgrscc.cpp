/*!
   \file

   \par Version Control:
   - $Revision: 161 $
   - $Date: 2007-12-12 14:24:47 +0000 (Wed, 12 Dec 2007) $
   - $Author: jabriffa $
*/

#include "grscc.h"
#include "gf.h"
#include <iostream>

int main(int argc, char *argv[])
   {
   using std::cout;
   using std::cerr;

   using libbase::vector;
   using libbase::matrix;

   // Define type for RSC code in GF(2^4): m(x) = 1 { 0011 }
   typedef libbase::gf<4,0x13> G;
   typedef libcomm::grscc<G> RSC;

   // Create generator matrix for a R=1/3 code
   matrix< vector<G> > gen(1,3);
   // 1 + D + a^4 D^2
   gen(0,0).init(3);
   gen(0,0)(2) = "0001";
   gen(0,0)(1) = "0001";
   gen(0,0)(0) = "0011";
   // 1 + a D + a^4 D^2
   gen(0,1).init(3);
   gen(0,1)(2) = "0001";
   gen(0,1)(1) = "0010";
   gen(0,1)(0) = "0011";
   // 1 + a^2 D + a^9 D^2
   gen(0,2).init(3);
   gen(0,2)(2) = "0001";
   gen(0,2)(1) = "0100";
   gen(0,2)(0) = "1010";
   // Display generator matrix
   cout << "Generator matrix:\n";
   cout << gen;

   // Compute and display exponential table using {03} as a multiplier
   // using the tabular format in Gladman.
   //for(int x=0; x<16; x++)
   //   for(int y=0; y<16; y++)
   //      {
   //      assert(E == table[(x<<4)+y]);
   //      cout << std::hex << int(E) << (y==15 ? '\n' : '\t');
   //      E *= 3;
   //      }

   return 0;
   }
