#include <memory>

namespace mxmz {

template <class Handlers>
class http_parser_base {
    class detail;
    std::unique_ptr<detail> i;

public:
    enum mode_t { Request,
                  Response,
                  Both
                };
    http_parser_base(mode_t, Handlers&);
    http_parser_base(mode_t mode ) : http_parser_base( mode, static_cast<Handlers&>(*this) ) { }
    void pause();
    void unpause();

    void reset();
    size_t parse(const char* buffer, size_t len);
};
}
