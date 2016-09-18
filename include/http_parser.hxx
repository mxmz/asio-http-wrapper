#ifndef http_parser_29834729834729834729384729482741762537165371635176235173651723615723
#define http_parser_29834729834729834729384729482741762537165371635176235173651723615723

#include <memory>


namespace mxmz {

class http_parser_state;

/*
    Handlers must implement:

    -   on_error( int , string )
    -   on_header_line( string, string )
    -   on_body( const char* , size_t )
    -   on_response_headers_complete( int, string )
    -   on_request_headers_complete( method, request_url )
    -   on_message_begin()
    -   on_message_complete()    
*/

template <class Handlers>
class http_parser_base {
    class detail;
    std::unique_ptr<detail> i;

public:
    enum mode_t { Request,
                  Response,
                  Both
                };
    http_parser_base( mode_t, Handlers& );
    http_parser_base( http_parser_state&&, Handlers& );
    http_parser_base( const http_parser_base& ) = delete;

    http_parser_base( mode_t mode );
    http_parser_base( http_parser_state&& ) ;
public:
    void pause();
    void unpause();

    bool paused() const ;
    bool good()   const ;

    void reset();
    size_t parse(const char* buffer, size_t len );

    http_parser_state&& move_state();
};



} //namespace xmmz

#endif