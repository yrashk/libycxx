// [string.cons]/28-36: operator= from basic_string (self-assignment has no effect), from
// basic_string&&, from string-view-like T, from const charT*, from charT, from
// initializer_list. [string.assign]: assign(str), assign(str&&), assign(str, pos, n),
// assign(t), assign(t, pos, n), assign(s, n), assign(s), assign(s, pos, n), assign(il),
// assign(n, c), assign(first, last), assign_range(rg); all return *this.
// [string.assign]/3,12 use substr, which throws out_of_range if pos > size()
// ([string.view.ops]).
// REQUIRES: exceptions
// COUNTERPART: libcxx:strings/basic.string/string.modifiers/string_assign/string.pass.cpp
#include <string>
#include <string_view>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool test() {
  std::string s = "initial";
  std::string other = "other";
  if (&(s = other) != &s || s != "other") return false;
  s = s;
  if (s != "other") return false;
  std::string m = "moved value that is long enough to live on the heap for sure";
  s = std::move(m);
  if (s != "moved value that is long enough to live on the heap for sure") return false;
  s = std::string_view("view");
  if (s != "view") return false;
  s = "ptr";
  if (s != "ptr") return false;
  s = 'c';
  if (s != "c" || s.size() != 1) return false;
  s = {'i', 'l'};
  if (s != "il") return false;

  std::string t;
  if (&t.assign(other) != &t || t != "other") return false;
  if (&t.assign(std::string("rvalue")) != &t || t != "rvalue") return false;
  if (t.assign(other, 1, 3) != "the") return false;
  if (t.assign(other, 2) != "her") return false;
  if (t.assign(other, 5) != "") return false;
  if (t.assign(std::string_view("sv-assign")) != "sv-assign") return false;
  if (t.assign(std::string_view("sv-assign"), 3, 3) != "ass") return false;
  if (t.assign(std::string_view("sv-assign"), 3) != "assign") return false;
  if (t.assign("abc\0de", 6).size() != 6) return false;
  if (t.assign("abc\0de") != "abc") return false;
  if (t.assign("abcdef", 2, 2) != "cd") return false;  // assign(const charT*, pos, n)
  if (t.assign({'x', 'y'}) != "xy") return false;
  if (t.assign(4, 'z') != "zzzz") return false;
  if (t.assign(0, 'z') != "") return false;
  char buf[] = "range";
  if (t.assign(InputIter<char>(buf), InputIter<char>(buf + 5)) != "range") return false;
  if (t.assign(buf + 1, buf + 3) != "an") return false;
  if (t.assign_range(InputRange<char>{buf, buf + 4}) != "rang") return false;
  if (&t.assign_range(std::string_view("ar")) != &t || t != "ar") return false;
  // assign from a part of itself
  t = "0123456789";
  t.assign(t, 2, 3);
  if (t != "234") return false;
  t = "0123456789";
  t.assign(t.data() + 5, 3);
  if (t != "567") return false;
  t = "0123456789";
  t.assign(t.begin() + 1, t.begin() + 4);
  if (t != "123") return false;
  return true;
}
static_assert(test());

static_assert(std::is_same_v<decltype(std::declval<std::string&>() = 'c'), std::string&>);
static_assert(std::is_same_v<decltype(std::declval<std::string&>().assign_range(std::string_view())),
                             std::string&>);

int main() {
  CHECK(test());
  std::string s = "abc";
  bool threw = false;
  try {
    s.assign(std::string("xy"), 3);
  } catch (const std::out_of_range&) {
    threw = true;
  }
  CHECK(threw);
  CHECK(s == "abc");  // [string.require]/2: no other effect
  threw = false;
  try {
    s.assign(std::string_view("xy"), 3, 1);
  } catch (const std::out_of_range&) {
    threw = true;
  }
  CHECK(threw);
  CHECK(s == "abc");
  return 0;
}
