// libycxx hosted runtime: future_category() ([futures.errors]).
#include <future>

namespace {

class future_error_category final : public std::error_category {
public:
  constexpr future_error_category() noexcept {}
  const char* name() const noexcept override { return "future"; }
  std::string message(int ev) const override {
    switch (static_cast<std::future_errc>(ev)) {
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
  future_error_category object;
  constexpr immortal() noexcept : object() {}
  ~immortal() {}
};
constinit immortal future_object;

} // namespace

namespace [[gnu::visibility("hidden")]] std {
const error_category& future_category() noexcept { return future_object.object; }
} // namespace std
