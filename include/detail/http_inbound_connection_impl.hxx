#include "http_inbound_connection.hxx"
#include <iostream>
#include <memory>
#include "http_parser.hxx"
#include <cassert>
#include <cstring>

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

 struct request_state {
        map< string, string >   headers;
        
        http_request_header_ptr  header_ready;

        string                  body_consumable;
        size_t                  body_consumed;
        
        request_state() : body_consumed() {}
     
        size_t consume_body(boost::asio::mutable_buffers_1 buff) {
            if ( body_consumable.size() > body_consumed ) {
                size_t minlen = min( buffer_size(buff), body_consumable.size() - body_consumed);
                char* output = buffer_cast<char*>(buff);
                memcpy( output, body_consumable.data() + body_consumed, minlen  );
                body_consumed += minlen;
                return minlen;
            }
            else {
                return 0;
            }
        }
    }; 

 typedef unique_ptr<request_state>  request_state_ptr; 
 
 request_state_ptr current;
 

 std::array<char, 8192>  buffer;
 mutable_buffers_1       body_sink;
 

 

public:
    detail( detail& ) = delete;
    detail( SrcStream&& s ) :
        base_t( base_t::Request ),
        stream( move(s)),
        body_sink((char*)"",0)
    {
               current = request_state_ptr( new request_state() );   
    }




    void on_response_headers_complete( int code, const string& response_status ) {
    }

    void  on_request_headers_complete( string&& method, const string&& request_url ) {
        http_request_header_ptr r( new http_request_header{move(method), move(request_url), move(current->headers)} );
        current->header_ready = move(r);
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
        current->headers[name] = move(value);
    }

    void reset() {
        current = request_state_ptr( new request_state() );   
        base_t::reset();
    }

    


    void 
    async_read_request_header( read_header_completion_t  comp ) {
        if ( current->header_ready ) {
            stream.get_io_service().post( [c = move(comp), r = move(current->header_ready)](){
                boost::system::error_code ec;
               //c(ec, move(r));
            });
        } else {
            stream.async_read_some(boost::asio::buffer(buffer),
                    [this,c = move(comp)](boost::system::error_code ec, std::size_t bytes_transferred)
            {
                if (!ec)
                {
                    size_t read = this->parse( buffer.data(), bytes_transferred );
                    assert( read == bytes_transferred );
                    async_read_request_header(move(c));
                } else {
                    c( ec, nullptr );
                }
            } );
        } 
    }

    void on_body( const char* p, size_t l) {
            if( size_t bslen = buffer_size( body_sink ) ) {
                    size_t sinklen = min( l, bslen );
                    char*  bsp = buffer_cast<char*>( body_sink ); 
                    memcpy( bsp, p, sinklen );
                    body_sink = mutable_buffers_1( bsp + sinklen, bslen - sinklen );
                    p += sinklen;
                    l -= sinklen;
            }
            current->body_consumable.append(p,l);
    }

    void async_read_some( mutable_buffers_1 buff, read_body_completion_t comp ){
        if ( size_t consumed = current->consume_body(buff)) {
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
                    body_sink = buff;      
                    size_t read = this->parse( buffer.data(), bytes_transferred );
                    assert( read == bytes_transferred );
                    size_t bytes_emitted = buffer_size(buff) - buffer_size(body_sink);
                    body_sink = mutable_buffers_1((char*)"",0);
                    comp( ec, bytes_emitted );
                } else {
                    comp( ec, 0 );
                }
            });
        }
    }

    
    
    
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