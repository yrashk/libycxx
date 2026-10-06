// The checked accessors of the non-owning views and of array throw out_of_range during constant
// evaluation as at run time (P3068; the members are constexpr):
// [string.view.access]/5: at(pos) throws out_of_range if pos >= size();
// [string.view.ops]/5: copy(s, n, pos) throws out_of_range if pos > size(); /10: substr and
//   subview throw out_of_range if pos > size(); /15-16: compare(pos1, n1, ...) is
//   substr(pos1, n1).compare(...), so throws for pos1 > size(), and the second string's
//   substr(pos2, n2) for pos2 > str.size();
// [span.elem]/5: span::at(idx) throws out_of_range if idx >= size();
// [mdspan.mdspan.members]/10, /12: mdspan::at throws out_of_range if the indices are not a
//   multidimensional index in extents() (also through the span and array overloads);
// [sequence.reqmts]/127: array::at(n) throws out_of_range if n >= size(), also for array<T, 0>.
// XFAIL: clang Clang 23 cannot throw during constant evaluation (P3068's core-language part)
// REQUIRES: exceptions
#include <array>
#include <mdspan>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include "check.hpp"

template <class F>
constexpr bool oor(F f) {
  try {
    f();
  } catch (const std::out_of_range&) {
    return true;
  } catch (...) {
    return false;
  }
  return false;
}

constexpr bool string_views() {
  constexpr std::string_view s = "hello";
  int ok = 0;
  ok += oor([&] { (void)s.at(5); });
  ok += !oor([&] { (void)s.at(4); });
  ok += oor([&] { (void)s.substr(6); });
  ok += !oor([&] { (void)s.substr(5); }); // pos == size(): empty
  ok += oor([&] { (void)s.subview(6, 1); });
  ok += oor([&] {
    char buf[2]{};
    (void)s.copy(buf, 2, 6);
  });
  ok += !oor([&] {
    char buf[2]{};
    (void)s.copy(buf, 2, 5); // copies nothing
  });
  ok += oor([&] { (void)s.compare(6, 1, "x"); });
  ok += oor([&] { (void)s.compare(0, 1, std::string_view("ab"), 3, 1); });
  ok += oor([&] { (void)std::wstring_view(L"w").at(1); });
  ok += oor([&] { (void)std::u8string_view().at(0); });
  return ok == 11;
}
static_assert(string_views());

constexpr bool spans_and_arrays() {
  int a[4] = {1, 2, 3, 4};
  std::span<int> dyn(a);
  std::span<int, 4> fixed(a);
  std::array<int, 3> arr{7, 8, 9};
  std::array<int, 0> none{};
  int ok = 0;
  ok += oor([&] { (void)dyn.at(4); });
  ok += oor([&] { (void)fixed.at(4); });
  ok += oor([&] { (void)dyn.subspan(1).at(3); });
  ok += oor([&] { (void)std::span<int>().at(0); });
  ok += dyn.at(3) == 4;
  ok += oor([&] { (void)arr.at(3); });
  ok += oor([&] { (void)std::as_const(arr).at(3); });
  ok += oor([&] { (void)none.at(0); });
  return ok == 8;
}
static_assert(spans_and_arrays());

constexpr bool mdspans() {
  int a[6] = {0, 1, 2, 3, 4, 5};
  std::mdspan m(a, 2, 3);
  int ok = 0;
  ok += m.at(1, 2) == 5;
  ok += oor([&] { (void)m.at(2, 0); });
  ok += oor([&] { (void)m.at(0, 3); });
  ok += oor([&] { (void)m.at(-1, 0); }); // not a multidimensional index (negative)
  ok += oor([&] { (void)m.at(std::array<int, 2>{1, 3}); });
  int idx[2] = {2, 2};
  ok += oor([&] { (void)m.at(std::span<int, 2>(idx)); });
  std::mdspan<int, std::extents<int>> scalar(a);
  ok += scalar.at() == 0;
  return ok == 7;
}
static_assert(mdspans());

int main() {
  CHECK(string_views());
  CHECK(spans_and_arrays());
  CHECK(mdspans());
}
