#ifndef http_inbound_connection_impl_394853985739857398623572653726537625495679458679458679486749
#define http_inbound_connection_impl_394853985739857398623572653726537625495679458679458679486749

#include "http_inbound_connection.hxx"
#include <iostream>
#include <memory>
#include <vector>
#include "http_parser.hxx"
#include "http_inbound_connection.hxx"
#include <cassert>
#include <cstring>

namespace mxmz {

using namespace std;
using boost::asio::mutable_buffers_1;
using boost::asio::buffer_cast;
using boost::asio::mutable_buffer;
using boost::asio::const_buffers_1;
using boost::asio::const_buffer;
using boost::system::error_code;

class http_request_header_builder {
        map< string, string >   _headers;
        string                  _method;
        string                  _request_uri;
        public:
        http_request_header_builder& set_method ( string&& method ) {
            _method = move(method);
            return *this;
        }     
        http_request_header_builder& set_request_uri ( string&& request_uri ) {
            _request_uri = move(request_uri);
            return *this;
        }
        http_request_header_builder& add_header ( string&& k, string&& v ) {
            _headers[k] = move(v);
            return *this;
        }

        http_request_header_ptr build() {
            http_request_header_ptr r( 
                    new const http_request_header{move(_method), move(_request_uri), move(_headers)} 

                );
            _method.clear(); _request_uri.clear(); _headers.clear();
            return r;
        }
        bool ready () const {
            return _method.size() and _request_uri.size();
        }

        typedef http_request_header_ptr type_ptr;

};

const string& http_request_header::operator[]( const string& k) const {
        auto found = headers.find(k);
        if ( found != headers.end() ) {
            return found->second;
        } else {
            static string empty;
            return empty;
        }
}

class consumable_buffer {
    std::vector<char> storage;
    size_t            consumed;
    public:
    consumable_buffer() : consumed(0) {}

    void append( const char* p, size_t l ) {
         for( auto i = p; i != p+l; ++i ) storage.emplace_back( *i );
     }

    const_buffers_1 data() const {
        return const_buffers_1( storage.data() + consumed, storage.size() - consumed ); 
    }
    void consume( size_t l) {
        consumed += l;
        if ( consumed == storage.size() ) {
            storage.clear();
            consumed = 0;
        }

    }

