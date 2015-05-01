#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <string>
#include <map>
#include <functional>
#include <memory>

#include <boost/asio.hpp>

#include "http_parser.hxx"

using namespace std;
using namespace boost::asio;


template < class Derived >
class base_handlers : public  mxmz::http_parser_base<Derived> {

    map< string, string > headers;

public:
    base_handlers() :
        mxmz::http_parser_base<Derived>( mxmz::http_parser_base<Derived>::Request)
    {}

    void  on_body(const char* b, size_t l) {
        cerr << "body data: " << l << " ";
        cerr.write(b, l) << endl;
    }

    void on_response_headers_complete( int code, const string& response_status ) {
        cerr << "res" << endl;
        static_cast<Derived&>(*this).on_response_headers_complete(code,response_status, move(headers) );
    }

    void  on_request_headers_complete( const string& method, const string& request_url ) {
        cerr << "req" << endl;
        static_cast<Derived&>(*this).on_request_headers_complete(method,request_url, move(headers) );
    };

    void on_error(int http_errno, const char* msg)
    {
        cerr << "Error: " << http_errno << " " << msg << endl;
    }
    void on_message_complete()
    {
//        pause();
        cerr << "Message complete" << endl;
    }
    void on_message_begin()
    {
//        pause();
        cerr << "Message begin" << endl;
    }
    void on_header_line( const std::string& name, string&& value )
    {
        cerr << name << "  = " << value << endl;
        headers[name] = move(value);
    }
};

template< class Map >
void dump( const Map& m ) {
    for ( auto& e : m ) {
        cerr << e.first << " : " << e.second << endl;
    }
}




/* --------------------------------------------- --------------------------------------------- --------------------------------------------- */


struct request {
    string method;
    string url;
    map<string,string> headers;
    string body;
    bool ready;

    request( string method, string url, map<string,string>&& headers ) :
        method(move(method) ),
        url( move(url) ),
        headers( move(headers) ),
        ready(false)
    {
    }
};

class http_connection : public base_handlers<http_connection>,  public std::enable_shared_from_this<http_connection> {
    ip::tcp::socket socket;
    size_t          bytes_in_body;
    unique_ptr<request>     current_request;
public:
    http_connection( ip::tcp::socket&& s ) :
        socket( move(s) ), bytes_in_body(0)
    {}

    using base_handlers<http_connection>::on_request_headers_complete;
    using base_handlers<http_connection>::on_response_headers_complete;

    void on_response_headers_complete( int code, const string& response_status, map<string,string>&& headers ) {
        cerr << "res" << endl;
        dump(headers);
    }

    void  on_request_headers_complete( const string& method, const string& request_url, map<string,string>&& headers ) {
        cerr << "req" << endl;
        current_request.reset(  new request( method, request_url, move(headers) ) );
        dump(headers);
    };

    void on_message_complete()
    {
//        pause();
        cerr << "Message complete (derived)" << endl;
        current_request->ready = true;
    }

    pair<const char*, size_t> last_body_chunk;

    void  on_body(const char* b, size_t l) {
        cerr << "body data: " << l << " ";
        //cerr.write(b, l) << endl;
        current_request->body.append( b, l );
        last_body_chunk = move(make_pair(b,l));
    }

    std::array<char, 8192> buffer;
    string response_buffer;

    void start_read() {
        auto self(shared_from_this());

        socket.async_read_some(boost::asio::buffer(buffer),
                               [this,self](boost::system::error_code ec, std::size_t bytes_transferred)
        {
            if (!ec)
            {
                size_t read = parse( buffer.data(), bytes_transferred );
                cerr << (void*) buffer.data() << " " << bytes_transferred << endl;
                cerr << (void*)  last_body_chunk.first << " " << last_body_chunk.second << endl;
                cerr << (char*)  last_body_chunk.first - (char*) buffer.data() << endl;

                if ( read != bytes_transferred ) {
                    cerr << "Error" << endl;
                }
                if ( current_request ) {
                    if ( current_request->ready ) {
                        cerr << "request and body ready" << endl;

                        response_buffer =   "HTTP/1.0 200 OK\r\n"
                                            "Content-Length: 4\r\n"
                                            "Content-Type: text/plain\r\n"
                                            "\r\n"
                                            "OK\r\n";
                        async_write(  socket, boost::asio::buffer(response_buffer),
                        [this,self](boost::system::error_code ec, std::size_t) {
                            boost::system::error_code ignored_ec;
                            socket.shutdown(ip::tcp::socket::shutdown_both, ignored_ec);
                        } );

                    } else {
                        cerr << "body not ready yet" << endl;
                        start_read();
                    }

                } else {
                    cerr << "request not ready yet" << endl;
                    start_read();
                }
            }


        }
                              );

    }

};



int main()
{

    io_service  ios;

    ip::tcp::resolver resolver(ios);
    ip::tcp::endpoint endpoint = *resolver.resolve({"localhost", "2080"});

    ip::tcp::acceptor acceptor(ios);

    ip::tcp::socket socket(ios);


    acceptor.open(endpoint.protocol());
    acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen();

    function<void()> start_accept = [&acceptor,&socket,&start_accept]() {
        acceptor.async_accept( socket, [&acceptor,&socket,&start_accept](boost::system::error_code ec)
        {
            if (!acceptor.is_open())
            {
                return;
            }

            if (!ec)
            {
                cerr << "new connnection" << endl;
                auto conn = make_shared<http_connection>( move(socket));
                conn->start_read();
            }
            start_accept();
        } );
    };

    start_accept();
    ios.run();
}

#include "detail/http_parser_impl.hxx"

