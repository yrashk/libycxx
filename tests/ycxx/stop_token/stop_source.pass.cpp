// [stopsource.cons]/2: stop_source(): "stop_possible() is true and stop_requested() is false";
// explicit stop_source(nostopstate_t) is disengaged; [stopsource.mem]: get_token() returns
// stop_token() if !stop_possible(), otherwise an associated token; request_stop() makes a stop
// request and returns true only the first time ([stoptoken.concepts]/12); a disengaged source
// returns false. Copies share the stop state and compare equal; swap.
#include <stop_token>
#include <type_traits>
#include <utility>
#include "check.hpp"

static_assert(std::is_nothrow_copy_constructible_v<std::stop_source>);
static_assert(std::is_nothrow_move_constructible_v<std::stop_source>);
static_assert(!std::is_convertible_v<std::nostopstate_t, std::stop_source>);
static_assert(std::is_nothrow_constructible_v<std::stop_source, std::nostopstate_t>);
static_assert(noexcept(std::declval<std::stop_source&>().request_stop()));
static_assert(noexcept(std::declval<const std::stop_source&>().get_token()));
static_assert(std::is_same_v<decltype(std::nostopstate), const std::nostopstate_t>);

int main() {
  std::stop_source s;
  CHECK(s.stop_possible());
  CHECK(!s.stop_requested());
  std::stop_token t = s.get_token();
  CHECK(t.stop_possible());
  CHECK(!t.stop_requested());

  std::stop_source copy = s;
  CHECK(copy == s);
  std::stop_source other;
  CHECK(other != s);

  CHECK(copy.request_stop());
  CHECK(s.stop_requested() && t.stop_requested());
  CHECK(!s.request_stop());
  CHECK(s.stop_possible());

  std::stop_source none(std::nostopstate);
  CHECK(!none.stop_possible());
  CHECK(!none.stop_requested());
  CHECK(!none.request_stop());
  CHECK(none.get_token() == std::stop_token());
  CHECK(!none.get_token().stop_possible());
  CHECK(none == std::stop_source(std::nostopstate));
  CHECK(none != s);

  std::stop_source a, b;
  std::stop_source a2 = a;
  a.swap(b);
  CHECK(b == a2 && a != a2);
  swap(a, b);
  CHECK(a == a2);

  std::stop_source moved = std::move(a2);
  CHECK(moved == a);
  CHECK(!a2.stop_possible());  // moved-from: disengaged (shared_ptr semantics of stop-state)
  return 0;
}
