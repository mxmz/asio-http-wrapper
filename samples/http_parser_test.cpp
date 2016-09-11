#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <string>
#include <map>
#include <functional>
#include <memory>
#include <cassert>

#include "http_parser.hxx"
#include "boost/lexical_cast.hpp"

using namespace std;


template< typename Type >
Type getenv(const char* name ) {
    const char * val = getenv(name);
    if ( val == nullptr ) {
        return Type();
    } else {
        return boost::lexical_cast<Type>(val);
    }
}

bool verbose = getenv<bool>("VERBOSE");

#define cerr if(verbose) cerr 

class handlers_interface {
public:
    virtual void on_body(const char* b, size_t l) = 0;
    virtual void on_response_headers_complete( int code, const string& response_status ) = 0;
    virtual void on_request_headers_complete( const string& method, const string& request_url ) = 0;
    virtual void on_error(int http_errno, const char* msg) = 0;
    virtual void on_message_complete() = 0;
    virtual void on_message_begin() = 0;
    virtual void on_header_line( const std::string& name, string&& value ) = 0;

};

template < class Derived >
struct base_handlers : public handlers_interface {

    map< string, string > headers;
    string body;
    
    pair<int,std::string> error;


    void  on_body(const char* b, size_t l) {
        cerr << "body data: " << l << " ";
        cerr.write(b, l) << endl;
        body.append(b,l);
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
        error = make_pair(http_errno,string(msg));
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



struct  my_handlers : public base_handlers<my_handlers> {
    int code;
    string  response_status;
    string request_url;
    string method;
    map< string, string > headers;

    void on_response_headers_complete( int _code, const string& _response_status, map<string,string>&& _headers ) {
        cerr << "res" << endl;
        headers = move(_headers);
    }

    void  on_request_headers_complete( const string& _method, const string& _request_url, map<string,string>&& _headers ) {
        cerr << "req" << endl;
        headers = move(_headers);
        method = _method;
        request_url = _request_url;

    };

    void on_message_complete() override
    {
        cerr << "Message complete (derived)" << endl;
    }


};

typedef mxmz::http_parser_base<handlers_interface> my_parser; 

void readparse( const string&s, my_parser& parser ) {
    float bufsize = 1;
    const char* p = s.data();
    const char* end = p + s.size();
    while( p != end ) {
          long int len = size_t(bufsize); bufsize *= 1.266;
          len = min( len , (end-p) );
          cerr << "readparse: len " << len << endl;
          parser.parse( p, len );
          p += len;
    }
}


void test1()
{
    cout << __FUNCTION__ << " ..." << endl; 

    my_handlers handlers;
    
    my_parser  parser(my_parser::Request, handlers);

    const string s1 = "POST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"
                      "Accept: */*\r\n"
                      "Transfer-Encoding: identity\r\n"
                      "Content-Length: 5\r\n"
                      "\r\n"
                      "World";

    readparse(s1,parser);

    dump(handlers.headers);

    assert( handlers.headers["Accept"] == "*/*" );
    assert( handlers.headers["Content-Length"] == "5" );
    assert( handlers.headers["Transfer-Encoding"] == "identity" );
    assert( handlers.method == "POST" );
    assert( handlers.request_url == "/post_identity_body_world?q=search#hey" );
    assert( handlers.body == "World" );



    cout << __FUNCTION__ << " ok" << endl;

}



void test2()
{
    cout << __FUNCTION__ << " ..." << endl; 

    my_handlers handlers;
    typedef mxmz::http_parser_base<handlers_interface> my_parser; 
    

    const string s1 = "POST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"
                      "Accept:";
                      
    const string s2 = " */*\r\n"
                      "Transfer-Encoding: identity\r\n"
                      "Content-Len";

    const string s3 = "gth: 5\r\n"
                      "\r\n"
                      "World";

    string chunks[] = { s1, s2 , s3 };

    typedef std::unique_ptr<my_parser> my_parser_ptr;

    my_parser_ptr parser( new my_parser(my_parser::Request, handlers) );

    for( auto& s : chunks ) {
        parser = move( my_parser_ptr(new my_parser(parser->move_state(), handlers))  );
        readparse(s,*parser);
    }
    

    dump(handlers.headers);

    assert( handlers.headers["Accept"] == "*/*" );
    assert( handlers.headers["Content-Length"] == "5" );
    assert( handlers.headers["Transfer-Encoding"] == "identity" );
    assert( handlers.method == "POST" );
    assert( handlers.request_url == "/post_identity_body_world?q=search#hey" );



    cout << __FUNCTION__ << " ok" << endl;

}

void test3()
{
    cout << __FUNCTION__ << " ..." << endl; 

    my_handlers handlers;
    typedef mxmz::http_parser_base<handlers_interface> my_parser; 
    my_parser  parser(my_parser::Request, handlers);

    const string s1 = "POST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"
                      "Acc ept: */*\r\n"
                      "Transfer-Encoding: identity\r\n"
                      "Content-Length: 5\r\n"
                      "\r\n"
                      "World";


    readparse(s1,parser);

    assert( handlers.error.first ==  24) ;
    assert( handlers.error.second ==  "invalid character in header" ) ;

    cout << __FUNCTION__ << " ok" << endl;

}
void test4()
{
    cout << __FUNCTION__ << " ..." << endl; 

    my_handlers handlers;
    typedef mxmz::http_parser_base<handlers_interface> my_parser; 
    my_parser  parser(my_parser::Request, handlers);

    const string s1 = "PO ST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"
                      "Accept: */*\r\n"
                      "Transfer-Encoding: identity\r\n"
                      "Content-Length: 5\r\n"
                      "\r\n"
                      "World";


    readparse(s1,parser);

    assert( handlers.error.first ==  16) ;
    assert( handlers.error.second ==  "invalid HTTP method" ) ;

    cout << __FUNCTION__ << " ok" << endl;

}
void test5()
{
    cout << __FUNCTION__ << " ..." << endl; 

    my_handlers handlers;
    typedef mxmz::http_parser_base<handlers_interface> my_parser; 
    my_parser  parser(my_parser::Request, handlers);

    const string s1 = "POST _/post_identity_body_world?q=search#hey HTTP/1.1\r\n"
                      "Accept: */*\r\n"
                      "Transfer-Encoding: identity\r\n"
                      "Content-Length: 5\r\n"
                      "\r\n"
                      "World";


    readparse(s1,parser);

    assert( handlers.error.first ==  17) ;
    assert( handlers.error.second ==  "invalid URL" ) ;

    cout << __FUNCTION__ << " ok" << endl;

}



int main() {
    test1();
    test2();
    test3();
    test4();
    test5();
}

#include "detail/http_parser_impl.hxx"

