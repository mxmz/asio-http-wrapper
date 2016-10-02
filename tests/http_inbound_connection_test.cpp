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
using std::shared_ptr;
using std::function;




using namespace mxmz;
using namespace mxmztest;

class parser  {
    typedef mxmz::buffering_request_http_parser<parser> request_parser_t;
    typedef shared_ptr<request_parser_t> request_parser_ptr;
    public:

    request_parser_ptr prs;

    parser() : prs( new request_parser_t(this, 1000) ) {

    }

    std::unique_ptr<const http_request_header> header;

    void notify_header( std::unique_ptr<const http_request_header> h ) {
            header = move(h);
    } 

    string body; 
    bool    finished = false;

    size_t handle_body_chunk( const char* p , size_t l ) {
        cerr << "handle_body: l " << l << endl;
        if ( l == 0 ) return 0;
        size_t rl = rand() % (l+1)  ;
        body.append( p, rl );
        cerr << "handle_body: rl " << rl << endl;
          cerr << "handle_body: body " << body.size() << endl;
        return rl; 
    }

    void notify_body_end() {
            finished = true;
    }

    size_t parse(  const char* p , size_t l ) {
        return prs->parse(p,l);
    }
    size_t paused() const {
        return prs->paused();
    }

    void flush()  {
        prs->flush();
    }
 
    
};


void test1() {

    parser prs;
    
    auto b1 = make_random_string( 1042 + rand() % 10042, 0, 256 );
    

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
    
     while( p != end ) {
          long int len = size_t( bufsize + 1 ); bufsize *= 1 + float(rand()%10 - 4 )/10;
          len = min( len , (end-p) );
          cerr << "paused " << prs.paused() << endl;
          long int consumed = prs.parse( p, len );
          cerr << "len " << len << " consumed " << consumed <<  " paused " << prs.paused() << endl;
    
          p += consumed;
     }
     cerr << "flushing" << endl;
     while ( not prs.finished ) {   prs.flush();  } ;
     cerr << b1.size() << endl;
     cerr << prs.body.size() << endl;

     assert( prs.body == b1 );

}

using mxmz::connection_tmpl;
using namespace std;

/* ---------------------------------------------------------- body_reader ----------------- */







typedef connection_tmpl<mxmz::body_reader> connection;


struct test_reader : std::enable_shared_from_this<test_reader> {
    connection::http_request_header_ptr h;
    connection::body_reader_ptr s;

    std::string body;
    mxmz::ring_buffer rb;
    std::promise<bool> finished;

    test_reader(size_t buffer_size ) : rb(buffer_size) {
        
    }     

    void start() {
        cerr << "test_reader: start " << endl;
        auto self( this->shared_from_this() );
        boost::asio::mutable_buffers_1 buff(rb.prepare() );
        s->async_read_some( buff, [buff,this,self](boost::system::error_code ec, size_t l)  {
                cerr << "test_reader: " << l <<  " " << ec << endl;
                body.append( buffer_cast<const char*>(buff), l );
                cerr.write( buffer_cast<const char*>(buff), l ) << endl;
                if ( ec ) {
                        cerr << "test_reader: " << ec << endl;
                        finished.set_value(true);
                } else {
                       start();
                }
        } );
    }
};

int init() {
    srand(time(nullptr));
    return 0;
}


int _ = init();

int srv_port = 60000 + (rand()%5000);