    bool empty() const {
        return buffer_size(data()) == 0;
    }
};


template< class Handlers, class RequestHeaderBuilder> 
class buffering_request_http_parser<Handlers,RequestHeaderBuilder>::detail : 
    public  mxmz::http_parser_base< buffering_request_http_parser<Handlers, RequestHeaderBuilder>::detail >
{
    typedef mxmz::http_parser_base< buffering_request_http_parser<Handlers, RequestHeaderBuilder>::detail > base_t;
        
    
    struct state_data {
        RequestHeaderBuilder hrh_builder;
        consumable_buffer       body_buffered;
        error_code              error;
        bool                    complete = false;
    };

public:    
    typedef typename RequestHeaderBuilder::type_ptr header_ptr;

    
private: 
    const size_t buffer_threshold;

    state_data             state;

    Handlers* handlers;

    using base_t::pause;
public:



    detail( const detail& ) = delete;
    detail(  detail&& ) = delete;

    
    
    detail(Handlers* c, size_t buffer_threshold = std::numeric_limits<size_t>::max()  ) :
        base_t( base_t::Request ),
        buffer_threshold(buffer_threshold),
        handlers(c)
    {
   
    }


    void on_response_headers_complete( int code, const string& response_status ) {
        std::abort();
    }

    void on_header_line( std::string&& name, string&& value )  {
        cerr << __FUNCTION__ << endl;
        state.hrh_builder.add_header( move(name), move(value) );
    }
    void  on_request_headers_complete( string&& method, string&& request_url ) {
        cerr << __FUNCTION__ << endl;
        http_request_header_ptr h(  
            move(  state.hrh_builder
                .set_method(move(method))
                .set_request_uri(move(request_url))
                .build() ) );
        handlers->notify_header(move(h));        
        this->pause();     
    };

    void on_error(int http_errno, const char* msg)
    {
        cerr << "Error: " << http_errno << " " << msg << endl;
        std::abort();
    }
    void on_message_complete()
    {
        this->pause();     
        state.complete = true;
        cerr << __FUNCTION__ << endl;
        flush();
    }

    void on_message_begin()
    {     state.complete = false;
//        cerr << "Message begin" << endl;
        cerr << __FUNCTION__ << endl;
    }
    

    void reset() {
        base_t::reset();
    }

        
    void on_body( const char* p, size_t l) {
                cerr << __FUNCTION__ << endl;
            flush();
            if ( state.body_buffered.empty() ) {
                size_t consumed = handlers->handle_body_chunk( p, l );
                cerr << "on_body: buffering " << l - consumed << endl;
                state.body_buffered.append(p + consumed, l - consumed );
            } else {
                state.body_buffered.append(p , l );
            }
            if ( buffer_size( state.body_buffered.data()) > buffer_threshold  ) {
                this->pause();
            }    
    }

    typedef std::pair<size_t,size_t>    flush_info_t;

   flush_info_t flush() {
       cerr << "flush" << endl;
       size_t flushed = 0;
        if ( not state.body_buffered.empty()  ) {
            auto data = state.body_buffered.data();
            flushed = handlers->handle_body_chunk( buffer_cast<const char*>(data), buffer_size(data) );
            state.body_buffered.consume(flushed);
        }
        cerr << "flush " <<  buffer_size( state.body_buffered.data()) << " " << state.body_buffered.empty() << " " << state.complete<<   endl;
        if ( state.complete and state.body_buffered.empty() ) {
            handlers->notify_body_end();
            state.complete = false;
        }
        //return buffer_size( state.body_buffered.data());
        return {flushed,buffer_size( state.body_buffered.data())};
   }
        
   bool buffering() const {
       return buffer_size(state.body_buffered.data()) > 0;
   }
   inline size_t parse(const char* buffer, size_t len)
    {
        if ( buffering() ) {
            flush();
            return 0;
        }
        if ( len ) {
            this->unpause();
            return this->base_t::parse(buffer, len);
        } else {
            return 0;
        }
    } 
    
};
         
   


template <class Handlers, class RHB>
buffering_request_http_parser<Handlers,RHB>::buffering_request_http_parser(Handlers* hndls, size_t buffer_threshold )
    : i( new detail(hndls, buffer_threshold) )
{
}

template <class Handlers, class RHB>
buffering_request_http_parser<Handlers,RHB>::buffering_request_http_parser(Handlers& hndls, size_t buffer_threshold )
    : i( new detail(&hndls, buffer_threshold) )
{
}

template <class Handlers, class RHB>
buffering_request_http_parser<Handlers,RHB>::buffering_request_http_parser(size_t buffer_threshold )
    : i( new detail(static_cast<Handlers*>(this), buffer_threshold) )
{
}


template <class Handlers, class RHB>
bool buffering_request_http_parser<Handlers,RHB>::paused() const 
{
    return i->paused();
}

template <class Handlers, class RHB>
bool buffering_request_http_parser<Handlers,RHB>::good() const 
{
    return i->good();
}

template <class Handlers, class RHB>
void buffering_request_http_parser<Handlers,RHB>::reset()
{
    return i->reset();
}
template <class Handlers, class RHB>
auto buffering_request_http_parser<Handlers,RHB>::flush() -> flush_info_t
{
    return i->flush();
}

template <class Handlers, class RHB>
size_t buffering_request_http_parser<Handlers,RHB>::parse(const char* buffer, size_t len)
{
    return i->parse(buffer, len);
}

template <class Handlers, class RHB>
bool buffering_request_http_parser<Handlers,RHB>::buffering() const
{   
        return i->buffering();
}



/*

template< class BodyObserver >
class connection_tmpl: 
    public   std::enable_shared_from_this< connection_tmpl<BodyObserver> >, 
    public  mxmz::buffering_request_http_parser< connection_tmpl<BodyObserver> > {

    typedef mxmz::buffering_request_http_parser<connection_tmpl<BodyObserver>> base_t;

    ip::tcp::socket     socket;
    mxmz::ring_buffer   rb;
    BodyObserver*       body_handler;

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

    size_t counter = 0;

    template<   typename ReadHandler >
     void async_read(ReadHandler handler) {

          
          auto self( this->shared_from_this() );
                     
          auto processData = [this,self, handler]( boost::system::error_code ec, std::size_t bytes ) {
                counter += bytes;
                cerr << "<<<<<<<<<<<<<<<<<<<<<<<< connection socket.async_read_some bytes: " << bytes <<  " " << counter << endl;
                rb.commit(bytes);
                
                auto toread = rb.data();
                cerr << "connection::async_read_some parsing  " << buffer_size(toread) << endl;
                size_t consumed = this->parse( buffer_cast<const char*>(toread), buffer_size(toread)  );
                cerr << "connection::async_read_some parsed: " << consumed << " " << this->paused()  << " " <<  ec <<  " " << buffer_size(toread) << " " << this->buffering() << endl;
                rb.consume(consumed);
                if ( this->buffering() ) { // still something to flush
                        ec = boost::system::error_code() ;
                }
                handler( ec );
           };

           if ( rb.readable() or this->buffering() ) {
               cerr << "async_read: start: unparsed stuff" << endl;
               socket.get_io_service().post([processData]() {
                   processData( boost::system::error_code(), 0  );
               });
           } else {
                auto towrite = rb.prepare();
               cerr << "async_read: start: towrite " <<buffer_size(towrite) << endl;
               socket.async_read_some( towrite, processData );
           }
         }


        typedef std::unique_ptr<const http_request_header>      http_request_header_ptr;
        typedef shared_ptr< BodyObserver >                      body_reader_ptr ;

        std::unique_ptr< const http_request_header> request_ready;

        void notify_header( std::unique_ptr< const http_request_header> h ) {
            request_ready = move(h);
        }    

*/        

template<   typename BodyObserver>
template<   typename ReadHandler >
void 
connection_tmpl<BodyObserver>::async_read_request(ReadHandler handler) {
        auto self( this->shared_from_this() );
        async_read( [this, self, handler](boost::system::error_code ec ) {
                    cerr << __FUNCTION__ << endl;
            if ( ec ) {
                    handler( ec, http_request_header_ptr(), body_reader_ptr() );
            } else if ( request_ready ) {
                    handler( boost::system::error_code(), move(request_ready), move( make_body_observer() ) );
            } else {
                async_read_request(handler);
            }
        } );
}


template<   typename BodyObserver>
template<   typename ReadHandler >
void 
connection_tmpl<BodyObserver>::async_read(ReadHandler handler) {
        auto self( this->shared_from_this() );
                    
        auto processData = [this,self, handler]( boost::system::error_code ec, std::size_t bytes ) {
            bytes_transferred += bytes;
            cerr << "<<<<<<<<<<<<<<<<<<<<<<<< connection socket.async_read_some bytes: " << bytes <<  " " << bytes_transferred << endl;
            rb.commit(bytes);
            
            auto toread = rb.data();
            cerr << "connection::async_read_some parsing  " << buffer_size(toread) << endl;
            size_t consumed = this->parse( buffer_cast<const char*>(toread), buffer_size(toread)  );
            cerr << "connection::async_read_some parsed: " << consumed << " " << this->paused()  << " " <<  ec <<  " " << buffer_size(toread) << " " << this->buffering() << endl;
            rb.consume(consumed);
            if ( this->buffering() ) { // still something to flush
                    ec = boost::system::error_code() ;
            }
            handler( ec );
        };

        if ( rb.readable() or this->buffering() ) {
            cerr << "async_read: start: unparsed stuff" << endl;
            socket.get_io_service().post([processData]() {
                processData( boost::system::error_code(), 0  );
            });
        } else {
            auto towrite = rb.prepare();
            cerr << "async_read: start: towrite " <<buffer_size(towrite) << endl;
            socket.async_read_some( towrite, processData );
        }
}



template<  class BodyObserver >
shared_ptr< BodyObserver> 
connection_tmpl<BodyObserver>::make_body_observer() {
    return make_shared<BodyObserver>(this->shared_from_this() );
}
































}



#include "detail/http_parser_impl.hxx"
#endif