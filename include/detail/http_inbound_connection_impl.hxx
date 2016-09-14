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
using boost::asio::mutable_buffer;
using boost::asio::const_buffers_1;;

template< class SrcStream >
class http_inbound_connection<SrcStream>::detail : 
    public  mxmz::http_parser_base<http_inbound_connection<SrcStream>::detail >
{
    typedef mxmz::http_parser_base<http_inbound_connection<SrcStream>::detail > base_t;
    

 SrcStream  stream;

 struct request_state {
        map< string, string >   headers;
        
        http_request_header_ptr  header_ready;

        string                  body;
        size_t                  body_consumed;
        
        request_state() : body_consumed() {}
     
        size_t consume_body(boost::asio::mutable_buffers_1 buff) {
            size_t consumed = buffer_copy( buff, const_buffer(body.data(), body.size()) + body_consumed  );
            body_consumed += consumed;
            cerr << "consume stored: " << consumed << endl;
            return consumed;
        }
    }; 

 typedef unique_ptr<request_state>  request_state_ptr; 
 
 request_state_ptr current;
 
 typedef std::function< size_t(const char*, size_t )> body_consumer_t;

 std::array<char, 8192>  buffer;

body_consumer_t   body_consumer;

 

public:
    detail( detail& ) = delete;
    detail( SrcStream&& s ) :
        base_t( base_t::Request ),
        stream( move(s))
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
            stream.get_io_service().post( [this,comp](){
                boost::system::error_code ec;
               comp(ec, move(current->header_ready));
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

            size_t consumed = body_consumer ? body_consumer( p, l  ) : 0 ;
            if ( consumed < l ) {
                    current->body.append(p + consumed, l - consumed );
            }
            body_consumer = body_consumer_t();
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
                    size_t consumed = 0;
                    body_consumer = [this,&buff,&consumed]( const char* p, size_t l)->size_t {
                            consumed = buffer_copy( buff, const_buffers_1(p,l) );
                            cerr << "consume direct: " << consumed << endl;
                            return consumed;
                    };
                   
                    size_t read = this->parse( buffer.data(), bytes_transferred );
                    assert( read == bytes_transferred );
                    
                    comp( ec, consumed );
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