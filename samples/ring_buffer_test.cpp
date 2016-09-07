#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <algorithm>
#include <string>
#include <random>
#include <thread>
#include <cassert>

using namespace std;

#include <fstream>



#include "ring_buffer.hxx"


std::string make_random_string(int c) {
    std::string s;
    for ( int i = 0; i < c; ++i) {
        s.push_back( 'a' + rand() % 26 );
    }
    return s;
}

using std::cerr;
using std::endl;

void random_test(size_t source_base_size) ;

int main()
{
    size_t source_base_size  = 1000;
    srand (time(nullptr));
    auto now = time(nullptr);
    while ( time(nullptr) - now < 4) {
        random_test(source_base_size );
        source_base_size *= ( 1 + rand() % 10 );
    }
}

void random_test(size_t source_base_size ) {
   

    mxmz::ring_buffer   rb(1024 + rand() % 1000);
    auto source = make_random_string(source_base_size  + rand() % 1000);
//    cerr << source << endl;

    cerr << rb.writable() << endl;
    cerr << source.size() <<endl;

    using boost::asio::buffer_cast;

    std::thread t1( [&source,&rb]{
            
            auto  i = source.begin();
            while ( i != source.end() ) {
                if ( not rb.full() ) {
                    auto buff = *rb.prepare().begin();
//                    cerr << ">" << buffer_size(buff) << endl;
                    size_t chunk = std::min( buffer_size(buff), std::min(  size_t(64 + rand() % 63),  size_t(source.end() - i ) ) );
                    std::copy( i, i + chunk, buffer_cast<char*>(buff));
                    rb.commit(chunk);
                    i += chunk;
                } else {
                    cerr << "sleeping ...  source read = " << (i - source.begin() )  << endl;
                    std::this_thread::sleep_for( std::chrono::milliseconds(rand() % 10 ));
                }
            } 
             
    });

    std::string copy;
    std::thread t2( [&rb, &copy, expected = source.size() ]{
        while ( copy.size() < expected ) {
            if ( not rb.empty() ) {
                auto buff = *rb.data().begin();
                auto data = buffer_cast<const char*>(buff);
                auto size = buffer_size(buff);
                copy.append(data,size);
                rb.consume(size);
            } else {
                cerr << "sleeping ...  copy written = " << copy.size()  << endl;
                std::this_thread::sleep_for( std::chrono::milliseconds(rand() % 10 ));
            }

        }

    });

    t1.join();
    t2.join();
    
    assert( source == copy );
    
}












