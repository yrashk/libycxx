// [string.cons]/4-7: basic_string(str, pos[, n][, a]) for const& and && str: let s be the
// value of str and rlen be pos + min(n, s.size() - pos) (or s.size() without n); the value
// is [s.data() + pos, s.data() + rlen). Throws out_of_range if pos > s.size().
// /9-10: template<class T> basic_string(const T& t, size_type pos, size_type n, a) behaves
// as basic_string(sv.substr(pos, n), a), constrained only on convertibility to string_view.
// REQUIRES: exceptions
#include <string>
#include <string_view>
#include <stdexcept>
#include <utility>
#include "test_allocators.hpp"
#include "check.hpp"

constexpr bool test() {
  const std::string s = "0123456789";
  if (std::string(s, 0) != s) return false;
  if (std::string(s, 3) != "3456789") return false;
  if (std::string(s, 10) != "") return false;
  if (std::string(s, 2, 3) != "234") return false;
  if (std::string(s, 2, 100) != "23456789") return false;
  if (std::string(s, 2, std::string::npos) != "23456789") return false;
  if (std::string(s, 10, 5) != "") return false;
  if (std::string(s, 0, 0) != "") return false;
  {
    std::string m = s;
    std::string r(std::move(m), 4);
    if (r != "456789") return false;
  }
  {
    std::string m = s;
    std::string r(std::move(m), 4, 2);
    if (r != "45") return false;
  }
  {
    std::string m = "a long string to defeat any small-string buffer, really quite long";
    std::string r(std::move(m), 2, 4);
    if (r != "long") return false;
  }
  // template<class T> (t, pos, n): T need only be convertible to string_view,
  // const char* included.
  std::string_view sv = "abcdef";
  if (std::string(sv, 1, 3) != "bcd") return false;
  if (std::string(sv, 1, 100) != "bcdef") return false;
  if (std::string("abcdef", 2, 2) != "cd") return false;  // T = char[7]
  return true;
}
static_assert(test());

template <class F>
bool throws_out_of_range(F f) {
  try {
    f();
  } catch (const std::out_of_range&) {
    return true;
  } catch (...) {
    return false;
  }
  return false;
}

int main() {
  CHECK(test());
  const std::string s = "abc";
  CHECK(throws_out_of_range([&] { std::string t(s, 4); }));
  CHECK(throws_out_of_range([&] { std::string t(s, 4, 1); }));
  CHECK(throws_out_of_range([&] { std::string t(std::string("abc"), 4); }));
  CHECK(throws_out_of_range([&] { std::string t(std::string("abc"), 4, 0); }));
  CHECK(throws_out_of_range([&] { std::string t(std::string_view("abc"), 4, 1); }));
  CHECK(!throws_out_of_range([&] { std::string t(s, 3); }));
  {
    using S = std::basic_string<char, std::char_traits<char>, IdAlloc<char>>;
    S a("hello world", IdAlloc<char>(1));
    S b(a, 6, 5, IdAlloc<char>(2));
    CHECK(b == "world");
    CHECK(b.get_allocator().id == 2);
    S c(a, 6, IdAlloc<char>(3));
    CHECK(c == "world");
    CHECK(c.get_allocator().id == 3);
    S d(std::move(a), 0, 5, IdAlloc<char>(4));
    CHECK(d == "hello");
    CHECK(d.get_allocator().id == 4);
  }
  return 0;
}
