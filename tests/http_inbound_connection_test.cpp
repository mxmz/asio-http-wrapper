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

#include "test.hxx"

#include "http_inbound_connection.hxx"
#include "util/ring_buffer.hxx"
#include "socketmock.hxx"

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




using namespace mxmz;
using namespace mxmztest;

namespace mxmz {
namespace nodejs {
    template <class Handlers>
        class http_parser_base;
}
}

class parser  {
    typedef mxmz::buffering_request_http_parser<parser,nodejs::http_parser_base> request_parser_t;
    typedef shared_ptr<request_parser_t> request_parser_ptr;
    public:

    request_parser_t prs;

    parser() : prs( this, 1000 ) {

    }

    std::unique_ptr<const http_request_header> header;

    void handle_header( std::unique_ptr<const http_request_header> h ) {
            header = move(h);
    } 

    string body; 
    bool    finished = false;

    size_t handle_body_chunk( const char* p , size_t l ) {
        CERR << "handle_body: l " << l << endl;
        if ( l == 0 ) return 0;
        size_t rl = rand_int() % (l+1)  ;
        body.append( p, rl );
        CERR << "handle_body: rl " << rl << endl;
          CERR << "handle_body: body " << body.size() << endl;
        return rl; 
    }

    void handle_body_end() {
            finished = true;
    }

    size_t parse(  const char* p , size_t l, bool eof ) {
        return prs.parse(p,l,eof);
    }
    size_t paused() const {
        return prs.paused();
    }

    void flush()  {
        prs.flush();
    }
 
    
};


void test1() {

    parser prs;
    
    auto b1 = make_random_string( 1042 + rand_int() % 10042, 0, 256 );
    

    string bodylen = boost::lexical_cast<string>( b1.size() );

    auto rand_head_name =  make_random_string(1 + rand_int() % 2042, 'a', 'a'+ 26 );
    auto rand_head_value = make_random_string(1 + rand_int() % 2042, 'a', 126  );

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
          long int len = size_t( bufsize + 1 ); bufsize *= 1 + float(rand_int()%10 - 2 )/10;
          len = min( len , (end-p) );
          CERR << "paused " << prs.paused() << endl;
          long int consumed = prs.parse( p, len, false );
          CERR << "len " << len << " consumed " << consumed << endl;
          parsed += consumed;
          p += consumed;
          if ( prs.paused() ) {
                CERR << "paused" << endl;
                    assert( parsed == eoh - 1 );
                    assert( prs.paused() ) ;
                    assert( prs.header );
                    assert( prs.header->method == "POST" );
                    assert( prs.header->url == "/post_identity_body_world?q=search#hey" );
                    assert( (*prs.header)[rand_head_name] == rand_head_value );
                        
                    break;
          }
          
          
    }
    
     while( p != end ) {
          long int len = size_t( bufsize + 1 ); bufsize *= 1 + float(rand_int()%10 - 4 )/10;
          len = min( len , (end-p) );
          CERR << "paused " << prs.paused() << endl;
          long int consumed = prs.parse( p, len, false );
          CERR << "len " << len << " consumed " << consumed <<  " paused " << prs.paused() << endl;
    
          p += consumed;
     }
     CERR << "flushing" << endl;
     while ( not prs.finished ) {   prs.flush();  } ;
     CERR << b1.size() << endl;
     CERR << prs.body.size() << endl;

     assert( prs.body == b1 );

}

using mxmz::inbound_connection_tmpl;
using namespace std;

/* ---------------------------------------------------------- body_reader ----------------- */

template< class Connection >
struct test_reader_tmpl : std::enable_shared_from_this<test_reader_tmpl<Connection>> {
    typedef Connection connection;
    typename connection::http_request_header_ptr h;
    typename connection::body_reader_ptr s;

    std::string body;
    mxmz::ring_buffer rb;
    std::promise<bool> finished;

    test_reader_tmpl(size_t buffer_size ) : rb(buffer_size) {
        
    }     

    void start() {
        CERR << "test_reader: start " << endl;
        auto self( this->shared_from_this() );
        boost::asio::mutable_buffers_1 buff(rb.prepare() );
        s->async_read_some( buff, [buff,this,self](boost::system::error_code ec, size_t l)  {
                CERR << "test_reader: " << l <<  " " << ec << endl;
                body.append( buffer_cast<const char*>(buff), l );
                CERR.write( buffer_cast<const char*>(buff), l ) << endl;
                if ( ec ) {
                        CERR << "test_reader: " << ec << endl;
                        finished.set_value(true);
                } else {
                       start();
                }
        } );
    }
};



int srv_port = 60000 + (rand_int()%5000);

