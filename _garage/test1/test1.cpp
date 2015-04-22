#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <string>
#include <map>
#include <functional>

#include "http_parser.hxx"

using namespace std;

class std_http_parser : public mxmz::http_parser_base<std_http_parser> {};

int main() {
  std_http_parser parser;

  const string s1 =
      "POST /post_identity_body_world?q=search#hey HTTP/1.1\r\n"
      "Accept: */*\r\n"
      "Transfer-Encoding: identity\r\n"
      "Content-Length: 5\r\n"
      "\r\n"
      "World";

  parser.parse(s1.data(), s1.size());

  const string s2 =
      "HTTP/1.1 200 OK\r\n"
      "Date: Tue, 04 Aug 2009 07:59:32 GMT\r\n"
      "Server: Apache\r\n"
      "X-Powered-By: Servlet/2.5 JSP/2.1\r\n"
      "Content-Type: text/xml; charset=utf-8\r\n"
      "Connection: close\r\n"
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

  parser.reset();

  parser.parse(s2.data(), s2.size());
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