auto test_server(io_service& ios, string s1) {

    size_t bodybuffer_size = rand() % 2048 + 10;
    size_t readbuffer_size = rand() % 2048 + 10;
    size_t testreader_readbuffer_size = rand() % 2048 + 10;

    
       
    ip::tcp::resolver resolver(ios);
    ip::tcp::endpoint endpoint( ip::address::from_string("127.0.0.1"), srv_port);

    ip::tcp::acceptor acceptor(ios);

    ip::tcp::socket socket(ios);


    acceptor.open(endpoint.protocol());
    acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen();

    std::string s;

    
    typedef body_reader  body_reader_t;

    typedef connection_tmpl< body_reader_t > conn_t;

    
    auto tr = std::make_shared<test_reader>(testreader_readbuffer_size);
    auto srv_finished_future = tr->finished.get_future();

    function<void()> start_accept = [ tr,&acceptor, &socket, &start_accept, bodybuffer_size, readbuffer_size ]() {
        acceptor.async_accept( socket, [ tr, &socket, &start_accept, bodybuffer_size, readbuffer_size](boost::system::error_code ec)
        {
            if (not ec)
            {
                cerr << "new connnection" << endl;
               
                auto conn = make_shared<conn_t>( move(socket), bodybuffer_size, readbuffer_size ) ;
                conn->async_read_request( [tr,conn]( boost::system::error_code ec, 
                                                     connection::http_request_header_ptr h,  
                                                     connection::body_reader_ptr br  ) {
                    cerr << ec <<  endl;
                        cerr << h->method << endl;
                        cerr << h->url << endl;
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
            size_t len = min( long(1024 + rand() % 2048)  , (end-p) );
            boost::system::error_code ec;
            auto rc = conn.write_some( const_buffers_1(p, len), ec );
            p += rc;
            cerr << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> t written " << rc << "  " << ec << " remain " << (end-p) <<  endl;
            std::this_thread::sleep_for( chrono::microseconds(( rand() % 100 ) / 96 ));

        }
        //std::this_thread::sleep_for( chrono::seconds( 1 ));
        f2.wait_for(chrono::seconds(10));
    });
   


    start_accept();
    srv_ready.set_value(true);
    ios.run();
    t.join();

    cerr << bodybuffer_size << endl;
    cerr << readbuffer_size << endl;

        return tr;
} 

void test2() {
    auto b1 = make_random_string( 1042 + rand() % 10042, 'a', 'z' +1  );
    string bodylen = boost::lexical_cast<string>( b1.size() );
    auto rand_head_name1 =  make_random_string(1 + rand() % 1042, 'a', 'a'+ 1 );
    auto rand_head_value1 = make_random_string(1 + rand() % 1042, 'a', 126  );
    auto rand_head_name2 =  make_random_string(1 + rand() % 1042, 'a', 'a'+ 26 );
    auto rand_head_value2 = make_random_string(1 + rand() % 1042, 'a', 126  );
    auto garbage = make_random_string(1 + rand() % 1042, 'A', 'Z'+ 1 );
    
    const string s1 = "POST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"
                      "Accept: */*\r\n"
                      + rand_head_name1 + ": " + rand_head_value1 + "\r\n"
                      "Transfer-Encoding: identity\r\n"
                      "Content-Length: " + bodylen + "\r\n"
                      + rand_head_name2 + ": " + rand_head_value2 + "\r\n"  
                      "\r\n"
                      + b1 + garbage;
    io_service ios;
    auto tr = test_server(ios,s1);
    cerr << tr->body.size() << endl;
    cerr << b1.size() << endl;
    assert( tr->body ==  b1 );
    assert( tr->h );
    assert( tr->h->method == "POST" );
    assert( tr->h->url == "/post_identity_body_world?q=search#hey" );
    assert( (*tr->h)[rand_head_name1] == rand_head_value1 );
    assert( (*tr->h)[rand_head_name2] == rand_head_value2 );
    
    auto buffered = tr->s->connection()->buffered_data();
    std::string buffered_str( buffer_cast<const char*>(buffered), buffer_size(buffered) );

    cerr << buffered_str << endl;
    cerr << garbage << endl;

    cerr << garbage.find(buffered_str) << endl;
    assert( garbage.find(buffered_str) == 0 ); // extra garbage must appear at the beginning of buffered data, if any 
    

}

void test3() {
    auto b1 = make_random_string( 1042 + rand() % 10042, 'a', 'z' +1  );
    auto rand_head_name1 =  make_random_string(1 + rand() % 1042, 'a', 'a'+ 1 );
    auto rand_head_value1 = make_random_string(1 + rand() % 1042, 'a', 126  );
    auto rand_head_name2 =  make_random_string(1 + rand() % 1042, 'a', 'a'+ 26 );
    auto rand_head_value2 = make_random_string(1 + rand() % 1042, 'a', 126  );
    
    const string s1 = "POST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"
                      "Accept: */*\r\n"
                      + rand_head_name1 + ": " + rand_head_value1 + "\r\n"
                      "Transfer-Encoding: identity\r\n"
                      + rand_head_name2 + ": " + rand_head_value2 + "\r\n"  
                      "\r\n"
                      + b1;
    io_service ios;
    auto tr = test_server(ios,s1);
    cerr << tr->body.size() << endl;
    cerr << b1.size() << endl;
    assert( tr->body ==  b1 );
    assert( tr->h );
    assert( tr->h->method == "POST" );
    assert( tr->h->url == "/post_identity_body_world?q=search#hey" );
    assert( (*tr->h)[rand_head_name1] == rand_head_value1 );
    assert( (*tr->h)[rand_head_name2] == rand_head_value2 );
    
    auto buffered = tr->s->connection()->buffered_data();
    std::string buffered_str( buffer_cast<const char*>(buffered), buffer_size(buffered) );


}




int main() {

    RUN( test1, verbose ? 10: 1000 );
    RUN( test2, verbose ? 10: 3000 );
}



#include "detail/http_inbound_connection_impl.hxx"