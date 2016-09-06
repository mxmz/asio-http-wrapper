#define BOOST_ASIO_DISABLE_EPOLL
#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <string>

using namespace std;

#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

#include <fstream>
#include <stdio.h>

using namespace boost;

#include "stream_pipe.hxx"

typedef mxmz::stream_pipe_tmpl< asio::ip::tcp::socket,asio::posix::stream_descriptor> tcp_socket_pipe;

int main()
{
        asio::io_service ios;
        asio::io_service ios2;
        asio::ip::tcp::socket src(ios);
//        asio::ip::tcp::socket dst(ios);



        
        
        asio::posix::stream_descriptor dst(ios, 1);

        


        src.connect( asio::ip::tcp::endpoint( asio::ip::address::from_string("127.0.0.1"), 9001) );
        //dst.connect( asio::ip::tcp::endpoint( asio::ip::address::from_string("192.168.69.23"), 9002) );
//        dst.connect( asio::ip::tcp::endpoint( asio::ip::address::from_string("127.0.0.1"), 9002) );


        auto pipe = std::make_shared<tcp_socket_pipe>(src, dst, 102);

        pipe->run( [pipe](const system::error_code& read_ec, const system::error_code& write_ec) {
                    cerr << read_ec.message() << endl;
                    cerr << write_ec.message() << endl;

                  }  );
        ios.run();


}

