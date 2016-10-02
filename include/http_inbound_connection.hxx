#ifndef http_inbound_connection_394853985739857398623572653726537625495679458679458679486749
#define http_inbound_connection_394853985739857398623572653726537625495679458679458679486749

#include <memory>
#include <map>
#include <string>
#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "http_parser.hxx"
#include "ring_buffer.hxx"
#include "test.h"

namespace mxmz {

using std::string;
using std::map;
using std::move;
using std::shared_ptr;
using std::unique_ptr;
using namespace boost::asio;

struct http_request_header {
    string method;
    string url;
    map<string,string> headers;
    const string& operator[]( const string& k) const;
    
    http_request_header( string&& m, string&& u, map<string,string>&& h ) :
            method( move(m)), 
            url( move(u)),
            headers( move(h)) 
            {}
};

typedef std::unique_ptr<const http_request_header>  http_request_header_ptr;

class http_request_header_builder;

/*
    Handlers must implement:
    -   void notify_header( http_request_header_ptr h )
    -   size_t handle_body_chunk( const char* p , size_t l )
    -   void notify_body_end()

*/
template< class Handlers, class RequestHeaderBuilder = http_request_header_builder > 
class buffering_request_http_parser  {
        class detail;
        std::unique_ptr<detail> i;

        public:
        buffering_request_http_parser( Handlers&, size_t buffer_threshold );
        buffering_request_http_parser( Handlers*, size_t buffer_threshold );
        buffering_request_http_parser( size_t buffer_threshold );
        buffering_request_http_parser() = delete;
        buffering_request_http_parser( const buffering_request_http_parser&& ) = delete;

        bool paused() const ;
        bool good()   const ;

        void reset();
        size_t parse(const char* buffer, size_t len );

        typedef std::pair<size_t,size_t>    flush_info_t;
        
        flush_info_t flush();
        bool         buffering() const;
};
  
/*
    usage:

      typedef mxmz::connection_tmpl<mxmz::body_reader> connection;

      acceptor.async_accept( socket, [ &socket ](boost::system::error_code ec) {
          if ( ! ec ) {
                auto conn = make_shared<connection>( move(socket), ... );
                conn->async_read_request( [conn]( boost::system::error_code ec, 
                                                     connection::http_request_header_ptr head,  
                                                     connection::body_reader_ptr body  ) {
                    CERR << ec <<  endl;
                        CERR << head->method << endl;
                        CERR << head->url << endl;

                        body->async_read_some( ... 
                        // ....

            } );
              

          }
           
      }
*/



template< class BodyObserver >
class connection_tmpl: 
    public   std::enable_shared_from_this< connection_tmpl<BodyObserver> >, 
    public  mxmz::buffering_request_http_parser< connection_tmpl<BodyObserver> > {

    typedef mxmz::buffering_request_http_parser<connection_tmpl<BodyObserver>> base_t;

    ip::tcp::socket     socket;
    mxmz::ring_buffer   rb;
    BodyObserver*       body_handler;
    size_t  bytes_transferred = 0;

    public:

    auto      buffered_data ()  {
            return rb.data() ;
    }

    connection_tmpl ( ip::tcp::socket&& s, size_t buffer_threshold, size_t buffer_size) :
                base_t(buffer_threshold),
                socket( move(s)), 
                rb( buffer_size ),
                body_handler(nullptr) {
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


    shared_ptr< BodyObserver> make_body_observer();

    template<   typename ReadHandler >
        void post(ReadHandler handler) {
                socket.get_io_service().post(handler);
        }


    template<   typename ReadHandler >
        void async_read(ReadHandler handler);


    typedef std::unique_ptr<const http_request_header>      http_request_header_ptr;
    typedef shared_ptr< BodyObserver >                      body_reader_ptr ;

    std::unique_ptr< const http_request_header> request_ready;

    void notify_header( std::unique_ptr< const http_request_header> h ) {
        request_ready = move(h);
    }    

    template<   typename ReadHandler >
    void async_read_request(ReadHandler handler);


};



class body_reader : public std::enable_shared_from_this< body_reader>  {
    typedef shared_ptr< connection_tmpl< body_reader > > conn_ptr;

    conn_ptr          cnn;
    
    public:

    conn_ptr connection() const {
        return cnn;
    }

    body_reader()  {

    }
    body_reader( conn_ptr p ) :
        cnn(move(p)),
        current_buffer((char*)"",0) 
        {
            cnn->bind(this);
        }
    ~body_reader() {
        cnn->bind(nullptr);
    }

    boost::asio::mutable_buffer  current_buffer;
    bool                            eof = false;

    size_t handle_body_chunk( const char* p , size_t l ) {
        CERR << "handle_body_chunk: got " << l <<  " avail " << buffer_size(current_buffer) << endl;
        size_t used = buffer_copy( current_buffer, const_buffers_1(p,l) );
        current_buffer = current_buffer + used;
        CERR << "handle_body_chunk: used " << used << endl;
        return used;
    }

    void notify_body_end() {
            CERR << __FUNCTION__ << endl;
            eof = true;
    }

    template< class ReadHandler > 
    void async_read_some(
        const boost::asio::mutable_buffer& buffer,
            ReadHandler handler) {
                    CERR << "body_reader async_read_some eof  " << eof << endl;
                    if ( eof ) {
                            cnn->post( [handler]() {
                                  handler(boost::asio::error::eof, 0);      
                            } );
                    } else {
                        current_buffer = buffer;
                        size_t origlen = buffer_size(buffer);
                        auto self( this->shared_from_this() );
                        cnn->async_read( [this,self,origlen,handler](boost::system::error_code ec) {
                            size_t len = origlen - buffer_size(current_buffer);
                            CERR << "body_reader async_read_some lambda  " << ec << endl;
                            if ( eof ) ec = boost::asio::error::eof;  
                            if ( ec ) {
                                handler( ec,  len  );
                            } else if ( len ) {
                                handler( boost::system::error_code(), len  );
                            } else {
                                async_read_some(current_buffer, handler);
                            }
                        } );
                    }
            }
};



















} //namespace xmmz

#endif