#include <iostream>
#include <chrono>
#include <queue>
#include <functional>
#include <string>
#include <map>
#include <functional>

#include "http_parser.hxx"



using namespace std;


class std_http_parser: mxmz::http_parser_base<std_http_parser> {


};

int main() {

        std_http_parser parser;



}

#include "detail/http_parser_impl.hxx"


/*
typedef std::map< std::string, std::string> map_t;
typedef std::map< std::string,  std::function<void( const std::string& )>  > setter_map_t;

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
