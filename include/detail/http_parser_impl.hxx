#include "../../http-parser/http_parser.h"
#include <iostream>
#include <memory>
#include "http_parser.hxx"

namespace mxmz {

using namespace std;

template< class Derived, class Tuple = pair< string, string > >
class tuple_builder {
    Tuple   tuple;
    size_t  last;


public:
    typedef Tuple tuple_t;
    template< size_t I, class ReadyFunc  >
    void add_tuple_chunk( const char* buf, size_t len, ReadyFunc notify ) {
        if ( I == 0 and last > 0 ) {
            tuple_t ready( move(tuple) );
            notify( move(ready) );
            get<I>(tuple).assign(buf,len);
        } else {
            get<I>(tuple).append(buf,len);
        }
        last = I;
    }

    tuple_builder(): last(0) { }
    void reset() {
        last = 0;
        tuple = move(tuple_t());
    }
};

struct http_parser_state : public tuple_builder< http_parser_state >{
    typedef tuple_builder< http_parser_state > base_t;
    http_parser*            parser;
    enum http_parser_type   type;
    string          request_url;
    string          response_status;

    http_parser_state(  http_parser_type t ) {
            parser = (http_parser*)malloc( sizeof(http_parser));
            type = t;
            reset();
    }
    ~http_parser_state() {
        free(parser);
    }
    void reset() {
        http_parser_init(parser, type);
        request_url.resize(0);
        response_status.resize(0);
        this->base_t::reset();
    }
    http_parser_state( const http_parser_state& ) = delete;
    http_parser_state( http_parser_state&& that ) 
        : base_t(that)   {
        swap( parser, that.parser );
        swap( type, that.type );
        swap( request_url, that.request_url );
        swap( response_status, that.response_status);
    }

};



template <class Handlers>
struct http_parser_base<Handlers>::detail   {

    static enum http_parser_type mode2type( mode_t mode ) {
    return  mode == Request
               ? HTTP_REQUEST
               : (mode == Response ? HTTP_RESPONSE : HTTP_BOTH) ;
}

    typedef http_parser_base<Handlers> owner_type;

    Handlers& handlers;
    std::unique_ptr<http_parser_state> state;
    struct http_parser_settings settings;


    detail( mode_t mode, Handlers& hndls)
        : handlers(hndls),
          state( std::unique_ptr<http_parser_state>(new http_parser_state(mode2type(mode))) ) 
    {
        state->parser->data = this;
        setup();
    }
    detail( http_parser_state&& that_state, Handlers& hndls)
        : handlers(hndls),
          state( std::unique_ptr<http_parser_state>(new http_parser_state(move(that_state)))) 
    {
        state->parser->data = this;
        setup();
    }
    void setup() {
        settings.on_message_begin = &message_begin_cb;
        settings.on_url = &request_url_cb;
        settings.on_status = &response_status_cb;
        settings.on_header_field = &header_field_cb;
        settings.on_header_value = &header_value_cb;
        settings.on_headers_complete = &headers_complete_cb;
        settings.on_body = &body_cb;
        settings.on_message_complete = &message_complete_cb;
    }

    void reset() {
        state->reset(); 
        state->parser->data = this;
    }

    ~detail() {
    }


    size_t parse(const char* buffer, size_t len)
    {
        http_parser* parser = state->parser;
        //cerr << http_errno_name(http_errno(parser->http_errno)) << endl;
        //http_parser_pause( parser, 0 );
        size_t rv = http_parser_execute(parser, &settings, buffer, len);
        // cerr << parser->http_errno << endl;
        // cerr << http_errno_name(http_errno(parser->http_errno)) << endl;
        // cerr << http_errno_description(http_errno(parser->http_errno)) << endl;
        // cerr << "." << endl;
        if ( HTTP_PARSER_ERRNO(parser) != HPE_PAUSED && HTTP_PARSER_ERRNO(parser) != HPE_OK ) {
            handlers.on_error(parser->http_errno,
                              http_errno_description(http_errno(parser->http_errno)));
        }
        return rv;
    }

    void pause() {
        http_parser_pause( state->parser , 1 );
    }
    void unpause() {
        http_parser_pause( state->parser , 0 );
    }

    http_parser_state&& move_state() {
        return move( *state );
    }

    /* callback functions */

    static detail& get_self(http_parser* p)
    {
        return *static_cast<detail*>(p->data);
    }

    static int header_field_cb(http_parser* p, const char* buf, size_t len)
    {
        auto& self ( get_self(p) );

        typedef http_parser_state::tuple_t tuple_t;

        self.state->template add_tuple_chunk<0>(buf,len, [&self]( tuple_t&& t) {
                self.handlers.on_header_line( move( get<0>(t) ), move( get<1>(t) ) );
        });

        // cerr << __FUNCTION__ << ": ";
        // cerr.write(buf, len) << endl;

        // cerr << p->http_major << endl;
        // cerr << p->http_minor << endl;
        // cerr << p->status_code << endl;
        // cerr << p->method << endl;
        // cerr << http_method_str((http_method)p->method) << endl;
        // cerr << p->http_errno << endl;
        // cerr << "." << endl;

        return 0;
    }

