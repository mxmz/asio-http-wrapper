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
protected:
    http_parser_base(mode_t mode );
public:
    void pause();
    void unpause();

    void reset();
    size_t parse(const char* buffer, size_t len);
};
}
