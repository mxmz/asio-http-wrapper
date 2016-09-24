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
            http_request_header_ptr r( new http_request_header{move(_method), move(_request_uri), move(_headers)} );
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
                state.body_buffered.append(p + consumed, l - consumed );
            } else {
                state.body_buffered.append(p , l );
            }
            if ( buffer_size( state.body_buffered.data()) > buffer_threshold  ) {
                this->pause();
            }    
    }

   size_t flush() {
       cerr << "flush" << endl;
        if ( not state.body_buffered.empty()  ) {
            auto data = state.body_buffered.data();
            size_t consumed = handlers->handle_body_chunk( buffer_cast<const char*>(data), buffer_size(data) );
            state.body_buffered.consume(consumed);
        }
        cerr << "flush " <<  buffer_size( state.body_buffered.data()) << " " << state.body_buffered.empty() << " " << state.complete<<   endl;
        if ( state.complete and state.body_buffered.empty() ) {
            handlers->notify_body_end();
            state.complete = false;
        }
        return buffer_size( state.body_buffered.data());
   }
        

    
};
         

    
/*

    void 
    async_read_request_header( read_header_completion_t  comp ) {
        if ( current->header_ready ) {
            stream.get_io_service().post( [this,comp](){
                boost::system::error_code ec;
               comp(ec, move(current->header_ready));
            });
        } else {
            stream.async_read_some(boost::asio::buffer(buffer),
                    [this,c = move(comp)](boost::system::error_code ec, std::size_t bytes_transferred)
            {
                if (!ec)
                {
                    size_t read = this->parse( buffer.data(), bytes_transferred );
                    assert( read == bytes_transferred );
                    async_read_request_header(move(c));
                } else {
                    c( ec, nullptr );
                }
            } );
        } 
    }

    

    

    void async_read_some( mutable_buffers_1 buff, read_body_completion_t comp ){
        if ( size_t consumed = current->consume_body(buff)) {
            stream.get_io_service().post([this,comp,consumed]{
                boost::system::error_code ec;
                comp( ec, consumed );
            });
        } else {
            size_t max_body_read = buffer_size(buff);
            stream.async_read_some(boost::asio::buffer(buffer, max_body_read),
                    [this,comp,buff](boost::system::error_code ec, std::size_t bytes_transferred)
            {
                if (!ec)
                {
                    size_t consumed = 0;
                    body_consumer = [this,&buff,&consumed]( const char* p, size_t l)->size_t {
                            consumed = buffer_copy( buff, const_buffers_1(p,l) );
                            cerr << "consume direct: " << consumed << endl;
                            return consumed;
                    };
                   
                    size_t read = this->parse( buffer.data(), bytes_transferred );
                    assert( read == bytes_transferred );
                    
                    comp( ec, consumed );
                } else {
                    comp( ec, 0 );
                }
            });
        }
    }

  */  
    


/*

template< class SrcStream >
void 
http_inbound_connection<SrcStream>::async_read_request_header( read_header_completion_t  comp ) {
    i->async_read_request_header(comp);
}

template< class SrcStream >
void 
http_inbound_connection<SrcStream>::async_read_some( boost::asio::mutable_buffers_1 buff, read_body_completion_t comp ){
    i->async_read_some(buff,comp);
}


template< class SrcStream >
http_inbound_connection<SrcStream>::http_inbound_connection( SrcStream&& s )
    : i( new detail(move(s)))
    {}

*/

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
size_t buffering_request_http_parser<Handlers,RHB>::flush()
{
    return i->flush();
}

template <class Handlers, class RHB>
size_t buffering_request_http_parser<Handlers,RHB>::parse(const char* buffer, size_t len)
{   
    i->unpause();
    return i->parse(buffer, len);
}

/*
template <class Handlers, class RHB>
http_parser_state&& 
buffering_request_http_parser<Handlers,RHB>::move_state() {
    return i->move_state();
}
*/





}



#include "detail/http_parser_impl.hxx"
#endif