    static int header_value_cb(http_parser* p, const char* buf, size_t len)
    {
        auto& self ( get_self(p) );
        typedef http_parser_state::tuple_t tuple_t;

        self.state->template add_tuple_chunk<1>(buf,len, [&self]( tuple_t&& t) {
                self.handlers.on_header_line( move( get<0>(t) ), move( get<1>(t) ) );
        });
        // cerr << __FUNCTION__ << ": ";
        // cerr.write(buf, len) << endl;
        return 0;
    }

    static int body_cb(http_parser* p, const char* buf, size_t len)
    {
        // cerr << __FUNCTION__ << ": ";
        // cerr.write(buf, len) << endl;

        get_self(p).handlers.on_body(buf, len);

        return 0;
    }

    static int message_begin_cb(http_parser* p)
    {
        cerr << __FUNCTION__ << endl;
        get_self(p).handlers.on_message_begin();
        // cerr << p->http_errno << endl;
        // cerr << "." << endl;
        // cerr << p->http_major << endl;
        // cerr << p->http_minor << endl;
        // cerr << p->status_code << endl;
        // cerr << p->method << endl;
        // cerr << http_method_str((http_method)p->method) << endl;
        // cerr << p->http_errno << endl;
        // cerr << "." << endl;

        return 0;
    }

    static int headers_complete_cb(http_parser* p)
    {
        header_field_cb(p, "", 0 ); // force flush last header
        
        auto& self ( get_self(p) );
        
        if ( HTTP_PARSER_ERRNO(p) == HPE_OK && p->status_code == 0 ) {
            string method( http_method_str((http_method)p->method) );
            string request_url ( move( self.state->request_url ) );
            self.handlers.on_request_headers_complete( move(method), move(request_url) );
        } else if ( HTTP_PARSER_ERRNO(p) == HPE_OK && p->status_code != 0 ) {
            string response_status( move( self.state->response_status ) );
            self.handlers.on_response_headers_complete( p->status_code, move(response_status) );
        }

        //cerr << __FUNCTION__ << endl;
        //cerr << "major  " << p->http_major << endl;
        //cerr << "minor  " << p->http_minor << endl;
        //cerr << "status " << p->status_code << endl;
        //cerr << "method " << p->method << endl;
        //cerr << "method " << http_method_str((http_method)p->method) << endl;
        //cerr << "errno  " << p->http_errno << endl;
        //cerr << "." << endl;
        return 0;
    }

    static int message_complete_cb(http_parser* p)
    {
        // cerr << __FUNCTION__ << endl;
        get_self(p).handlers.on_message_complete();
        return 0;
    }

    static int response_status_cb(http_parser* p, const char* buf, size_t len)
    {
        // cerr << __FUNCTION__ << ": ";
        // cerr.write(buf, len) << endl;
        get_self(p).state->response_status.append( buf, len );
        return 0;
    }

    static int request_url_cb(http_parser* p, const char* buf, size_t len)
    {
        // cerr << __FUNCTION__ << ": ";
        // cerr.write(buf, len) << endl;
        get_self(p).state->request_url.append( buf, len );
        return 0;
    }
};
}

namespace {
}

namespace mxmz {

using namespace std;

template <class Handlers>
http_parser_base<Handlers>::http_parser_base(mode_t mode, Handlers& hndls)
    : i( new detail(mode, hndls) )
{
}

template <class Handlers>
http_parser_base<Handlers>::http_parser_base(mode_t mode)
    : i( new detail(mode, static_cast<Handlers&>(*this)))
{
}

template <class Handlers>
http_parser_base<Handlers>::http_parser_base(http_parser_state&& state, Handlers& hndls)
    : i( new detail(move(state), hndls) )
{
}

template <class Handlers>
http_parser_base<Handlers>::http_parser_base(http_parser_state&& state)
    : i( new detail(move(state), static_cast<Handlers&>(*this)))
{
}

template <class Handlers>
void http_parser_base<Handlers>::pause()
{
    return i->pause();
}

template <class Handlers>
void http_parser_base<Handlers>::unpause()
{
    return i->unpause();
}

template <class Handlers>
void http_parser_base<Handlers>::reset()
{
    return i->reset();
}
template <class Handlers>
size_t http_parser_base<Handlers>::parse(const char* buffer, size_t len)
{
    return i->parse(buffer, len);
}

template <class Handlers>
http_parser_state&& 
http_parser_base<Handlers>::move_state() {
    return i->move_state();
}

}
