#ifndef http_inbound_connection_394853985739857398623572653726537625495679458679458679486749
#define http_inbound_connection_394853985739857398623572653726537625495679458679458679486749

#include <memory>
#include <map>
#include <string>
#include <boost/asio/io_service.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "http_parser.hxx"

namespace mxmz {

using namespace std;

struct http_request_header {
    const string method;
    const string url;
    const map<string,string> headers;
    const string& operator[]( const string& v) const;
};

typedef std::unique_ptr<http_request_header>  http_request_header_ptr;

class http_request_header_builder;

/*
    Handlers must implement:
    -   void notify_header( std::unique_ptr<http_request_header> h )
    -   size_t handle_body_chunk( const char* p , size_t l )
    -   void notify_body_end()

*/
template< class Handlers, class RequestHeaderBuilder = http_request_header_builder > 
class buffering_request_http_parser  {
        class detail;
        std::unique_ptr<detail> i;

        public:
        buffering_request_http_parser( Handlers&, size_t buffer_threshold );
        buffering_request_http_parser( Handlers*, size_t buffer_threshold );
        buffering_request_http_parser( size_t buffer_threshold );
        buffering_request_http_parser() = delete;
        buffering_request_http_parser( const buffering_request_http_parser&& ) = delete;

        bool paused() const ;
        bool good()   const ;

        void reset();
        size_t parse(const char* buffer, size_t len );

        typedef std::pair<size_t,size_t>    flush_info_t;
        
        flush_info_t flush();
};
  


} //namespace xmmz

#endif