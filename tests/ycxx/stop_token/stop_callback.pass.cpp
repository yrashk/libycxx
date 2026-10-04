// [stoptoken.concepts]/3.2: constructing a stop_callback with a token whose stop state has not
// received a request registers the callback; it runs when the request is made (/12: "the stop
// state's registered callback invocations shall be synchronously executed"); if stop was
// already requested it runs immediately "on the thread executing scb's constructor"; /3.3:
// destruction deregisters it (it then never runs); a disengaged token: nothing is registered.
// [stopcallback.general]: deduction guide, callback_type, not copyable or movable;
// [stopcallback.cons]: the constructor is noexcept iff the initialization of the callback is.
#include <stop_token>
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Fn {
  int* p;
  void operator()() { ++*p; }
};
struct ThrowingCopy {
  ThrowingCopy() = default;
  ThrowingCopy(const ThrowingCopy&) noexcept(false) {}
  void operator()() {}
};

static_assert(!std::is_copy_constructible_v<std::stop_callback<Fn>>);
static_assert(!std::is_move_constructible_v<std::stop_callback<Fn>>);
static_assert(std::is_same_v<std::stop_callback<Fn>::callback_type, Fn>);
static_assert(std::is_nothrow_constructible_v<std::stop_callback<Fn>, std::stop_token, Fn>);
static_assert(!std::is_nothrow_constructible_v<std::stop_callback<ThrowingCopy>, std::stop_token, const ThrowingCopy&>);
static_assert(!std::is_convertible_v<std::stop_token, std::stop_callback<Fn>>);  // explicit
static_assert(!std::is_constructible_v<std::stop_callback<Fn>, std::stop_token, int>);

int main() {
  int n = 0;
  std::stop_source src;
  {
    std::stop_callback cb(src.get_token(), Fn{&n});
    static_assert(std::is_same_v<decltype(cb), std::stop_callback<Fn>>);
    CHECK(n == 0);
    CHECK(src.request_stop());
    CHECK(n == 1);
    CHECK(!src.request_stop());
    CHECK(n == 1);  // only once
  }
  CHECK(n == 1);

  // already stopped: runs in the constructor
  {
    std::stop_callback cb(src.get_token(), [&] { n += 10; });
    CHECK(n == 11);
  }
  CHECK(n == 11);

  // deregistered before the request: never runs
  std::stop_source s2;
  {
    std::stop_callback cb(s2.get_token(), Fn{&n});
  }
  s2.request_stop();
  CHECK(n == 11);

  // disengaged token: never runs
  {
    std::stop_callback cb(std::stop_token(), Fn{&n});
  }
  CHECK(n == 11);

  // several callbacks, rvalue token, std::function
  std::stop_source s3;
  int a = 0, b = 0;
  std::stop_token tok = s3.get_token();
  std::stop_callback c1(tok, [&] { ++a; });
  std::stop_callback c2(std::move(tok), [&] { ++b; });
  std::stop_callback<std::function<void()>> c3(s3.get_token(), [&] { a += 100; });
  s3.request_stop();
  CHECK(a == 101 && b == 1);

  // a callback may destroy its own stop_callback while running ([stoptoken.concepts]/3.3.4:
  // "If callback_fn is executing on the current thread, then the destructor shall not block").
  std::stop_source s4;
  struct Holder {
    std::stop_callback<std::function<void()>>* self = nullptr;
  } h;
  int ran = 0;
  auto* cb4 = new std::stop_callback<std::function<void()>>(s4.get_token(), [&] {
    ++ran;
    delete h.self;
  });
  h.self = cb4;
  s4.request_stop();
  CHECK(ran == 1);
  return 0;
}
