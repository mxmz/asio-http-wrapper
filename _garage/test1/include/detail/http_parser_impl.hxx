#include "../../http-parser/http_parser.h"
#include <iostream>
using namespace std;

namespace mxmz {

template <class Handlers>
struct http_parser_base<Handlers>::detail {
    typedef http_parser_base<Handlers> owner_type;

    Handlers& handlers;
    struct http_parser* parser;
    struct http_parser_settings settings;
    enum http_parser_type type;

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

    void reset() { http_parser_init(parser, type); }

    ~detail() { free(parser); }


    size_t parse(const char* buffer, size_t len)
    {
        size_t rv = http_parser_execute(parser, &settings, buffer, len);
        cerr << parser->http_errno << endl;
        cerr << http_errno_name(http_errno(parser->http_errno)) << endl;
        cerr << http_errno_description(http_errno(parser->http_errno)) << endl;
        cerr << "." << endl;
        if (parser->http_errno) {
            handlers.on_error(parser->http_errno,
                              http_errno_description(http_errno(parser->http_errno)));
        }
        return rv;
    }

    /* callback functions */

    static Handlers& get_handlers(http_parser* p)
    {
        return static_cast<detail*>(p->data)->handlers;
    }

    static int header_field_cb(http_parser* p, const char* buf, size_t len)
    {
        cerr << __FUNCTION__ << ": ";
        cerr.write(buf, len) << endl;
        
        cerr << p->http_major << endl;
        cerr << p->http_minor << endl;
        cerr << p->status_code << endl;
        cerr << p->method << endl;
        cerr << http_method_str((http_method)p->method) << endl;
        cerr << p->http_errno << endl;
        cerr << "." << endl;

        return 0;
    }

    static int header_value_cb(http_parser* p, const char* buf, size_t len)
    {
        cerr << __FUNCTION__ << ": ";
        cerr.write(buf, len) << endl;
        return 0;
    }

    static int body_cb(http_parser* p, const char* buf, size_t len)
    {
        cerr << __FUNCTION__ << ": ";
        cerr.write(buf, len) << endl;
        ;

        get_handlers(p).on_body(buf, len);

        return 0;
    }

    static int message_begin_cb(http_parser* p)
    {
        cerr << __FUNCTION__ << endl;
        cerr << p->http_errno << endl;
        cerr << "." << endl;
        cerr << p->http_major << endl;
        cerr << p->http_minor << endl;
        cerr << p->status_code << endl;
        cerr << p->method << endl;
        cerr << http_method_str((http_method)p->method) << endl;
        cerr << p->http_errno << endl;
        cerr << "." << endl;

        return 0;
    }

    static int headers_complete_cb(http_parser* p)
    {
        cerr << __FUNCTION__ << endl;
        cerr << p->http_major << endl;
        cerr << p->http_minor << endl;
        cerr << p->status_code << endl;
        cerr << p->method << endl;
        cerr << http_method_str((http_method)p->method) << endl;
        cerr << p->http_errno << endl;
        cerr << "." << endl;

        return 0;
    }

    static int message_complete_cb(http_parser* p)
    {
        cerr << __FUNCTION__ << endl;
        return 0;
    }

    static int response_status_cb(http_parser* p, const char* buf, size_t len)
    {
        cerr << __FUNCTION__ << ": ";
        cerr.write(buf, len) << endl;
        ;
        return 0;
    }

    static int request_url_cb(http_parser* p, const char* buf, size_t len)
    {
        cerr << __FUNCTION__ << ": ";
        cerr.write(buf, len) << endl;
        ;
        return 0;
    }
};
}

namespace {
}

namespace mxmz {

template <class Handlers>
http_parser_base<Handlers>::http_parser_base(mode_t mode, Handlers& hndls)
    : i(new detail(mode, hndls))
{
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
