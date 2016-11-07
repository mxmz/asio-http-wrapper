#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <string>
#include <map>
#include <functional>
#include <memory>
#include <cassert>
#include <thread>
#include <future>
#include "http_inbound_connection.hxx"
#include "wrapper/http_parser.hxx"
#include "../tests/test.h"

using namespace boost::asio;
using boost::asio::mutable_buffers_1;
using boost::asio::buffer_cast;
using boost::asio::mutable_buffer;
using boost::asio::const_buffers_1;
using boost::asio::const_buffer;
using boost::system::error_code;

using std::string;
using std::cerr;
using std::cout;
using std::endl;
using std::min;
using std::shared_ptr;
using std::function;
using std::make_shared;
using std::move;


    const size_t bodybuffer_size = 1024;
    const size_t readbuffer_size = 1024;

    const int srv_port = 60001; 

int main() {

    io_service ios;
    
    ip::tcp::resolver resolver(ios);
    ip::tcp::endpoint endpoint( ip::address::from_string("127.0.0.1"), srv_port);

    ip::tcp::acceptor acceptor(ios);

    ip::tcp::socket socket(ios);


    acceptor.open(endpoint.protocol());
    acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen();

    typedef mxmz::connection_tmpl< mxmz::nodejs::http_parser_base > conn_t;

   
    function<void()> start_accept = [ &acceptor, &socket, &start_accept ]() {
        acceptor.async_accept( socket, [ &socket, &start_accept ](boost::system::error_code ec)
        {
            if (not ec)
            {
                CERR << "new connnection" << endl;
               
                auto conn = make_shared<conn_t>( move(socket), bodybuffer_size, readbuffer_size ) ;
                conn->async_wait_request( [conn]( boost::system::error_code ec, 
                                                     conn_t::http_request_header_ptr h  
                                                     ) {
                        CERR << ec <<  endl;
                        CERR << h->method << endl;
                        CERR << h->url << endl;
                        auto br  = conn->make_body_reader();

                } );
            }
            start_accept();
        } );
    }; 
    
    start_accept();
    ios.run();
}

#include "wrapper/detail/http_parser_impl.hxx"
#include "detail/http_inbound_connection_impl.hxx"
#include "util/detail/pimpl.hxx"