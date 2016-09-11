#include "http_inbound_connection.hxx"
#include <iostream>
#include <memory>
#include "http_parser.hxx"
#include <cassert>

namespace mxmz {

using namespace std;
using boost::asio::mutable_buffers_1;
using boost::asio::buffer_cast;

template< class SrcStream >
class http_inbound_connection<SrcStream>::detail : 
    public  mxmz::http_parser_base<http_inbound_connection<SrcStream>::detail >
{
    typedef mxmz::http_parser_base<http_inbound_connection<SrcStream>::detail > base_t;

 SrcStream  stream;

 map< string, string > headers;

 http_request_header_ptr current_request;

 std::array<char, 8192> buffer;

 

 string  buffered_body;
 size_t buffered_body_consumed;

public:
    detail( detail& ) = delete;
    detail( SrcStream&& s ) :
        base_t( base_t::Request ),
        stream( move(s))
    {
        reset();
    }




    void on_response_headers_complete( int code, const string& response_status ) {
    }

    void  on_request_headers_complete( string&& method, const string&& request_url ) {
        http_request_header_ptr r( new http_request_header{move(method), move(request_url), move(headers)} );
        current_request = move(r);
    };

    void on_error(int http_errno, const char* msg)
    {
        cerr << "Error: " << http_errno << " " << msg << endl;
    }
    void on_message_complete()
    {
        cerr << "Message complete" << endl;
    }
    void on_message_begin()
    {
        cerr << "Message begin" << endl;
    }
    void on_header_line( const std::string& name, string&& value )
    {
        headers[name] = move(value);
    }

    void reset() {
        current_request = http_request_header_ptr();
        headers.clear();
        buffered_body_consumed = 0;
        reset_on_body();
        base_t::reset();
    }
    void reset_on_body() {
        on_body = [this]( const char* p, size_t l) {
                buffered_body.append(p,l);
        };
    }


    void 
    async_read_request_header( read_header_completion_t  comp ) {
        if ( current_request ) {
            stream.get_io_service().post([this,comp]{
                boost::system::error_code ec;
                comp( ec, move(current_request));
            });
        } else {
            stream.async_read_some(boost::asio::buffer(buffer),
                    [this,comp](boost::system::error_code ec, std::size_t bytes_transferred)
            {
                if (!ec)
                {
                    size_t read = this->parse( buffer.data(), bytes_transferred );
                    assert( read == bytes_transferred );
                    if ( current_request ) {
                        boost::system::error_code ec;
                        comp( ec, move(current_request));
                    }
                    else {
                        async_read_request_header(comp);
                    }  
                } else {
                    comp( ec, nullptr );
                }
            } );
        } 
    }

    void async_read_some( boost::asio::mutable_buffers_1 buff, read_body_completion_t comp ){
        if ( size_t consumed = consume_buffered_body(buff)) {
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
                    size_t bytes_emitted = 0;
                    on_body = [this,&buff,&bytes_emitted ]( const char* p, size_t l) {
                            char* output = buffer_cast<char*>(buff);
                            memcpy( output, p, l );
                            bytes_emitted = l;
                    };   
                    size_t read = this->parse( buffer.data(), bytes_transferred );
                    assert( read == bytes_transferred );
                    comp( ec, bytes_emitted );
                } else {
                    comp( ec, 0 );
                }
            });
        }
    }

    size_t consume_buffered_body(boost::asio::mutable_buffers_1 buff) {
        if ( buffered_body.size() > buffered_body_consumed ) {
            size_t minlen = min( buffer_size(buff), buffered_body.size() - buffered_body_consumed);
            char* output = buffer_cast<char*>(buff);
            memcpy( output, buffered_body.data() + buffered_body_consumed, minlen  );
            buffered_body_consumed += minlen;
            return minlen;
        }
        else {
            return 0;
        }

    }

    
    function< void( const char*, size_t ) >    on_body;
};


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





}



#include "detail/http_parser_impl.hxx"