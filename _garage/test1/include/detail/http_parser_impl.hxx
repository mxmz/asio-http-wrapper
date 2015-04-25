#include "../../http-parser/http_parser.h"
#include <iostream>
#include <memory>

namespace mxmz {

using namespace std;

template< class Derived, class Tuple = pair< string, string > >
class tuple_builder {
    Tuple   tuple;
    size_t  last;

public:
    template< size_t I  >
    void add_tuple_chunk( const char* buf, size_t len ) {
        if ( I == 0 and last > 0 ) {
            Tuple ready( move(tuple) );
            static_cast<Derived&>(*this).on_tuple_ready( move(ready) );
            get<I>(tuple).assign(buf,len);
        } else {
            get<I>(tuple).append(buf,len);
        }
        last = I;
    }
    void flush_tuple() {
        add_tuple_chunk<0>("",0);
    }

    tuple_builder(): last(0) { }
};


template <class Handlers>
struct http_parser_base<Handlers>::detail : public tuple_builder< http_parser_base<Handlers>::detail > {
    typedef http_parser_base<Handlers> owner_type;

    Handlers& handlers;
    struct http_parser* parser;
    struct http_parser_settings settings;
    enum http_parser_type type;

    string          request_url;
    string          response_status;

    detail( mode_t mode, Handlers& hndls)
        : handlers(hndls)
    {
        parser = (http_parser*)malloc(sizeof(http_parser));
        type = mode == Request
               ? HTTP_REQUEST
               : (mode == Response ? HTTP_RESPONSE : HTTP_BOTH);

        settings.on_message_begin = &message_begin_cb;
        settings.on_url = &request_url_cb;
        settings.on_status = &response_status_cb;
        settings.on_header_field = &header_field_cb;
        settings.on_header_value = &header_value_cb;
        settings.on_headers_complete = &headers_complete_cb;
        settings.on_body = &body_cb;
        settings.on_message_complete = &message_complete_cb;
        parser->data = this;
        reset();
    }

    void reset() {
        http_parser_init(parser, type);
    }

    ~detail() {
        free(parser);
    }


    size_t parse(const char* buffer, size_t len)
    {
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
        http_parser_pause( parser , 1 );
    }
    void unpause() {
        http_parser_pause( parser , 0 );
    }

    /* callback functions */

    static detail& get_self(http_parser* p)
    {
        return *static_cast<detail*>(p->data);
    }

    static int header_field_cb(http_parser* p, const char* buf, size_t len)
    {
        auto& self ( get_self(p) );

        self.template add_tuple_chunk<0>(buf,len);

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
        get_self(p).template add_tuple_chunk<1>(buf,len);

        // cerr << __FUNCTION__ << ": ";
        // cerr.write(buf, len) << endl;
        return 0;
    }

    static int body_cb(http_parser* p, const char* buf, size_t len)
    {
        auto& self ( get_self(p) );
        // cerr << __FUNCTION__ << ": ";
        // cerr.write(buf, len) << endl;

        self.handlers.on_body(buf, len);

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

    void on_tuple_ready( tuple<string,string>&& t ) {
        handlers.on_header_line( move( get<0>(t) ), move( get<1>(t) ) );
    }

    static int headers_complete_cb(http_parser* p)
    {
        auto& self ( get_self(p) );
        self.flush_tuple();
        if ( HTTP_PARSER_ERRNO(p) == HPE_OK && p->status_code == 0 ) {
            string method( http_method_str((http_method)p->method) );
            string request_url ( move( self.request_url ) );
            self.handlers.on_request_headers_complete( move(method), move(request_url) );
        } else if ( HTTP_PARSER_ERRNO(p) == HPE_OK && p->status_code != 0 ) {
            string response_status( move( self.response_status ) );
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
        get_self(p).handlers.on_message_end();
        return 0;
    }

    static int response_status_cb(http_parser* p, const char* buf, size_t len)
    {
        // cerr << __FUNCTION__ << ": ";
        // cerr.write(buf, len) << endl;
        auto& self ( get_self(p) );
        self.response_status.append( buf, len );
        return 0;
    }

    static int request_url_cb(http_parser* p, const char* buf, size_t len)
    {
        // cerr << __FUNCTION__ << ": ";
        // cerr.write(buf, len) << endl;
        auto& self ( get_self(p) );
        self.request_url.append( buf, len );
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
}
