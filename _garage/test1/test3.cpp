#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <string>
#include <map>
#include <functional>

#include "http_parser.hxx"

using namespace std;


template < class Derived >
class base_handlers : public  mxmz::http_parser_base<Derived> {

    map< string, string > headers;

public:
    base_handlers() : mxmz::http_parser_base<Derived>( mxmz::http_parser_base<Derived>::Request) {}

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



class my_handlers : public base_handlers<my_handlers> {
public:

    using base_handlers<my_handlers>::on_request_headers_complete;
    using base_handlers<my_handlers>::on_response_headers_complete;

    void on_response_headers_complete( int code, const string& response_status, map<string,string>&& handlers ) {
        cerr << "res" << endl;
        dump(handlers);
    }

    void  on_request_headers_complete( const string& method, const string& request_url, map<string,string>&& handlers ) {
        cerr << "req" << endl;
        dump(handlers);
    };

    void on_message_complete()
    {
//        pause();
        cerr << "Message complete (derived)" << endl;
    }


};



int main()
{
    my_handlers parser;

    const string s1 = "POST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"
                      "Accept: */*\r\n"
                      "Transfer-Encoding: identity\r\n"
                      "Content-Length: 5\r\n"
                      "\r\n"
                      "World";


    for( auto& c : s1 ) {
        parser.parse(&c, 1 );
    }

}

#include "detail/http_parser_impl.hxx"

