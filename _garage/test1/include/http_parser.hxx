#include <memory>

namespace mxmz {

template < class Derived >
class http_parser_base  {

    class detail;
    std::unique_ptr<detail>   i_;

        public:
    
    http_parser_base();



};

}
