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
};

  
//ypedef std::function< void ( boost::system::error_code ec ) >                  write_header_completion_t;

//typedef std::function< void ( boost::system::error_code ec, std::size_t bytes_transferred ) > write_body_completion_t;



template< class SrcStream >
class http_inbound_connection {
    class detail;
    std::unique_ptr<detail> i;

public:
    
    http_inbound_connection( SrcStream&& socket );


    typedef std::unique_ptr<http_request_header>  http_request_header_ptr;

    typedef std::function< void ( boost::system::error_code ec, http_request_header_ptr ) > read_header_completion_t;
    void async_read_request_header( read_header_completion_t );

    typedef std::function< void ( boost::system::error_code ec, std::size_t bytes_transferred ) > read_body_completion_t;
    void async_read_some( boost::asio::mutable_buffers_1, read_body_completion_t );

//    void async_write_response_header( http_response_header_ptr, write_header_completion_t );

//    void async_write_some( boost::asio::mutable_buffers_1, write_body_completion_t );

};



} //namespace xmmz

#endif