#ifndef http_parser_29834729834729834729384729482741762537165371635176235173651723615723
#define http_parser_29834729834729834729384729482741762537165371635176235173651723615723

#include <memory>


namespace mxmz {

class http_parser_state;

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
    http_parser_base(http_parser_state&&, Handlers&);
protected:
    http_parser_base(mode_t mode );
    http_parser_base(http_parser_state&&);
public:
    void pause();
    void unpause();

    void reset();
    size_t parse(const char* buffer, size_t len);

    http_parser_state&& move_state();
};
}

#endif