#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <string>
#include <map>
#include <functional>

#include "http_parser.hxx"

using namespace std;

class std_http_parser : public mxmz::http_parser_base<std_http_parser> {
public:

    typedef map< string, string > headers_t;

    explicit std_http_parser(mode_t mode)
        : mxmz::http_parser_base<std_http_parser>(mode)
    {
        on_body = [](const char* b, size_t l) {
            cerr << "body data: " << l << " ";
            cerr.write(b, l) << endl;
        };
        on_request_headers_complete = []( const string& method, const string& request_url ) {
            cerr << "req" << endl;
        };
        on_response_headers_complete = []( int code, const string& response_status ) {
            cerr << "res" << endl;
        };
    }

    typedef function< void( const char*, size_t) > on_body_handler;
    typedef function< void( const string& method, const string& req_url  ) > on_request_headers_complete_handler;
    typedef function< void( int status_code, string&& res_staus  ) > on_response_headers_complete_handler;

    on_body_handler on_body;
    on_request_headers_complete_handler   on_request_headers_complete;
    on_response_headers_complete_handler  on_response_headers_complete;

    /*
     void on_body(const char* b, size_t len) { cerr.write(b, len) << endl; }
    */


    void on_error(int http_errno, const char* msg)
    {
        cerr << "Error: " << http_errno << " " << msg << endl;
    }
    void on_message_end()
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
    }
};

class handlers_interface {
public:
    virtual void on_body(const char* b, size_t l) = 0;
    virtual void on_response_headers_complete( int code, const string& response_status ) = 0;
    virtual void on_request_headers_complete( const string& method, const string& request_url ) = 0;
    virtual void on_error(int http_errno, const char* msg) = 0;
    virtual void on_message_end() = 0;
    virtual void on_message_begin() = 0;
    virtual void on_header_line( const std::string& name, string&& value ) = 0;

};

class simple_handlers : public handlers_interface {
public:

    void  on_body(const char* b, size_t l) {
        cerr << "body data: " << l << " ";
        cerr.write(b, l) << endl;
    }

    void on_response_headers_complete( int code, const string& response_status ) {
        cerr << "res" << endl;
    }

    void  on_request_headers_complete( const string& method, const string& request_url ) {
        cerr << "req" << endl;
    };

    void on_error(int http_errno, const char* msg)
    {
        cerr << "Error: " << http_errno << " " << msg << endl;
    }
    void on_message_end()
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
    }
};




template< class Map >
void dump( const Map& m ) {
    for ( auto& e : m ) {
        cerr << e.first << " : " << e.second << endl;
    }
}

int main()
{
    simple_handlers handlers;
    mxmz::http_parser_base<handlers_interface> p0(mxmz::http_parser_base<handlers_interface>::Request, handlers);


    std_http_parser parser(std_http_parser::Request);

    const string s1 = "POST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"
                      "Accept: */*\r\n"
                      "Transfer-Encoding: identity\r\n"
                      "Content-Length: 5\r\n"
                      "\r\n"
                      "World";

    parser.on_request_headers_complete = [&]( const string& m, const string& url ) {
        cerr << "req headers: " << m << " " << url << endl;
//            parser.pause();
    };


    for( auto& c : s1 ) {
        parser.parse(&c, 1 );
        p0.parse(&c, 1 );
    }

    /*
    auto rc1 = parser.parse(s1.data(), s1.size());

    cerr << rc1 <<  " " << s1.size() << endl;

    parser.unpause();
    auto rc2 = parser.parse(s1.data() + rc1, s1.size() - rc1 );

    cerr << rc2 <<  " " << s1.size() - rc1 << endl;
    */

    cerr << "---------------------------------------------------------------" << endl;

    const string s2 = "HTTP/1.1 200 OK\r\n"
                      "Date: Tue, 04 Aug 2009 07:59:32 GMT\r\n"
                      "Server: Apache\r\n"
                      "X-aa:\r\n"
                      "X-Powered-By: Servlet/2.5 JSP/2.1\r\n"
                      "Content-Type: text/xml; charset=utf-8\r\n"
                      "Content-Length: 311\r\n"
                      "Connection: keepalive\r\n"
                      "\r\n"
                      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                      "<SOAP-ENV:Envelope "
                      "xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\">\n"
                      "  <SOAP-ENV:Body>\n"
                      "    <SOAP-ENV:Fault>\n"
                      "       <faultcode>SOAP-ENV:Client</faultcode>\n"
                      "       <faultstring>Client Error</faultstring>\n"
                      "    </SOAP-ENV:Fault>\n"
                      "  </SOAP-ENV:Body>\n"
                      "</SOAP-ENV:Envelope>";


    std_http_parser parser2(std_http_parser::Response);

    parser2.on_response_headers_complete = [&]( int code, const string& response_status ) {
        cerr << "response status " << code << " " << response_status << endl;
        parser2.pause();
    };

    cerr << "parse(1) " << parser2.parse(s2.data(), 198 ) << " " << s2.size() << endl;
    cerr << "--------------" << endl;;
    cerr.write((const char*)s2.data(), 198 ) << "|||" << endl;
    cerr << "--------------" << endl;;

    parser2.unpause();
    cerr << "parse(2)" << parser2.parse(s2.data() + 196, 10 ) << endl;
    cerr << "parse(2)" << parser2.parse("012345", 6 ) << endl;

    /*
    parser2.unpause();
    cerr << "parse(2)" << parser2.parse(s2.data(), s2.size()) << endl;
    */
}

#include "detail/http_parser_impl.hxx"

/*
typedef std::map< std::string, std::string> map_t;
typedef std::map< std::string,  std::function<void( const std::string& )>  >
setter_map_t;

struct base_box {

    setter_map_t  setters;

    void set( const string& k, const string& v ) {
            auto& f = setters[k];
            if ( f ) f(v);
    }
};

struct base_boxed {
   std::string v;
   std::function<void( const std::string& )> get_setter() {
            return [this]( const string& v ) {
                    this->v = v;
            };
    }
};

struct A  : public base_boxed {
    static constexpr const char* name =  "A";
    string get_A() { return  v; }
};

struct B : public base_boxed  {
    static constexpr const char* name = "B";
    string get_B() { return v; }
};

template< class Boxed, class Base >
struct box: public Base, public Boxed {
    box()
    {
            this->setters[ Boxed::name ] = Boxed::get_setter();
    }
};

int main() {


       box<A, box< B, base_box> > a;

       a.set("A", "allo");
       a.set("B", "bllo");
       a.set("C", "bllo");

       cerr << a.get_A() << endl;
       cerr << a.get_B() << endl;


}

*/
