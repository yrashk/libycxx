// libycxx hosted runtime: future_category() ([futures.errors]).
#include <future>

namespace {

class future_error_category final : public std::error_category {
public:
  constexpr future_error_category() noexcept {}
  const char* name() const noexcept override { return "future"; }
  std::string message(int __ev) const override {
    switch (static_cast<std::future_errc>(__ev)) {
    case std::future_errc::broken_promise:
      return "the promise was destroyed before it provided a result (broken promise)";
    case std::future_errc::future_already_retrieved:
      return "the future has already been retrieved";
    case std::future_errc::promise_already_satisfied:
      return "the promise has already been satisfied";
    case std::future_errc::no_state:
      return "no associated state";
    }
    return "unknown future error";
  }
};

// Constant-initialized and never destroyed, like the other category objects
// (src/hosted/system_error.cpp).
union immortal {
  future_error_category __object;
  constexpr immortal() noexcept : __object() {}
  ~immortal() {}
};
constinit immortal future_object;

} // namespace

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
const error_category& future_category() noexcept { return future_object.__object; }
}} // namespace std
