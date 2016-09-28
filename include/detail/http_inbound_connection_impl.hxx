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
    if ( i->buffering() ) {
        i->flush();
        return 0;
    }
    if ( len ) {
        i->unpause();
        return i->parse(buffer, len);
    } else {
        return 0;
    }
}

template <class Handlers, class RHB>
bool buffering_request_http_parser<Handlers,RHB>::buffering() const
{   
        return i->buffering();
}



}



#include "detail/http_parser_impl.hxx"
#endif