#ifndef htest_hxx_30948503985039458309458309458039485039485039583234987
#define htest_hxx_30948503985039458309458309458039485039485039583234987

#include <iostream>
#include <chrono>
#include <stdlib.h>
#include "boost/lexical_cast.hpp"

#include "test.h"

namespace mxmztest {

using std::cerr;
using std::cout;
using std::endl;


int rand_int() ;
std::string make_random_string(int c, int from,  int to );

std::string make_random_string(int c);

void run(const char* name, void(* func)(), int count );

}


#define RUN(f,count) mxmztest::run( #f, &f, count )

#endif // !htest_hxx_30948503985039458309458309458039485039485039583234987
