// Library functions that report errors with <stdexcept> classes, called during constant
// evaluation, with the exception caught within that evaluation (P3068, P3378):
// [string.access]/at: out_of_range if pos >= size(); [string.substr]: out_of_range if
// pos > size(); [string.cons]: basic_string(const basic_string& str, size_type pos, ...) throws
// out_of_range if pos > str.size(); [string.insert], [string.erase], [string.copy],
// [string.compare]: out_of_range for a position > size();
// [string.view.access]/5, [string.view.ops]/5, /10: string_view::at, copy, substr;
// [sequence.reqmts] a.at(n): out_of_range if n >= a.size() (vector, vector<bool>, array); [vector.capacity]: reserve(n) throws length_error if n > max_size();
// [bitset.cons]/7: out_of_range if pos > str.size(), invalid_argument if any of the rlen
// characters is other than zero or one (rlen = min(n, str.size() - pos), so characters past
// rlen are not examined, but those past N within rlen are); /9 (const charT*) is defined in
// terms of /2; [bitset.members]: set/reset/flip/test with an invalid position throw
// out_of_range, to_ulong/to_ullong throw overflow_error if the value does not fit.
// [res.on.exception.handling]/1: an exception of a type derived from the named one is allowed,
// so each check catches the named type.
// XFAIL-COMPILER: clang  no constexpr exception support (P3068) in clang yet
// REQUIRES: exceptions
#include <stdexcept>
#include <array>
#include <bitset>
#include <climits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "check.hpp"

template <class E, class F>
constexpr bool throws(F f) {
  try {
    f();
  } catch (const E& e) {
    return e.what() != nullptr;
  } catch (...) {
    return false;
  }
  return false;
}
template <class F>
constexpr bool no_throw(F f) {
  try {
    f();
  } catch (...) {
    return false;
  }
  return true;
}

constexpr bool strings() {
  using std::out_of_range;
  std::string s = "abc";
  const std::string& cs = s;
  if (!throws<out_of_range>([&] { (void)s.at(3); })) return false;
  if (!throws<out_of_range>([&] { (void)cs.at(100); })) return false;
  if (!no_throw([&] { (void)s.at(2); })) return false;
  if (!throws<out_of_range>([&] { (void)s.substr(4); })) return false;
  if (!no_throw([&] { (void)s.substr(3); })) return false;  // pos == size() is valid
  if (!throws<out_of_range>([&] { std::string t(s, 4); })) return false;
  if (!throws<out_of_range>([&] { std::string t(s, 5, 1); })) return false;
  if (!throws<out_of_range>([&] { s.insert(4, "x"); })) return false;
  if (!throws<out_of_range>([&] { s.erase(4); })) return false;
  if (!throws<out_of_range>([&] { s.replace(4, 1, "x"); })) return false;
  if (!throws<out_of_range>([&] { (void)s.compare(4, 1, "x"); })) return false;
  if (!throws<out_of_range>([&] {
        char buf[4]{};
        (void)s.copy(buf, 1, 4);
      }))
    return false;
  if (s != "abc") return false;  // [res.on.exception.handling]: no change on these failures
  std::string_view v = "xyz";
  if (!throws<out_of_range>([&] { (void)v.at(3); })) return false;
  if (!throws<out_of_range>([&] { (void)v.substr(4); })) return false;
  if (!throws<out_of_range>([&] {
        char buf[2]{};
        (void)v.copy(buf, 1, 4);
      }))
    return false;
  return true;
}
static_assert(strings());

constexpr bool sequences() {
  using std::out_of_range;
  std::vector<int> v{1, 2, 3};
  if (!throws<out_of_range>([&] { (void)v.at(3); })) return false;
  if (!throws<out_of_range>([&] { (void)std::as_const(v).at(7); })) return false;
  if (!throws<std::length_error>([&] { v.reserve(v.max_size() + 1); })) return false;
  if (v.size() != 3 || v[2] != 3) return false;
  std::vector<bool> b{true, false};
  if (!throws<out_of_range>([&] { (void)b.at(2); })) return false;
  if (!throws<out_of_range>([&] { (void)std::as_const(b).at(2); })) return false;
  if (!throws<std::length_error>([&] { b.reserve(b.max_size() + 1); })) return false;
  std::array<int, 2> a{};
  if (!throws<out_of_range>([&] { (void)a.at(2); })) return false;
  if (!throws<out_of_range>([&] { (void)std::as_const(a).at(2); })) return false;
  return true;
}
static_assert(sequences());

constexpr bool bitsets() {
  using std::invalid_argument;
  using std::out_of_range;
  using std::string;
  if (!throws<invalid_argument>([] { std::bitset<8> b(string("10x1")); })) return false;
  if (!throws<invalid_argument>([] { std::bitset<8> b("1012"); })) return false;
  if (!throws<invalid_argument>([] { std::bitset<8> b(std::string_view("2")); })) return false;
  // a bad character past N but within rlen is still examined
  if (!throws<invalid_argument>([] { std::bitset<2> b(string("10x")); })) return false;
  if (!throws<invalid_argument>([] { std::bitset<2> b("10x"); })) return false;
  // a bad character past rlen is not
  if (!no_throw([] { std::bitset<8> b(string("10x"), 0, 2); })) return false;
  if (!no_throw([] { std::bitset<8> b("10x", 2); })) return false;
  if (!no_throw([] { std::bitset<8> b(string("x10"), 1); })) return false;
  // custom zero/one characters: '0' and '1' are then invalid
  if (!throws<invalid_argument>([] { std::bitset<4> b("ab0", 3, 'a', 'b'); })) return false;
  if (!no_throw([] { std::bitset<4> b("abba", 4, 'a', 'b'); })) return false;
  // pos > str.size()
  if (!throws<out_of_range>([] { std::bitset<8> b(string("101"), 4); })) return false;
  if (!no_throw([] { std::bitset<8> b(string("101"), 3); })) return false;

  std::bitset<8> b;
  if (!throws<out_of_range>([&] { b.set(8); })) return false;
  if (!throws<out_of_range>([&] { b.set(8, false); })) return false;
  if (!throws<out_of_range>([&] { b.reset(9); })) return false;
  if (!throws<out_of_range>([&] { b.flip(100); })) return false;
  if (!throws<out_of_range>([&] { (void)b.test(8); })) return false;
  if (b.any()) return false;

  std::bitset<sizeof(unsigned long) * CHAR_BIT + 1> wide;
  wide.set(sizeof(unsigned long) * CHAR_BIT);
  if (!throws<std::overflow_error>([&] { (void)wide.to_ulong(); })) return false;
  std::bitset<sizeof(unsigned long long) * CHAR_BIT + 8> wider;
  wider.set(sizeof(unsigned long long) * CHAR_BIT + 3);
  if (!throws<std::overflow_error>([&] { (void)wider.to_ullong(); })) return false;
  wider.reset();
  wider.set(0);
  if (!no_throw([&] { (void)wider.to_ullong(); })) return false;
  return true;
}
static_assert(bitsets());

int main() {
  CHECK(strings());
  CHECK(sequences());
  CHECK(bitsets());
  return 0;
}