auto test_server(io_service& ios, string s1) {

    typedef inbound_connection_tmpl<nodejs::http_parser_base > connection;
    typedef test_reader_tmpl<connection> test_reader;

    size_t bodybuffer_size = rand_int() % 2048 + 10;
    size_t readbuffer_size = rand_int() % 2048 + 10;
    size_t testreader_readbuffer_size = rand_int() % 2048 + 10;

    
       
    ip::tcp::resolver resolver(ios);
    ip::tcp::endpoint endpoint( ip::address::from_string("127.0.0.1"), srv_port);

    ip::tcp::acceptor acceptor(ios);

    ip::tcp::socket socket(ios);


    acceptor.open(endpoint.protocol());
    acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen();

    std::string s;

    
    typedef inbound_connection_tmpl< nodejs::http_parser_base > conn_t;

    
    auto tr = std::make_shared<test_reader>(testreader_readbuffer_size);
    auto srv_finished_future = tr->finished.get_future();

    function<void()> start_accept = [ tr,&acceptor, &socket, &start_accept, bodybuffer_size, readbuffer_size ]() {
        acceptor.async_accept( socket, [ tr, &socket, &start_accept, bodybuffer_size, readbuffer_size](boost::system::error_code ec)
        {
            if (not ec)
            {
                CERR << "new connnection" << endl;
               
                auto conn = make_shared<conn_t>( move(socket), bodybuffer_size, readbuffer_size ) ;
                conn->async_wait_request( [tr,conn]( boost::system::error_code ec, 
                                                     connection::http_request_header_ptr h  
                                                     ) {
                    CERR << ec <<  endl;
                        CERR << h->method << endl;
                        CERR << h->url << endl;
                        auto br  = conn->make_body_reader();
                        tr->s = br;
                        tr->h = move(h);
                        tr->start();
                } );
            }
            //start_accept();
        } );
    };
    std::promise<bool>   srv_ready;
    auto srv_ready_future = srv_ready.get_future();

        

    thread t( [s1,f = move(srv_ready_future), f2 = move(srv_finished_future)]() {
        io_service ios;
        ip::tcp::socket conn(ios);
        f.wait();

        conn.connect( ip::tcp::endpoint( ip::address::from_string("127.0.0.1"), srv_port) );
        string source = s1; 
        

        const char* p = source.data();
        const char* end = p + source.size();
        while( p != end ) {
            size_t len = min( long(1024 + rand_int() % 2048)  , (end-p) );
            boost::system::error_code ec;
            auto rc = conn.write_some( const_buffers_1(p, len), ec );
            p += rc;
            CERR << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> t written " << rc << "  " << ec << " remain " << (end-p) <<  endl;
            std::this_thread::sleep_for( chrono::microseconds(( rand_int() % 100 ) / 98 ));

        }
        //std::this_thread::sleep_for( chrono::seconds( 1 ));
        CERR << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> t wait future" << endl;
        f2.wait_for(chrono::seconds(10));
        CERR << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> t finished" << endl;
    });
   


    start_accept();
    srv_ready.set_value(true);
    ios.run();
    t.join();

    CERR << bodybuffer_size << endl;
    CERR << readbuffer_size << endl;

        return tr;
}


auto test_socketmock(io_service& ios, string s1) {

    
    


    size_t bodybuffer_size = rand_int() % 2048 + 10;
    size_t readbuffer_size = rand_int() % 2048 + 10;
    size_t testreader_readbuffer_size = rand_int() % 2048 + 10;

    
           
    mock_asio_socket    socket(  ios, move(s1) );
    
    

    typedef inbound_connection_tmpl<nodejs::http_parser_base,mock_asio_socket> conn_t;
    typedef test_reader_tmpl<conn_t>  test_reader;

    auto tr = std::make_shared<test_reader>(testreader_readbuffer_size);
    auto srv_finished_future = tr->finished.get_future();

    auto conn = make_shared<conn_t>( move(socket), bodybuffer_size, readbuffer_size ) ;
    conn->async_wait_request( [tr,conn]( boost::system::error_code ec, 
                                                     conn_t::http_request_header_ptr h  
                                                     ) {
                    CERR << ec <<  endl;
                        CERR << h->method << endl;
                        CERR << h->url << endl;
                        auto br  = conn->make_body_reader();
                        tr->s = br;
                        tr->h = move(h);
                        tr->start();
                } );


    ios.run();                
    return tr;

}

