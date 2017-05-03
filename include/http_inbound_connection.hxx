#ifndef http_inbound_connection_394853985739857398623572653726537625495679458679458679486749
#define http_inbound_connection_394853985739857398623572653726537625495679458679458679486749

#include <memory>
#include <map>
#include <string>
#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "util/ring_buffer.hxx"
#include "util/pimpl.h"


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

struct http_response_header {
    typedef std::pair<int,string>  status_t; 
    status_t status;
    map<string,string> headers;
    const string& operator[]( const string& k) const;
    
    http_response_header( status_t&& s, map<string,string>&& h ) :
            status( move(s)), 
            headers( move(h)) 
            {}
};

typedef std::unique_ptr<const http_response_header>  http_response_header_ptr;

class http_response_header_builder;



/*
    Handlers must implement:
    -   void   handle_header( http_request_header_ptr h )
    -   size_t handle_body_chunk( const char* p , size_t l )
    -   void   handle_body_end()

*/
template< class Handlers, 
        template< class H > class ParserImplBase, 
        class RequestHeaderBuilder = http_request_header_builder 
        > 
class buffering_request_http_parser  {
        class detail;
        pimpl<detail,550,8> i;

        public:
        buffering_request_http_parser( Handlers&, size_t buffer_threshold );
        buffering_request_http_parser( Handlers*, size_t buffer_threshold );
        buffering_request_http_parser( size_t buffer_threshold );
        buffering_request_http_parser() = delete;
        buffering_request_http_parser( const buffering_request_http_parser& ) = delete;

        bool paused() const ;
        bool good()   const ;

        void reset();
        size_t parse(const char* buffer, size_t len, bool eof );

        typedef std::pair<size_t,size_t>    flush_info_t;
        
        flush_info_t flush();
        bool         buffering() const;
};
  
/*
    example:

     template< class Handlers >
       using default_parser_base = mxmz::nodejs::http_parser_base;

      typedef mxmz::inbound_connection_tmpl<default_parser_base> connection;

      acceptor.async_accept( socket, [ &socket ](boost::system::error_code ec) {
          if ( ! ec ) {
                auto conn = make_shared<connection>( move(socket), ... );
                conn->async_read_request( [conn]( boost::system::error_code ec, 
                                                     connection::http_request_header_ptr head  
                                                     ) {
                    CERR << ec <<  endl;
                        CERR << head->method << endl;
                        CERR << head->url << endl;
                        auto body = conn->make_body_reader();    
                        body->async_read_some( ... 
                        // ....

            } );
              

          }
           
      }
*/

template < class Connection >
class body_reader_tmpl ;


template<   template< class H > class ParserImplBase, 
            class Socket =  ip::tcp::socket, 
            template<class C> class BodyReaderTmpl = body_reader_tmpl  >
