// basic_string reports errors during constant evaluation as at run time (P3068: a throw-expression
// caught within the evaluation is a constant expression; the members below are constexpr):
// (stdexcept/constexpr_library_throws covers the basic positions on a short string; this test
// covers the second-string positions, strings in allocated storage, length_error and the
// no-effects guarantee.)
// [string.access]/6: at(pos) throws out_of_range if pos >= size();
// [string.substr]/2: substr(pos, n) && is basic_string(std::move(*this), pos, n), which
//   throws out_of_range if pos > size() ([string.cons]/6);
// [string.insert]/2, /6, [string.replace]/2, /6, [string.append], [string.assign],
// [string.compare]/10: the overloads taking (str, pos2[, n2]) throw out_of_range if
//   pos2 > str.size() (through basic_string_view's substr, [string.view.ops]/10);
// [string.capacity]/15: reserve(n) throws length_error if n > max_size();
// [string.require]/1: an operation that would make size() exceed max_size() throws length_error;
// [string.require]/2: a member that throws has no other effect on the string.
// XFAIL: clang Clang 23 cannot throw during constant evaluation (P3068's core-language part)
// REQUIRES: exceptions
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include "check.hpp"

// Runs f; true iff it threw E (and nothing else).
template <class E, class F>
constexpr bool throws(F f) {
  try {
    f();
  } catch (const E&) {
    return true;
  } catch (...) {
    return false;
  }
  return false;
}

// A string longer than any small-string buffer, so its characters live in allocated storage.
constexpr std::string long_text() { return std::string("0123456789abcdefghijklmnopqrstuvwxyz0123456789"); }

constexpr bool out_of_range_members() {
  std::string s = long_text();
  const std::string orig = s;
  const auto n = s.size();
  int ok = 0;
  ok += throws<std::out_of_range>([&] { (void)s.at(n); });
  ok += throws<std::out_of_range>([&] { s.insert(n + 1, 3, 'x'); });
  ok += throws<std::out_of_range>([&] { s.insert(0, orig, n + 1, 1); }); // pos2 > str.size()
  ok += throws<std::out_of_range>([&] { s.replace(0, 1, orig, n + 1, 1); });
  ok += throws<std::out_of_range>([&] { s.append(orig, n + 1); });
  ok += throws<std::out_of_range>([&] { s.assign(orig, n + 1); });
  ok += throws<std::out_of_range>([&] { (void)s.compare(0, 1, orig, n + 1); });
  ok += throws<std::out_of_range>([&] { (void)std::move(s).substr(n + 1); });
  ok += !throws<std::out_of_range>([&] { (void)s.substr(n); }); // pos == size(): empty
  // [string.require]/2: none of them changed s (substr && throws before moving from s)
  return ok == 9 && s == orig && s.size() == n;
}
static_assert(out_of_range_members());

constexpr bool length_errors() {
  std::string s = long_text();
  const std::string orig = s;
  const auto cap = s.capacity();
  int ok = 0;
  ok += throws<std::length_error>([&] { s.reserve(s.max_size() + 1); });
  ok += throws<std::length_error>([&] { s.resize(s.max_size() + 1); });
  ok += throws<std::length_error>([&] { s.append(s.max_size(), 'x'); });
  ok += throws<std::length_error>([&] { s.insert(0, s.max_size(), 'x'); });
  ok += throws<std::length_error>([&] { s.replace(0, 1, s.max_size(), 'x'); });
  ok += throws<std::length_error>([&] { std::string t(std::string().max_size() + 1, 'x'); });
  return ok == 6 && s == orig && s.capacity() == cap;
}
static_assert(length_errors());

// The exception carries a message and is a logic_error, caught after unwinding through the
// string's members.
constexpr bool caught_as_bases() {
  int ok = 0;
  try {
    std::string s("ab");
    (void)s.at(2);
  } catch (const std::logic_error& e) {
    ok += std::string_view(e.what()).size() > 0;
  }
  try {
    std::string s;
    s.reserve(s.max_size() + 1);
  } catch (const std::exception&) {
    ++ok;
  }
  return ok == 2;
}
static_assert(caught_as_bases());

// The same members on a wide string.
constexpr bool wide() {
  std::wstring w(L"wide characters beyond the small buffer");
  const std::wstring orig = w;
  int ok = 0;
  ok += throws<std::out_of_range>([&] { (void)w.at(w.size()); });
  ok += throws<std::out_of_range>([&] { w.insert(w.size() + 1, L"x"); });
  ok += throws<std::length_error>([&] { w.reserve(w.max_size() + 1); });
  ok += throws<std::length_error>([&] { w.append(w.max_size(), L'x'); });
  return ok == 4 && w == orig;
}
static_assert(wide());

int main() {
  CHECK(out_of_range_members());
  CHECK(length_errors());
  CHECK(caught_as_bases());
  CHECK(wide());
}