template< class TestFunc >
void test_no_chunked(TestFunc test_func) {
    auto b1 = make_random_string( 1042 + rand_int() % 10042, 'a', 'z' +1  );
    string bodylen = boost::lexical_cast<string>( b1.size() );
    auto rand_head_name1 =  make_random_string(1 + rand_int() % 1042, 'a', 'a'+ 26 );
    auto rand_head_value1 = make_random_string(1 + rand_int() % 1042, 'a', 126  );
    auto rand_head_name2 =  make_random_string(1 + rand_int() % 1042, 'a', 'a'+ 26 );
    auto rand_head_value2 = make_random_string(1 + rand_int() % 1042, 'a', 126  );
    auto garbage = make_random_string(1 + rand_int() % 1042, 'A', 'Z'+ 1 );
    
    const string s1 = "POST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"
                      "Accept: */*\r\n"
                      + rand_head_name1 + ": " + rand_head_value1 + "\r\n"
                      "Transfer-Encoding: identity\r\n"
                      "Content-Length: " + bodylen + "\r\n"
                      + rand_head_name2 + ": " + rand_head_value2 + "\r\n"  
                      "\r\n"
                      + b1 + garbage;
    io_service ios;
    auto tr = test_func(ios,s1);
    CERR << tr->body.size() << endl;
    CERR << b1.size() << endl;
    assert( tr->body ==  b1 );
    assert( tr->h );
    assert( tr->h->method == "POST" );
    assert( tr->h->url == "/post_identity_body_world?q=search#hey" );
    assert( (*tr->h)[rand_head_name1] == rand_head_value1 );
    assert( (*tr->h)[rand_head_name2] == rand_head_value2 );
    
    auto buffered = tr->s->connection()->buffered_data();
    std::string buffered_str( buffer_cast<const char*>(buffered), buffer_size(buffered) );

    CERR << buffered_str << endl;
    CERR << garbage << endl;

    CERR << garbage.find(buffered_str) << endl;
    assert( garbage.find(buffered_str) == 0 ); // extra garbage must appear at the beginning of buffered data, if any 
    

}

string int2hex(int i) {
    char buffer[100];
    snprintf(buffer, 100, "%x", i );
    return string(buffer);
}

string string2chunks( const string& s ) {
    string rv;
    const char* p = s.data();
    const char* end = p + s.size();
    while( p != end ) {
        size_t len = min( long(1 + rand_int() % 512)  , (end-p) );
        rv.append(int2hex(len));
        rv.append("\r\n");
        rv.append( p, len);
        rv.append("\r\n");
        p += len;
    }
    rv.append("0\r\n\r\n");
    return rv;
}


template< class TestFunc >
void test_chunked(TestFunc test_func) {
    auto s = make_random_string( 1042 + rand_int() % 10042, verbose ? 'a': 0, 'z' +1  );
    auto rand_head_name1 =  make_random_string(1 + rand_int() % 1042, 'a', 'a'+ 26 );
    auto rand_head_value1 = make_random_string(1 + rand_int() % 1042, 'a', 126  );
    auto rand_head_name2 =  make_random_string(1 + rand_int() % 1042, 'a', 'a'+ 26 );
    auto rand_head_value2 = make_random_string(1 + rand_int() % 1042, 'a', 126  );
    auto garbage = make_random_string(1 + rand_int() % 1042, 'A', 'Z'+ 1 );

    auto b1 = string2chunks(s);
    CERR << "++++++++++++++++++++ " << b1 << endl;
    
    const string s1 = "POST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"
                      "Accept: */*\r\n"
                      + rand_head_name1 + ": " + rand_head_value1 + "\r\n"
                      "Transfer-Encoding: chunked\r\n"
                      + rand_head_name2 + ": " + rand_head_value2 + "\r\n"  
                      "\r\n"
                      + b1
                      + garbage;
    io_service ios;
    CERR << b1.size() << endl;
    auto tr = test_func(ios,s1);
    CERR << tr->body.size() << endl;
    
    
    assert( tr->body ==  s );
    assert( tr->h );
    assert( tr->h->method == "POST" );
    assert( tr->h->url == "/post_identity_body_world?q=search#hey" );
    assert( (*tr->h)[rand_head_name1] == rand_head_value1 );
    assert( (*tr->h)[rand_head_name2] == rand_head_value2 );
    
    auto buffered = tr->s->connection()->buffered_data();
    std::string buffered_str( buffer_cast<const char*>(buffered), buffer_size(buffered) );

    CERR << buffered_str << endl;
        assert( garbage.find(buffered_str) == 0 ); // extra garbage must appear at the beginning of buffered data, if any
}

void test2() {
    test_no_chunked(&test_server);
}
void test3() {
    test_chunked(&test_server);
}

void test2mok() {
    test_no_chunked(&test_socketmock);
}
void test3mok() {
    test_chunked(&test_socketmock);
}


int main() {

    RUN( test1, verbose ? 10: 1000 );
    RUN( test2, verbose ? 10: 1000 );
    RUN( test3, verbose ? 10: 1000 );
    RUN( test3mok, verbose ? 10: 1000 );  
}


#include "wrapper/detail/http_parser_impl.hxx"
#include "detail/http_inbound_connection_impl.hxx"
#include "util/detail/pimpl.hxx"