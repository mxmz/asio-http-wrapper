#include "../../http-parser/http_parser.h"

namespace mxmz {

        template < class Derived >
        struct http_parser_base<Derived>::detail  {
     
                struct http_parser* parser;

                detail();
                ~detail();
        };

}

namespace {

        int
        header_field_cb (http_parser *p, const char *buf, size_t len)
        {
          return 0;
        }

        int
        header_value_cb (http_parser *p, const char *buf, size_t len)
        {
          return 0;
        }

        int
        body_cb (http_parser *p, const char *buf, size_t len)
        {
          return 0;
        }

        int
        message_begin_cb (http_parser *p)
        {
          return 0;
        }

        int
        headers_complete_cb (http_parser *p)
        {
          return 0;
        }

        int
        message_complete_cb (http_parser *p)
        {
          return 0;
        }

        int
        response_status_cb (http_parser *p, const char *buf, size_t len)
        {
          return 0;
        }

        int
        request_url_cb (http_parser *p, const char *buf, size_t len)
        {
          return 0;
        }

        static constexpr http_parser_settings settings =
          { 
            message_begin_cb
          , request_url_cb
          , response_status_cb
          , header_field_cb
          , header_value_cb
          , headers_complete_cb
          , body_cb
          , message_complete_cb
          };


}

namespace mxmz {

        template < class Derived >
        http_parser_base<Derived>::http_parser_base()  : i_( new detail() ) {

        }


        template< class Derived>
           http_parser_base<Derived>::detail::detail() {
                parser = (http_parser*)malloc(sizeof(http_parser));
                http_parser_init(parser, HTTP_BOTH);
           }

        template< class Derived>
           http_parser_base<Derived>::detail::~detail() {
                free(parser);
           }

}