class inbound_connection_tmpl: 
    public  std::enable_shared_from_this< inbound_connection_tmpl<ParserImplBase, Socket> >, 
    public  mxmz::buffering_request_http_parser< inbound_connection_tmpl<ParserImplBase,Socket>,ParserImplBase > {

    public:
    typedef inbound_connection_tmpl<ParserImplBase,Socket> this_t;
    
    typedef Socket socket_t;

    typedef mxmz::buffering_request_http_parser<this_t,ParserImplBase> base_t;
    
    typedef BodyReaderTmpl<this_t>    body_observer_t;

    private:

    long int connection_id_ = 0;

    Socket              socket;
    mxmz::ring_buffer   rb;
    
    body_observer_t*       body_handler;
    size_t  bytes_transferred = 0;

    public:

    auto& get_io_service() {
        return socket.get_io_service();
    }

    auto& get_socket() {
        return socket;
    }

    long int connection_id () const {
        return (long int)connection_id_;
    }

    auto      buffered_data ()  {
            return rb.data() ;
    }

    inbound_connection_tmpl ( Socket&& s, size_t buffer_threshold, size_t buffer_size) :
                base_t(buffer_threshold),
                socket( move(s)), 
                rb( buffer_size ),
                body_handler(nullptr) {
                CERR << connection_id() << " open" << endl;           
    }

    inbound_connection_tmpl ( long int id, Socket&& s, size_t buffer_threshold, size_t buffer_size) :
                base_t(buffer_threshold),
                connection_id_(id),
                socket( move(s)), 
                rb( buffer_size ),
                body_handler(nullptr) {
                CERR << connection_id() << " open" << endl;           
    }
    ~inbound_connection_tmpl() {
        CERR << connection_id() << " closed  " << last_ec << " " << bytes_transferred <<  endl;
    }
    boost::system::error_code last_ec;

    size_t handle_body_chunk( const char* p , size_t l ) {
        return body_handler->handle_body_chunk(p,l);
        }

    void   handle_body_end() {
            body_handler->  handle_body_end();
    }
    void bind( body_observer_t* p) {
        body_handler = p;
    }


    template<   typename ReadHandler >
        void post(ReadHandler handler) {
                socket.get_io_service().post(handler);
        }


    template<   typename ReadHandler >
     void async_read(ReadHandler handler) {
         auto self( this->shared_from_this() );
         auto processData = [this,self, handler]( boost::system::error_code ec, std::size_t bytes ) {

            last_ec = ec; 
            bytes_transferred += bytes;
         CERR << connection_id() << " <<<<<<<<<<<<<<<<<<<<<<<< connection socket.async_read_some bytes: "<< ec  << " " << bytes <<  " " << bytes_transferred << endl;
            rb.commit(bytes);
            
            auto toread = rb.data();
           CERR << connection_id() << " connection::async_read_some parsing  " << buffer_size(toread) << endl;
            size_t consumed = this->parse( buffer_cast<const char*>(toread), buffer_size(toread), false  );
           CERR << connection_id() << " connection::async_read_some parsed: " << consumed << " " << this->paused()  << " " <<  ec <<  " " << buffer_size(toread) << " " << this->buffering() << endl;
            rb.consume(consumed);
            if ( this->buffering() ) { // still something to flush
                    ec = boost::system::error_code() ;
            }
            handler( ec );
        };

        if ( rb.readable() or this->buffering() ) {
            CERR << connection_id()<< " async_read: start: unparsed stuff" << endl;
            socket.get_io_service().post([processData]() {
                processData( boost::system::error_code(), 0  );
            });
        } else {
            auto towrite = rb.prepare();
//            CERR << "async_read: start: towrite " <<buffer_size(towrite) << endl;
            socket.async_read_some( towrite, processData );
        }
    }


    typedef std::unique_ptr<const http_request_header>      http_request_header_ptr;
    typedef shared_ptr< body_observer_t >                   body_reader_ptr ;

    auto make_body_reader() {
        return body_reader_ptr( new body_observer_t(this->shared_from_this()));
    }

    std::unique_ptr< const http_request_header> request_ready;

    void handle_header( std::unique_ptr< const http_request_header> h ) {
        request_ready = move(h);
    }    

    template<   typename ReadHandler >
    void async_wait_request(ReadHandler handler) {
        auto self( this->shared_from_this() );
        async_read( [this, self, handler](boost::system::error_code ec ) {
//                    CERR << __FUNCTION__ << endl;
            if ( ec ) {
                    handler( ec, http_request_header_ptr() );
            } else if ( request_ready ) {
                    handler( boost::system::error_code(), move(request_ready) );
            } else {
                async_wait_request(handler);
            }
        } );
    }




};


template < class Connection >
class body_reader_tmpl : 
    public std::enable_shared_from_this< body_reader_tmpl<Connection> >  {

    typedef shared_ptr< Connection > conn_ptr;

    conn_ptr          cnn;
    
    public:

    conn_ptr connection() const {
        return cnn;
    }

    body_reader_tmpl() = delete;    

    auto& get_io_service() {
        return cnn->get_io_service();
    }

    private:
    friend Connection;

    body_reader_tmpl( conn_ptr p ) :
        cnn(move(p)),
        current_buffer((char*)"",0) 
        {
            cnn->bind(this);
        }

    public:    
    ~body_reader_tmpl() {
        cnn->bind(nullptr);
    }

    boost::asio::mutable_buffer  current_buffer;
    bool                            eof = false;

    size_t handle_body_chunk( const char* p , size_t l ) {
//        CERR << "handle_body_chunk: got " << l <<  " avail " << buffer_size(current_buffer) << endl;
        size_t used = buffer_copy( current_buffer, const_buffers_1(p,l) );
        current_buffer = current_buffer + used;
//        CERR << "handle_body_chunk: used " << used << endl;
        return used;
      }

    void   handle_body_end() {
//            CERR << __FUNCTION__ << endl;
            eof = true;
    }

    template< class ReadHandler > 
    void async_read_some(
        const boost::asio::mutable_buffer& buffer,
            ReadHandler handler) {
                    //CERR << "body_reader async_read_some eof  " << eof << endl;
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
                            //CERR << "body_reader async_read_some lambda  " << ec << " " << len << endl;
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