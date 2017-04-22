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

#include "test.hxx"
#include "util/stream_pipe.hxx"


using namespace boost;


using namespace mxmztest;


typedef mxmz::stream_pipe_tmpl< asio::ip::tcp::socket> tcp_socket_pipe;


using namespace std;

const int src_port = 60000 + (rand_int()%5000);
const int dst_port = 60000 + (rand_int()%5000);


void test1()
{
    

        asio::io_service ios;
        asio::io_service ios2;
        auto src = std::make_shared<asio::ip::tcp::socket>(ios);
        asio::ip::tcp::socket dst(ios);

        auto source  = make_random_string(1024*32 + ( 256 * rand_int() % 2 ) + (rand_int() % 256 )  );
        string copy;

        std::promise<bool>   src_ready;
        std::promise<bool>   dst_ready;
        auto src_ready_future = src_ready.get_future();
        auto dst_ready_future = dst_ready.get_future();

         std::thread t1( [&source,&src_ready]{
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
                    size_t len = min( long(1 + rand_int() % 512)  , (end-p) );
                    auto rc = sock.write_some( const_buffers_1(p, len) );
                    CERR << "t1 written " << rc << endl;
                    p += rc;
                  //  std::this_thread::sleep_for( chrono::microseconds(( rand_int() % 10 ) / 9 ));
                }
             } 
             CERR << "t1 finished" << endl;      
         });
         std::thread t2( [&copy,&dst_ready]{
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
                 char buffer[ 1024 ];
                 while( true ) {
                     size_t len = 1 + rand_int() % 1000 ;
                     auto rv = sock.read_some( mutable_buffers_1( buffer, len  ) , ec );
                     CERR << "t2 read " << rv << endl;
                     if ( ec ) break ;
                     copy.append( buffer, rv );
                     //std::this_thread::sleep_for( chrono::microseconds(( rand_int() % 10 ) / 9 ));
                 }
             }       
             CERR << "t2 finished" << endl;
         });
   
         src_ready_future.wait();
         dst_ready_future.wait();

        src->connect( asio::ip::tcp::endpoint( asio::ip::address::from_string("127.0.0.1"), src_port) );
        dst.connect( asio::ip::tcp::endpoint( asio::ip::address::from_string("127.0.0.1"), dst_port) );

        std::promise<bool>   pipe_finished;
        auto pipe_finished_future = pipe_finished.get_future();

        auto pipe =  mxmz::make_stream_pipe(*src, dst, 4567);

        pipe->run( [pipe,src,&dst,&pipe_finished](const system::error_code& read_ec, const system::error_code& write_ec) {
                    CERR << read_ec.message() << endl;
                    CERR << write_ec.message() << endl;
                    dst.close();
                    CERR << std::this_thread::get_id() << endl;
                    pipe_finished.set_value(true);
                  }  );
                  
        std::vector<std::thread> iosrunners;
        int thcount = 1  + rand_int() % 4;
        for ( int i = 0; i != thcount; ++i ) {
            thread t( [&ios,i ]() {
                        std::this_thread::sleep_for( chrono::microseconds(rand_int() % 3 ));
//                        cout << i << ": " << std::this_thread::get_id() << endl; 
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
        CERR << source.size() << endl;
        assert( source == copy );
        //cout << "#" << std::flush;        
}


int main() {
    srand(time(nullptr));
    RUN( test1, verbose ? 1: 100 );
}
