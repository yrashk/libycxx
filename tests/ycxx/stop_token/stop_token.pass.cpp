// [stoptoken.general], [stoptoken.mem]: a default stop_token is disengaged:
// stop_requested() and stop_possible() are false; tokens compare equal iff they share a stop
// state or are both disengaged ([stoptoken.concepts]/7.5); stop_possible() is "false if
// ... a stop request was not made and there are no associated stop_source objects".
#include <stop_token>
#include <concepts>
#include <optional>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_nothrow_default_constructible_v<std::stop_token>);
static_assert(std::is_nothrow_copy_constructible_v<std::stop_token>);

int main() {
  std::stop_token d;
  CHECK(!d.stop_requested() && !d.stop_possible());
  CHECK(d == std::stop_token());

  std::stop_token t1, t2;
  {
    std::optional<std::stop_source> src(std::in_place);
    t1 = src->get_token();
    t2 = t1;
    CHECK(t1 == t2);
    CHECK(t1 != d);
    CHECK(t1 != std::stop_source().get_token());
    CHECK(t1.stop_possible());
    src.reset();  // no stop_source left and no request made
    CHECK(!t1.stop_possible());
    CHECK(!t1.stop_requested());
    CHECK(t1 == t2);  // still the same stop state
  }
  {
    std::optional<std::stop_source> src(std::in_place);
    std::stop_token t = src->get_token();
    src->request_stop();
    src.reset();
    CHECK(t.stop_possible());  // a request was made
    CHECK(t.stop_requested());
  }
  std::stop_source s;
  std::stop_token a = s.get_token(), b;
  a.swap(b);
  CHECK(!a.stop_possible() && b.stop_possible());
  swap(a, b);
  CHECK(a == s.get_token() && b == std::stop_token());

  return 0;
}
