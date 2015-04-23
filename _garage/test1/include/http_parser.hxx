#include <memory>

namespace mxmz {

template <class Observer >
class http_parser_base {
  class detail;
  std::unique_ptr<detail> i;

 public:
  enum mode_t { Request, Response, Both };
  http_parser_base(mode_t, Observer& );

  void reset();
  size_t parse(const char* buffer, size_t len);
};
}
