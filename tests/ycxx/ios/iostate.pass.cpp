// [iostate.flags]: good() is rdstate() == 0; eof()/fail()/bad(); fail() is true if failbit or
// badbit is set; operator bool is !fail(); operator! is fail(); clear(state) sets rdstate() to
// state (| badbit if rdbuf() is null); setstate(state) is clear(rdstate() | state);
// exceptions(except) followed by a state matching it throws ios_base::failure, including from
// exceptions() itself when the current state already matches.
#include <ios>
#include <sstream>
#include <type_traits>
#include "check.hpp"

static_assert(!std::is_convertible_v<std::istringstream, bool>);
static_assert(std::is_constructible_v<bool, std::istringstream&>);

int main() {
  using B = std::ios_base;
  std::istringstream s("x");
  CHECK(s.good() && bool(s) && !!s);
  s.setstate(B::eofbit);
  CHECK(s.eof() && !s.fail() && !s.good() && bool(s));
  s.setstate(B::failbit);
  CHECK(s.fail() && !s && s.eof());
  s.clear(B::badbit);
  CHECK(s.bad() && s.fail() && !s.eof());
  s.clear();
  CHECK(s.good());

  std::ostream null(nullptr);
  null.clear();
  CHECK(null.bad());  // badbit added: rdbuf() is null

  bool threw = false;
  try {
    s.exceptions(B::failbit);
    s.setstate(B::eofbit);  // not covered
    CHECK(s.exceptions() == B::failbit);
    s.setstate(B::failbit);
  } catch (const B::failure& f) {
    threw = true;
  }
  CHECK(threw);
  CHECK(s.fail());

  std::istringstream t("");
  t.setstate(B::badbit);
  threw = false;
  try {
    t.exceptions(B::badbit);  // clear(rdstate()) throws at once
  } catch (const B::failure&) {
    threw = true;
  }
  CHECK(threw);

  // extraction failure with exceptions enabled
  std::istringstream u("abc");
  u.exceptions(B::failbit);
  threw = false;
  int i;
  try {
    u >> i;
  } catch (const B::failure&) {
    threw = true;
  }
  CHECK(threw && u.fail());
  return 0;
}
