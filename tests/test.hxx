#include <iostream>
#include <chrono>
#include <stdlib.h>

#include "boost/lexical_cast.hpp"

namespace mxmztest {

using std::cerr;
using std::cout;
using std::endl;


extern const bool verbose;
extern const int  max_time;

#define cerr if(mxmztest::verbose) cerr 

std::string make_random_string(int c, int from,  int to );
    

std::string make_random_string(int c);
    

void run(const char* name, void(* func)(), int count );

}


#define RUN(f,count) mxmztest::run( #f, &f, count )
