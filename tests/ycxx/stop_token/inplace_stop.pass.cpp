// [stopsource.inplace]: inplace_stop_source: stop_possible() is always true; get_token()
// returns a token whose stop-source is this; request_stop() executes a stop request and
// "Postconditions: stop_requested() is true"; non-copyable, non-movable.
// [stoptoken.inplace]: a default inplace_stop_token is disengaged; tokens compare equal iff
// they reference the same source. [stopcallback.inplace]: callbacks registered / invoked /
// deregistered as for stop_callback; deduction guide.
#include <stop_token>
#include <type_traits>
#include "check.hpp"

static_assert(!std::is_copy_constructible_v<std::inplace_stop_source>);
static_assert(!std::is_move_constructible_v<std::inplace_stop_source>);
static_assert(std::is_nothrow_default_constructible_v<std::inplace_stop_source>);
static_assert(std::inplace_stop_source::stop_possible());
static_assert(std::is_same_v<std::inplace_stop_token::callback_type<void (*)()>,
                             std::inplace_stop_callback<void (*)()>>);
static_assert(!std::is_copy_constructible_v<std::inplace_stop_callback<void (*)()>>);

int main() {
  std::inplace_stop_token none;
  CHECK(!none.stop_possible() && !none.stop_requested());
  CHECK(none == std::inplace_stop_token());

  std::inplace_stop_source src;
  CHECK(!src.stop_requested());
  std::inplace_stop_token t = src.get_token();
  CHECK(t == src.get_token());
  CHECK(t != none);
  CHECK(t.stop_possible() && !t.stop_requested());
  std::inplace_stop_source other;
  CHECK(t != other.get_token());

  int n = 0;
  {
    std::inplace_stop_callback cb(t, [&] { ++n; });
    std::inplace_stop_callback cb2(t, [&] { n += 10; });
    {
      std::inplace_stop_callback gone(t, [&] { n += 1000; });
    }
    CHECK(src.request_stop());
    CHECK(n == 11);
    CHECK(src.stop_requested() && t.stop_requested());
    CHECK(!src.request_stop());
    CHECK(n == 11);
  }
  std::inplace_stop_callback late(t, [&] { n += 100; });
  CHECK(n == 111);

  std::inplace_stop_token a = other.get_token(), b;
  a.swap(b);
  CHECK(a == none && b == other.get_token());
  std::inplace_stop_callback on_none(none, [&] { n = -1; });
  CHECK(n == 111);
  return 0;
}
