// [alg.clamp]: clamp(v, lo, hi[, comp]) and ranges::clamp(v, lo, hi, comp, proj) return
// const T&: "lo if bool(invoke(comp, invoke(proj, v), invoke(proj, lo))) is true, hi if
// bool(invoke(comp, invoke(proj, hi), invoke(proj, v))) is true, otherwise v" -- so v
// itself when it is equivalent to a bound. "At most two comparisons and three applications
// of the projection." Floating-point T is fine when NaN is avoided.
#include <algorithm>
#include <functional>
#include <type_traits>
#include "sort_support.hpp"
#include "check.hpp"

constexpr bool test() {
  int lo = 1, hi = 5;
  int v = 3, below = 0, above = 9, at_lo = 1, at_hi = 5;
  static_assert(std::is_same_v<decltype(std::clamp(v, lo, hi)), const int&>);
  static_assert(std::is_same_v<decltype(std::ranges::clamp(v, lo, hi)), const int&>);
  if (&std::clamp(v, lo, hi) != &v) return false;
  if (&std::clamp(below, lo, hi) != &lo || &std::clamp(above, lo, hi) != &hi) return false;
  if (&std::clamp(at_lo, lo, hi) != &at_lo || &std::clamp(at_hi, lo, hi) != &at_hi) return false;
  // lo == hi
  int same = 4;
  if (&std::clamp(v, same, same) != &same) return false;
  int four = 4;
  if (&std::clamp(four, same, same) != &four) return false;
  // comparator: reversed bounds for greater
  if (&std::clamp(above, hi, lo, std::greater<>{}) != &hi) return false;
  if (&std::clamp(below, hi, lo, std::greater<>{}) != &lo) return false;
  if (&std::clamp(v, hi, lo, std::greater<>{}) != &v) return false;
  // floating point and explicit T
  if (std::clamp(2.5, 0.0, 1.0) != 1.0 || std::clamp(-0.5, 0.0, 1.0) != 0.0) return false;
  if (std::clamp<long>(7, 1L, 3L) != 3) return false;
  // ranges with projection
  KV kv{10, 0}, klo{2, 1}, khi{8, 2}, keq{8, 3};
  if (&std::ranges::clamp(kv, klo, khi, {}, &KV::key) != &khi) return false;
  if (&std::ranges::clamp(keq, klo, khi, {}, &KV::key) != &keq) return false;
  KV kmid{5, 4};
  if (&std::ranges::clamp(kmid, klo, khi, {}, &KV::key) != &kmid) return false;
  KV ksmall{-1, 5};
  if (&std::ranges::clamp(ksmall, klo, khi, {}, &KV::key) != &klo) return false;
  if (&std::ranges::clamp(above, lo, hi) != &hi) return false;
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  int lo = 10, hi = 20;
  for (int v : {0, 10, 15, 20, 30}) {
    int comps = 0, projs = 0;
    (void)std::clamp(v, lo, hi, CountingLess{&comps});
    CHECK(comps <= 2);
    comps = 0;
    (void)std::ranges::clamp(v, lo, hi, CountingLess{&comps}, CountingProj{&projs});
    CHECK(comps <= 2 && projs <= 3);
  }
  return 0;
}
