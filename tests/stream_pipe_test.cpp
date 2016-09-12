#define BOOST_ASIO_DISABLE_EPOLL
#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <string>
#include <thread>
#include <cassert>
#include <vector>
#include <future>

using namespace std;

#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include "boost/lexical_cast.hpp"

#include <fstream>
#include <stdio.h>


#include "stream_pipe.hxx"


using namespace boost;



template< typename Type >
Type getenv(const char* name, Type dflt = Type() ) {
    const char * val = getenv(name);
    if ( val == nullptr ) {
        return Type(dflt);
    } else {
        return boost::lexical_cast<Type>(val);
    }
}

bool verbose = getenv<bool>("VERBOSE", false);

#define cerr if(verbose) cerr 


typedef mxmz::stream_pipe_tmpl< asio::ip::tcp::socket> tcp_socket_pipe;


std::string make_random_string(int c) {
    std::string s;
    s.reserve(c);
    for ( int i = 0; i < c; ++i) {
        s.push_back( 'a' + rand() % 26 );
    }
    return s;
}

using namespace std;

void test1(int max_micro_sleep )
{
        cout << __FUNCTION__ << " ..." << endl;
        asio::io_service ios;
        asio::io_service ios2;
        asio::ip::tcp::socket src(ios);
        asio::ip::tcp::socket dst(ios);

        auto source  = make_random_string(1024*1024*5);
        int src_port = 60000 + (rand()%5000);
        int dst_port = 60000 + (rand()%5000);
        string copy;

        std::promise<bool>   src_ready;
        std::promise<bool>   dst_ready;
        auto src_ready_future = src_ready.get_future();
        auto dst_ready_future = dst_ready.get_future();

         std::thread t1( [&source,src_port,max_micro_sleep,&src_ready]{
             using namespace boost::asio;
             using namespace boost::asio::ip;
              using boost::asio::const_buffers_1;
             io_service ios;
             tcp::socket sock(ios);
             tcp::endpoint endpoint(tcp::v4(), src_port);
             tcp::acceptor acceptor( ios, endpoint );
             boost::system::error_code ec;
             src_ready.set_value(true);  
             acceptor.accept(sock,ec);
             if ( ! ec ) {
                const char* p = source.data();
                const char* end = p + source.size();
                while( p != end ) {
                    size_t len = min( long(1 + rand() % 127)  , (end-p) );
                    auto rc = sock.write_some( const_buffers_1(p, len) );
                    cerr << "t1 written " << rc << endl;
                    p += rc;
                    std::this_thread::sleep_for( chrono::microseconds(rand() % max_micro_sleep ));
                }
             } 
             cerr << "t1 finished" << endl;      
         });
         std::thread t2( [&copy,dst_port,max_micro_sleep,&dst_ready]{
             using namespace boost::asio;
             using namespace boost::asio::ip;
             using boost::asio::mutable_buffers_1;
             io_service ios;
             tcp::socket sock(ios);
             tcp::endpoint endpoint(tcp::v4(), dst_port);
             tcp::acceptor acceptor( ios, endpoint );
             boost::system::error_code ec;  
             dst_ready.set_value(true);
             acceptor.accept(sock,ec);
             if ( ! ec ) {
                 char buffer[ 128 ];
                 while( true ) {
                     size_t len = 1 + rand() % 127 ;
                     auto rv = sock.read_some( mutable_buffers_1( buffer, len  ) , ec );
                     cerr << "t2 read " << rv << endl;
                     if ( ec ) break ;
                     copy.append( buffer, rv );
                     std::this_thread::sleep_for( chrono::microseconds(rand() % max_micro_sleep ));
                 }
             }       
             cerr << "t2 finished" << endl;
         });
   
         src_ready_future.wait();
         dst_ready_future.wait();

        src.connect( asio::ip::tcp::endpoint( asio::ip::address::from_string("127.0.0.1"), src_port) );
        dst.connect( asio::ip::tcp::endpoint( asio::ip::address::from_string("127.0.0.1"), dst_port) );

        std::promise<bool>   pipe_finished;
        auto pipe_finished_future = pipe_finished.get_future();

        auto pipe = std::make_shared<tcp_socket_pipe>(src, dst, 4567);

        pipe->run( [pipe,&dst,&pipe_finished](const system::error_code& read_ec, const system::error_code& write_ec) {
                    cerr << read_ec.message() << endl;
                    cerr << write_ec.message() << endl;
                    dst.close();
                    cout << std::this_thread::get_id() << endl;
                    pipe_finished.set_value(true);
                  }  );

        std::vector<std::thread> iosrunners;

        for ( int i = 0; i != 10; ++i ) {
            thread t( [&ios,i ]() {
                        std::this_thread::sleep_for( chrono::milliseconds(rand() % 100 ));
                        cout << i << ": " << std::this_thread::get_id() << endl; 
                        ios.run();
            });
            iosrunners.emplace_back( move(t));
        }

        pipe_finished_future.wait();

        t1.join();
        t2.join();

        for( thread& t : iosrunners) {
            t.join();
        }

        assert( source == copy );
        cout << __FUNCTION__ << " ok" << endl;
}


int main(){
    time_t max_time = getenv<time_t>("MAX_TIME", 10);
    
    auto start = time(nullptr);
    srand(start);
    while ( time(nullptr) - start < max_time ) {  
        test1( rand() % 5  + 1 );
    }
    return 0;
}

