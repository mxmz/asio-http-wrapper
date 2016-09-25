#include "test.hxx"
#include <iostream>
#include <chrono>
#include <stdlib.h>

#include "boost/lexical_cast.hpp"

template< typename Type >
Type getenv(const char* name, Type dflt = Type() ) {
    const char * val = getenv(name);
    if ( val == nullptr or *val == '\0') {
        return dflt;
    } else {
        return boost::lexical_cast<Type>(val);
    }
}

const bool verbose = getenv<bool>("VERBOSE");
const int  max_time = getenv<int>("MAX_TIME", 1 );

#define cerr if(verbose) cerr 

std::string make_random_string(int c, int from,  int to ) {
    std::string s;
    s.reserve(c);
    for ( int i = 0; i < c; ++i) {
          s.push_back( from  + rand() % (to-from) );
    }
    return s;
}

std::string make_random_string(int c) {
    std::string s;
    s.reserve(c);
    for ( int i = 0; i < c; ++i) {
        s.push_back( 'a' + rand() % 26 );
    }
    return s;
}

void run(const char* name, void(* func)(), int count )
{
    using namespace std::chrono;

    system_clock::time_point endtime = system_clock::now() + seconds(max_time);

    cout << name << " ... " << std::flush ;
    int i = 0;
    for( i = 0; i < count ; ++i) { 
        func();
        if ( system_clock::now() > endtime  ) {
            break;
        }
    } 
    cout << name << " ok " << i  << endl;
}

#define RUN(f,count) run( #f, &f, count )
