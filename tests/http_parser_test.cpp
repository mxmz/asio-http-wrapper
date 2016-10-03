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

#include "test.hxx"
using namespace mxmztest;

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
        CERR << "body data: " << l << " ";
        CERR.write(b, l) << endl;
        body.append(b,l);
    }

    void on_response_headers_complete( int code, const string& response_status ) {
        CERR << "res" << endl;
        static_cast<Derived&>(*this).on_response_headers_complete(code,response_status, move(headers) );
    }

    void  on_request_headers_complete( const string& method, const string& request_url ) {
        CERR << "req" << endl;
        static_cast<Derived&>(*this).on_request_headers_complete(method,request_url, move(headers) );
    };

    void on_error(int http_errno, const char* msg)
    {
        CERR << "Error: " << http_errno << " " << msg << endl;
        error = make_pair(http_errno,string(msg));
    }
    void on_message_complete()
    {
        CERR << "Message complete" << endl;
    }
    void on_message_begin()
    {
        CERR << "Message begin" << endl;
    }
    void on_header_line( const std::string& name, string&& value )
    {
        CERR << name << "  = " << value << endl;
        headers[name] = move(value);
    }
};

template< class Map >
void dump( const Map& m ) {
    for ( auto& e : m ) {
        CERR << e.first << " : " << e.second << endl;
    }
}



struct  my_handlers : public base_handlers<my_handlers> {
    int code;
    string  response_status;
    string request_url;
    string method;
    map< string, string > headers;

    using base_handlers<my_handlers>::on_response_headers_complete;
    void on_response_headers_complete( int _code, const string& _response_status, map<string,string>&& _headers ) {
        CERR << "res" << endl;
        headers = move(_headers);
    }

    using base_handlers<my_handlers>::on_request_headers_complete;
    void  on_request_headers_complete( const string& _method, const string& _request_url, map<string,string>&& _headers ) {
        CERR << "req" << endl;
        headers = move(_headers);
        method = _method;
        request_url = _request_url;

    };

    void on_message_complete() override
    {
        CERR << "Message complete (derived)" << endl;
    }
};


 
template< class Parser >
void readparse( const string&s, Parser& parser ) {
    float bufsize = 1;
    const char* p = s.data();
    const char* end = p + s.size();
    while( p != end ) {
          long int len = size_t(bufsize); bufsize *= 1.266; bufsize += rand_int() % 10 ;
          len = min( len , (end-p) );
          CERR << "readparse: len " << len << endl;
          parser.parse( p, len );
          p += len;
    }
}


void test1()
{
 

    typedef mxmz::http_parser_base<handlers_interface> my_parser;
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





}



void test2()
{
 

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
        auto current = move(parser);
        parser = move( my_parser_ptr(new my_parser(current->move_state(), handlers))  );
        readparse(s,*parser);
    }
    

    dump(handlers.headers);

    assert( handlers.headers["Accept"] == "*/*" );
    assert( handlers.headers["Content-Length"] == "5" );
    assert( handlers.headers["Transfer-Encoding"] == "identity" );
    assert( handlers.method == "POST" );
    assert( handlers.request_url == "/post_identity_body_world?q=search#hey" );





}

void test3()
{
 

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



}
void test4()
{
 

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



}
void test5()
{
 

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



}

struct  my_parser_pausing :   public base_handlers<my_parser_pausing>,
                                public mxmz::http_parser_base<my_parser_pausing>
{
    int code;
    string  response_status;
    string request_url;
    string method;
    map< string, string > headers;
    string body;

    using base_handlers<my_parser_pausing>::on_response_headers_complete;
    void on_response_headers_complete( int _code, const string& _response_status, map<string,string>&& _headers ) {
        CERR << "res" << endl;
        headers = move(_headers);
    }

    using base_handlers<my_parser_pausing>::on_request_headers_complete;
    void  on_request_headers_complete( const string& _method, const string& _request_url, map<string,string>&& _headers ) {
        CERR << __PRETTY_FUNCTION__ << endl;
        CERR << "req" << endl;
        headers = move(_headers);
        method = _method;
        request_url = _request_url;

        this->pause();  // <--------------- pausing when headers are ready; cfr.  test_readparse_pausing and test6

    };

    void on_message_complete() override
    {
        CERR << "Message complete (derived)" << endl;
    }
    void on_message_begin() {
      CERR << __PRETTY_FUNCTION__ << endl;
      //pause();
    }
    void  on_body(const char* b, size_t l) {
        CERR << __PRETTY_FUNCTION__ << " " << l << endl;
        body.append(b,l);
        //pause();
       
    }

    using mxmz::http_parser_base<my_parser_pausing>::http_parser_base;
};



bool test_readparse_pausing( const string&s, my_parser_pausing& parser ) {
    bool did_pause = false;
    float bufsize = 1;
    const char* p = s.data();
    const char* end = p + s.size();
    size_t eoh = s.find("\r\n\r\n") + 4;
    size_t parsed = 0;
    while( p != end ) {
          long int len = size_t(bufsize); bufsize *= 1.666;
          len = min( len , (end-p) );
          CERR << "readparse: len " << len <<  " " << parser.paused() << endl;
          long int consumed = parser.parse( p, len );
          CERR << "readparse: consumed " << consumed <<  " " << parser.paused() << endl;
          if ( consumed < len ) {
              assert ( parser.paused() );
              assert( parsed + consumed == eoh - 1 ) ;
              parser.unpause();
              consumed += parser.parse( p + consumed, len - consumed );
              assert( consumed == len );
              did_pause = true;
          }
          parsed += consumed;
          p += len;
    }
    CERR << parsed << "   " << s.size() << endl;
    assert( parsed == s.size() );
    return did_pause;
}
using namespace mxmztest;

void test6()
{      
    my_parser_pausing  parser(my_parser_pausing::Request);

    auto b1 = make_random_string( 1042 + rand_int() % 1042, 0, 256 );
    auto b2 = make_random_string( 1042 + rand_int() % 42, 0, 256  );

    string bodylen = boost::lexical_cast<string>( b1.size() + b2.size() );

    auto rand_head_name =  make_random_string(142 + rand_int() % 142, 'a', 'z' );
    auto rand_head_value = make_random_string(142 + rand_int() % 142, 'a', 127 );

    const string s1 = "POST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"
                      "Accept: */*\r\n"
                      "Transfer-Encoding: identity\r\n"
                      "Content-Length: " + bodylen + "\r\n"
                      + rand_head_name + ": " + rand_head_value + "\r\n"  
                      "\r\n"
                      + b1;

    const string s2 =  b2 ;         

    assert( test_readparse_pausing( s1, parser ) == true );
    assert( test_readparse_pausing( s2, parser ) == false );


    assert( parser.body == b1+b2);
    assert( parser.headers[rand_head_name] == rand_head_value );


}



int main() {
    srand ( time(nullptr));
    RUN( test1 , 5000 );
    RUN( test2 , 5000 );
    RUN( test3 , 5000 );
    RUN( test4 , 5000 );
    RUN( test5 , 5000 );
    RUN( test6 , 5000 );
    //cout << mxmztest::verbose << endl;
}

#include "detail/http_parser_impl.hxx"

