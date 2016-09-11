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
#include "boost/lexical_cast.hpp"


using namespace std;
using namespace boost::asio;

typedef mxmz::http_inbound_connection<ip::tcp::socket> conn_t;
struct body_reader : public enable_shared_from_this<body_reader> {
    shared_ptr<conn_t>  conn;

    char buf[ 1024 ];

    void start_read_body() {
            auto _this = shared_from_this(); 
            conn->async_read_some( buffer(buf, 1024 ), [this,_this]( boost::system::error_code ec, size_t bytes_transferred) {
                    if ( ! ec ) {
                        cerr.write( buf, bytes_transferred);
                        start_read_body();
                    }
            });

    }
} ;


void test1() {
    io_service  ios;
    
    int srv_port = 60000 + (rand()%5000);
        
    ip::tcp::resolver resolver(ios);
    ip::tcp::endpoint endpoint( ip::address::from_string("127.0.0.1"), srv_port);

    ip::tcp::acceptor acceptor(ios);

    ip::tcp::socket socket(ios);


    acceptor.open(endpoint.protocol());
    acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen();

    std::string s;
    

    function<void()> start_accept = [&acceptor,&socket,&start_accept]() {
        acceptor.async_accept( socket, [&acceptor,&socket,&start_accept](boost::system::error_code ec)
        {
            if (!acceptor.is_open())
            {
                return;
            }

            if (!ec)
            {
                cerr << "new connnection" << endl;
               
                auto conn = make_shared<conn_t>( move(socket) ) ;
                conn->async_read_request_header( [conn]( boost::system::error_code ec, conn_t::http_request_header_ptr h  ) {
                    cerr << ec << endl;
                        cerr << h->method << endl;
                        cerr << h->url << endl;
                        auto rb = make_shared<body_reader>();
                        rb->conn = conn;
                        rb->start_read_body();

                } );
            }
            //start_accept();
        } );
    };
    std::promise<bool>   srv_ready;
    auto srv_ready_future = srv_ready.get_future();
        

    thread t( [srv_port, f = move(srv_ready_future)]() {
        io_service ios;
        ip::tcp::socket conn(ios);
        f.wait();

        size_t max_micro_sleep = 5;

        conn.connect( ip::tcp::endpoint( ip::address::from_string("127.0.0.1"), srv_port) );
         const string source = "POST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"
                      "Accept: */*\r\n"
                      "Transfer-Encoding: identity\r\n"
                      "Content-Length: 5\r\n"
                      "\r\n"
                      "World";

        const char* p = source.data();
        const char* end = p + source.size();
        while( p != end ) {
            size_t len = min( long(1 + rand() % 12)  , (end-p) );
            auto rc = conn.write_some( const_buffers_1(p, len) );
            cerr << "t written " << rc << endl;
            p += rc;
            std::this_thread::sleep_for( chrono::microseconds(rand() % max_micro_sleep ));
        }
        std::this_thread::sleep_for( chrono::seconds( 10 ));
    });
   
    start_accept();
    srv_ready.set_value(true);
    ios.run();
    t.join();

} 



int main() {
    srand(time(nullptr));
    test1();
}



#include "detail/http_inbound_connection_impl.hxx"