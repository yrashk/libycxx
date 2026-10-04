// [string.capacity]: size() == length(); max_size() >= size(); resize(n, c) erases or
// appends copies of c; resize(n) is resize(n, charT()); /14: after reserve(r), capacity()
// >= r if reallocation happened, otherwise unchanged; reallocation happens iff capacity() <
// r (so reserve never shrinks); /16: shrink_to_fit does not increase capacity();
// /15: reserve throws length_error if res_arg > max_size(). size() <= capacity() always
// ([basic.string.general]/3). [string.capacity]/18 note: without reallocation, pointers stay
// valid.
#include <string>
#include <stdexcept>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::string().max_size()), std::string::size_type>);
static_assert(noexcept(std::string().size()) && noexcept(std::string().capacity()));
static_assert(noexcept(std::string().max_size()) && noexcept(std::string().empty()));

constexpr bool test() {
  std::string s = "abc";
  if (s.size() != 3 || s.length() != 3 || s.capacity() < 3) return false;
  if (s.max_size() < s.size()) return false;
  s.resize(5, 'x');
  if (s != "abcxx") return false;
  s.resize(7);
  if (s.size() != 7 || s[5] != '\0' || s[6] != '\0') return false;
  s.resize(2);
  if (s != "ab" || s.data()[2] != '\0') return false;
  s.resize(0, 'q');
  if (!s.empty()) return false;

  std::string r;
  r.reserve(100);
  if (r.capacity() < 100) return false;
  auto cap = r.capacity();
  const char* p = r.data();
  for (int i = 0; i < 100; ++i) r.push_back('a');
  if (r.capacity() != cap || r.data() != p) return false;  // no reallocation
  r.reserve(10);  // smaller than capacity: no effect
  if (r.capacity() != cap || r.size() != 100) return false;
  r.reserve(cap);
  if (r.capacity() != cap) return false;
  r.resize(5);
  r.shrink_to_fit();
  if (r.capacity() > cap || r.capacity() < 5 || r != "aaaaa") return false;
  std::string e;
  auto ecap = e.capacity();
  e.shrink_to_fit();
  if (e.capacity() > ecap) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::string s = "abc";
  bool threw = false;
  if (s.max_size() < std::string::npos) {
    try {
      s.reserve(s.max_size() + 1);
    } catch (const std::length_error&) {
      threw = true;
    }
    CHECK(threw);
    CHECK(s == "abc");
  }
  return 0;
}
