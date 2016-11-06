#ifndef http_parser_29834729834729834729384729482741762537165371635176235173651723615723
#define http_parser_29834729834729834729384729482741762537165371635176235173651723615723

#include <memory>
#include "util/pimpl.h"

namespace mxmz { namespace nodejs {

class http_parser_state;

/*
    Handlers must implement:

    -   on_error( int , string )
    -   on_header_line( string, string )
    -   on_body( const char* , size_t )
    -   on_response_headers_complete( status, description )
    -   on_request_headers_complete( method, request_url )
    -   on_message_begin()
    -   on_message_complete()    
*/

enum class parser_mode {
            Request,
            Response,
            Both
};

template <class Handlers>
class http_parser_base {
    class detail;
    pimpl<detail,330,8> i;

public:
    typedef parser_mode mode_t ;
                
    http_parser_base( mode_t, Handlers* );
    http_parser_base( mode_t, Handlers& ) ;
    http_parser_base( http_parser_state&&, Handlers* );
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



} } //namespace xmmz::nodejs

#endif