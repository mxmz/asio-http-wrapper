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
#include "detail/http_inbound_connection_impl.hxx"
#include "boost/lexical_cast.hpp"
#include "ring_buffer.hxx"

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

template< typename Type >
Type getenv(const char* name ) {
    const char * val = getenv(name);
    if ( val == nullptr ) {
        return Type();
    } else {
        return boost::lexical_cast<Type>(val);
    }
}

bool verbose = getenv<bool>("VERBOSE");

#define cerr if(verbose) cerr 


std::string make_random_string(int c, int from,  int to ) {
    std::string s;
    s.reserve(c);
    for ( int i = 0; i < c; ++i) {
          s.push_back( from  + rand() % (to-from) );
    }
    return s;
}


/*
typedef mxmz::http_inbound_connection<ip::tcp::socket> conn_t;
struct body_reader : public enable_shared_from_this<body_reader> {
    shared_ptr<conn_t>  conn;

    char buf[ 1024 ];

    void start_read_body() {
            auto _this = shared_from_this(); 
            conn->async_read_some( buffer(buf, 1024 ), [this,_this]( boost::system::error_code ec, size_t bytes_transferred) {
                cerr  << "read " << bytes_transferred << endl;
                    if ( ! ec ) {
                        cerr.write( buf, bytes_transferred);
                        start_read_body();
                    }
            });

    }
} ;

*/


class parser : public  mxmz::pausing_request_http_parser<parser> {
    public:

    parser::header_ptr header;

    void notify_header( parser::header_ptr h ) {
            header = move(h);
    } 

    string body; 
    string body_ok; 

    size_t handle_body( const char* p , size_t l ) {
        cerr << "handle_body: l " << l << endl;
        if ( l == 0 ) return 0;
        size_t rl = rand() % l + 1 ;
        body.append( p, rl );
        cerr << "handle_body: rl " << rl << endl;
          cerr << "handle_body: body " << body.size() << endl;
        return rl; 
    }

    void notify_body_end() {
            body_ok.swap(body);
    }


    
};


void test1() {

    parser prs;
    
    auto b1 = make_random_string( 1042 + rand() % 1042, 0, 256 );
    

    string bodylen = boost::lexical_cast<string>( b1.size() );

    auto rand_head_name =  make_random_string(1 + rand() % 2042, 'a', 'a'+ 26 );
    auto rand_head_value = make_random_string(1 + rand() % 2042, 'a', 126  );

    const string s1 = "POST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"
                      "Accept: */*\r\n"
                      "Transfer-Encoding: identity\r\n"
                      "Content-Length: " + bodylen + "\r\n"
                      + rand_head_name + ": " + rand_head_value + "\r\n"  
                      "\r\n"
                      + b1;


    size_t eoh = s1.find("\r\n\r\n") + 4;
    
    const char* p = s1.data();
    const char* end = p + s1.size();
    size_t parsed = 0;
    float bufsize = 4;

    while( p != end ) {
          long int len = size_t( bufsize + 1 ); bufsize *= 1 + float(rand()%10 - 2 )/10;
          len = min( len , (end-p) );
          cerr << "paused " << prs.paused() << endl;
          long int consumed = prs.parse( p, len );
          cerr << "len " << len << " consumed " << consumed << endl;
          parsed += consumed;
          p += consumed;
          if ( prs.paused() ) {
                cerr << "paused" << endl;
                    assert( parsed == eoh - 1 );
                    assert( prs.paused() ) ;
                    assert( prs.header );
                    assert( prs.header->method == "POST" );
                    assert( prs.header->url == "/post_identity_body_world?q=search#hey" );
                    assert( (*prs.header)[rand_head_name] == rand_head_value );
                        
                    break;
          }
          
          
    }
    prs.unpause();
     while( p != end ) {
          long int len = size_t( bufsize + 1 ); bufsize *= 1 + float(rand()%10 - 4 )/10;
          len = min( len , (end-p) );
          cerr << "paused " << prs.paused() << endl;
          long int consumed = prs.parse( p, len );
          cerr << "len " << len << " consumed " << consumed << endl;
          prs.unpause();
          if ( prs.body_ok.size() ) break;
          p += consumed;
     }
     cerr << "flushing" << endl;
     while ( prs.flush() ) {} ;
     cerr << b1.size() << endl;
     cerr << prs.body_ok.size() << endl;

     assert( prs.body_ok == b1 );

/*


    size_t consumed  = p.parse( s1.data(), s1.size());

    assert( consumed == eoh + 4 - 1 );
    assert( p.paused() ) ;
    assert( p.header );
    assert( p.header->method == "POST" );
    assert( p.header->url == "/post_identity_body_world?q=search#hey" );
    assert( (*p.header)[rand_head_name] == rand_head_value );

    p.unpause();
*/

}

#if 0
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
                    cerr << ec <<  endl;
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
        string source = "POST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"
                      "Accept: */*\r\n"
                      "Transfer-Encoding: identity\r\n"
                      "Content-Length: 5\r\n"
                      "\r\n"
                      "World";

                      source += source;

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
#endif

void run(const char* name, void(* func)(), int count )
{
    cout << name << " ... " << std::flush ;
    for( int i = 0; i < count ; ++i) func(); 
    cout << name << " ok" << endl;
}

#define RUN(f,count) run( #f, &f, count )

int main() {
    srand(time(nullptr));
    RUN( test1, 5000 );
}



