#include <memory>

namespace mxmz {

    template < class Derived >
    class http_parser_base  {

        class detail;
        std::unique_ptr<detail>   i;
        public: 
           
        http_parser_base();

	void reset();
        size_t parse( const char* buffer, size_t len );


    };























}
