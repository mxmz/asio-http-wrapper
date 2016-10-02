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

#include "test.hxx"
using namespace mxmztest;



void random_test(size_t source_base_size) ;

int main()
{
    size_t source_base_size  = 1000 + ( rand() % 1000);
    srand (time(nullptr));
    auto now = time(nullptr);
    while ( (time(nullptr) - now < max_time ) and (source_base_size < 50000000) ) {
        random_test(source_base_size);
        source_base_size += ( 100 * (rand() % (source_base_size/100))) ;
    }
}


void random_test(size_t source_base_size ) {
   

    CERR << "preparing... size=" << source_base_size << endl;
    mxmz::ring_buffer   rb(1024 + rand() % (source_base_size/10));
    auto source = make_random_string(source_base_size  + rand() % 1000);
//    CERR << source << endl;

    CERR << "testing..." << endl;
    CERR << rb.writable() << endl;
    CERR << source.size() <<endl;

    using boost::asio::buffer_cast;

    std::thread t1( [&source,&rb]{
            auto  i = source.begin();
            while ( i != source.end() ) {
                if ( not rb.full() ) {
                    auto buff = *rb.prepare().begin();
//                    CERR << ">" << buffer_size(buff) << endl;
                    size_t chunk = std::min( buffer_size(buff), std::min(  size_t(64 + rand() % 63),  size_t(source.end() - i ) ) );
                    std::copy( i, i + chunk, buffer_cast<char*>(buff));
                    rb.commit(chunk);
                    i += chunk;
                    CERR << "\rt1: written " << chunk << "      ";
                } else {
   //                 CERR << "sleeping ...  source read = " << (i - source.begin() )  << endl;
                    CERR << "\rt1: sleeping                   ";
                    std::this_thread::sleep_for( std::chrono::milliseconds(rand() % 100 ));
                }
            }
            CERR << endl;
            CERR << "t1: end" << endl; 
             
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
                CERR << "\r\t\t\t\tt2: consumed " << size << "         ";
            } else {
 //               CERR << "sleeping ...  copy written = " << copy.size()  << endl;
                 CERR << "\r\t\t\t\tt2: sleeping          ";
                std::this_thread::sleep_for( std::chrono::milliseconds(rand() % 100 ));
            }
        }
        CERR << endl;
        CERR << "t2: end" << endl;

    });

    t1.join();
    t2.join();
    
    CERR << "checking..." << endl;   
    assert( source == copy );
    assert( rb.empty() );

    mxmz::ring_buffer   rb2(1);
    rb2.commit(1);
    assert( not rb2.empty() );

    rb.swap(rb2);

    assert( rb2.empty() );

    rb.consume(1);
    assert(rb.empty() );

    CERR << "ok" << endl;

    
}












