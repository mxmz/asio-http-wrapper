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

using namespace mxmz;

class parser  {
    typedef mxmz::buffering_request_http_parser<parser> request_parser_t;
    typedef shared_ptr<request_parser_t> request_parser_ptr;
    public:

    request_parser_ptr prs;

    parser() : prs( new request_parser_t(this, 1000) ) {

    }

    std::unique_ptr<http_request_header> header;

    void notify_header( std::unique_ptr<http_request_header> h ) {
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

    size_t flush()  {
        return prs->flush();
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


template< class RequestObserver, class BodyObserver >
class connection: 
    public   std::enable_shared_from_this< connection<RequestObserver,BodyObserver> >, 
    public  mxmz::buffering_request_http_parser< connection<RequestObserver,BodyObserver> > {

    typedef mxmz::buffering_request_http_parser<connection<RequestObserver,BodyObserver>> base_t;

    ip::tcp::socket     socket;
    mxmz::ring_buffer   rb;
    RequestObserver*    request_notifier;
    BodyObserver*        body_handler;

    public:

    connection ( ip::tcp::socket&& s, size_t buffer_threshold, size_t buffer_size) :
                base_t(buffer_threshold),
                socket( move(s)), 
                rb( buffer_size ),
                request_notifier(nullptr),
                body_handler(nullptr) {
    }
    

    void notify_header( std::unique_ptr<http_request_header> h ) {
            request_notifier->notify_header(move(h));
    } 

    size_t handle_body_chunk( const char* p , size_t l ) {
        return body_handler->handle_body_chunk(p,l);
    }

    void notify_body_end() {
            body_handler->notify_body_end();
    }
    void bind( BodyObserver * p) {
        body_handler = p;
    }
    void bind( RequestObserver * p) {
        request_notifier = p;
    }

    
    shared_ptr< RequestObserver> make_request_observer();
    shared_ptr< BodyObserver> make_body_observer();

    template<   typename ReadHandler >
     void async_read(ReadHandler handler) {
         
         if ( this->flush() ) {
             cerr << "async_read: flush " << endl;
             socket.get_io_service().post([handler]() {
                 handler( boost::system::error_code() );
             });
         } else {
          cerr << "async_read: start" << endl;
          auto towrite = rb.prepare();
          auto self( this->shared_from_this() );
          socket.async_read_some( towrite, [this,self, handler]( boost::system::error_code ec, std::size_t bytes ) {
              cerr << "async_read_some " << bytes << endl;
              rb.commit(bytes);
              auto toread = rb.data();
              size_t consumed = this->parse( buffer_cast<const char*>(toread), buffer_size(toread)  );
                 
              if ( consumed == 0 and not this->paused() and not ec ) {
                            async_read(handler);
                } else {
                        rb.consume(consumed);
                        handler( ec );
                }
           });  
         }
    }

};

template< class RequestObserver >
class body_reader : public std::enable_shared_from_this< body_reader<RequestObserver>>  {
    typedef shared_ptr< connection<RequestObserver, body_reader<RequestObserver> > > conn_ptr;

    conn_ptr          cnn;
    
    public:

    body_reader()  {

    }
    body_reader( conn_ptr p ) :
        cnn(p),
        current_buffer((char*)"",0) 
        {
            cnn->bind(this);
        }
    ~body_reader() {
        cnn->bind(static_cast<decltype(this)>(nullptr));
    }

    boost::asio::mutable_buffer  current_buffer;
    bool                            completed = false;

    size_t handle_body_chunk( const char* p , size_t l ) {
        cerr << "handle_body_chunk: " << l << endl;
        size_t used = buffer_copy( current_buffer, const_buffers_1(p,l) );
        current_buffer = current_buffer + used;
        return used;
    }

    void notify_body_end() {
            completed = true;
    }

    template< class ReadHandler > 
    void async_read_some(
        const boost::asio::mutable_buffer& buffer,
            ReadHandler handler) {
                    current_buffer = buffer;
                    size_t origlen = buffer_size(buffer);
                    auto self( this->shared_from_this() );
                    cnn->async_read( [this,self,origlen,handler](boost::system::error_code ec) {
                        size_t len = origlen - buffer_size(current_buffer);
                        if ( ec ) {
                           handler( ec,  len  );
                        } else if ( len ) {
                            handler( boost::system::error_code(), len  );
                        } else {
                            if ( completed ) {
                                    handler( boost::asio::error::eof, len  );
                            } else {
                                async_read_some(current_buffer, handler);
                            }
                        }
                    } );
            }
};



class request_reader   :
    public std::enable_shared_from_this< request_reader >
{
    typedef connection< request_reader,body_reader<request_reader> > conn_t;
    typedef shared_ptr< conn_t > conn_ptr;

    conn_ptr cnn;
    public:
    
    request_reader( conn_ptr p ) :
        cnn(p) {
            cnn->bind(this);
        }
    ~request_reader() {
        cnn->bind(static_cast<decltype(this)>(nullptr));
    }

    void notify_header( std::unique_ptr<http_request_header> h ) {
            cerr << __FUNCTION__ << endl;
            ready = move(h);
    } 

    typedef std::unique_ptr<http_request_header>        http_request_header_ptr;
    typedef shared_ptr< body_reader<request_reader> >   body_reader_ptr ;

    std::unique_ptr<http_request_header> ready;

    template<   typename ReadHandler >
     void async_read_request(ReadHandler handler) {
            auto self( this->shared_from_this() );
            cnn->async_read( [this, self, handler](boost::system::error_code ec ) {
                        cerr << __FUNCTION__ << endl;
                if ( ec ) {
                        handler( ec, http_request_header_ptr(), body_reader_ptr() );
                } else if ( ready ) {
                        handler( boost::system::error_code(), move(ready), move( cnn->make_body_observer() ) );
                } else {
                    async_read_request(handler);
                }
            } );
     }
};


template< class RequestObserver, class BodyObserver >
shared_ptr< RequestObserver>
connection<RequestObserver,BodyObserver>::make_request_observer() {
    return make_shared<RequestObserver>(this->shared_from_this() );
}

template< class RequestObserver, class BodyObserver >
shared_ptr< BodyObserver> 
connection<RequestObserver,BodyObserver>::make_body_observer() {
    return make_shared<BodyObserver>(this->shared_from_this() );
}












struct test_reader : std::enable_shared_from_this<test_reader> {
    http_request_header_ptr h;
    request_reader::body_reader_ptr s;

    std::string body;
    mxmz::ring_buffer rb;

    test_reader() : rb(1024) {}     

    void start() {
        cerr << "test_reader: start " << endl;
        auto self( this->shared_from_this() );
        boost::asio::mutable_buffers_1 buff(rb.prepare() );
        s->async_read_some( buff, [buff,this,self](boost::system::error_code ec, size_t l)  {
                cerr << "test_reader: " << l << endl;
                body.append( buffer_cast<const char*>(buff), l );
                if ( ec ) {
                        cerr << "test_reader: " << ec << endl;
                } else {
                       start();
                }
        } );
    }
};






int srv_port = 60000 + (rand()%5000);

void test2() {
    io_service  ios;
       
    ip::tcp::resolver resolver(ios);
    ip::tcp::endpoint endpoint( ip::address::from_string("127.0.0.1"), srv_port);

    ip::tcp::acceptor acceptor(ios);

    ip::tcp::socket socket(ios);


    acceptor.open(endpoint.protocol());
    acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen();

    std::string s;

    
    typedef body_reader<request_reader> body_reader_t;
    typedef connection<request_reader, body_reader_t > conn_t;

    auto tr = std::make_shared<test_reader>();

    function<void()> start_accept = [tr,&acceptor,&socket,&start_accept]() {
        acceptor.async_accept( socket, [tr,&acceptor,&socket,&start_accept](boost::system::error_code ec)
        {
            if (!acceptor.is_open())
            {
                return;
            }

            if (!ec)
            {
                cerr << "new connnection" << endl;
               
                auto conn = make_shared<conn_t>( move(socket), 1024, 1024 ) ;
                auto rreader = conn->make_request_observer();
                rreader->async_read_request( [tr,conn]( boost::system::error_code ec, 
                                                     request_reader::http_request_header_ptr h,  
                                                     request_reader::body_reader_ptr br  ) {
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

    auto b1 = make_random_string( 1042 + rand() % 10042, 0, 256 );

    //auto b1 = make_random_string( 42 + rand() % 42, 'a', 'z' );
    

    string bodylen = boost::lexical_cast<string>( b1.size() );

    auto rand_head_name1 =  make_random_string(1 + rand() % 1042, 'a', 'a'+ 26 );
    auto rand_head_value1 = make_random_string(1 + rand() % 1042, 'a', 126  );

    auto rand_head_name2 =  make_random_string(1 + rand() % 1042, 'a', 'a'+ 26 );
    auto rand_head_value2 = make_random_string(1 + rand() % 1042, 'a', 126  );

    const string s1 = "POST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"
                      "Accept: */*\r\n"
                      + rand_head_name1 + ": " + rand_head_value1 + "\r\n"
                      "Transfer-Encoding: identity\r\n"
                      "Content-Length: " + bodylen + "\r\n"
                      + rand_head_name2 + ": " + rand_head_value2 + "\r\n"  
                      "\r\n"
                      + b1;
        

    thread t( [s1,f = move(srv_ready_future)]() {
        io_service ios;
        ip::tcp::socket conn(ios);
        f.wait();

        conn.connect( ip::tcp::endpoint( ip::address::from_string("127.0.0.1"), srv_port) );
        string source = s1; 
        

        const char* p = source.data();
        const char* end = p + source.size();
        while( p != end ) {
            size_t len = min( long(1024 + rand() % 1024)  , (end-p) );
            boost::system::error_code ec;
            auto rc = conn.write_some( const_buffers_1(p, len), ec );
            cerr << "t written " << rc << "  " << ec << endl;
            p += rc;
            std::this_thread::sleep_for( chrono::microseconds(( rand() % 100 ) / 96 ));
        }
        //std::this_thread::sleep_for( chrono::seconds( 1 ));
    });
   


    start_accept();
    srv_ready.set_value(true);
    ios.run();
    t.join();

        cerr << tr->body.size() << endl;
        cerr << b1.size() << endl;
        assert( tr->body ==  b1 );
        assert( tr->h );
        assert( tr->h->method == "POST" );
        assert( tr->h->url == "/post_identity_body_world?q=search#hey" );
        assert( (*tr->h)[rand_head_name1] == rand_head_value1 );
        assert( (*tr->h)[rand_head_name2] == rand_head_value2 );

} 


void run(const char* name, void(* func)(), int count )
{
    cout << name << " ... " << std::flush ;
    for( int i = 0; i < count ; ++i) func(); 
    cout << name << " ok" << endl;
}

#define RUN(f,count) run( #f, &f, count )

int main() {
    srand(time(nullptr));
    RUN( test1, verbose ? 1: 1000 );
    RUN( test2, verbose ? 1: 1000 );
}



#include "detail/http_inbound_connection_impl.hxx"