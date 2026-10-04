// [sequence.reqmts] a.at(n): "Throws: out_of_range if n >= a.size()". deque and inplace_vector
// are usable in constant evaluation in C++26 ([deque.overview]: all members constexpr;
// [inplace.vector.overview]), so the out_of_range can be thrown and caught during constant
// evaluation (P3068, P3378). inplace_vector::reserve(n) throws bad_alloc if n > capacity()
// ([inplace.vector.capacity]) - not a <stdexcept> class, checked here for completeness.
// XFAIL-COMPILER: clang  no constexpr exception support (P3068) in clang yet
#include <stdexcept>
#include <deque>
#include <inplace_vector>
#include <new>
#include <utility>
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

constexpr bool test() {
  std::deque<int> d{1};
  if (!throws<std::out_of_range>([&] { (void)d.at(1); })) return false;
  if (!throws<std::out_of_range>([&] { (void)std::as_const(d).at(5); })) return false;
  if (d.size() != 1 || d[0] != 1) return false;
  std::inplace_vector<int, 4> iv{1, 2};
  if (!throws<std::out_of_range>([&] { (void)iv.at(2); })) return false;
  if (!throws<std::out_of_range>([&] { (void)std::as_const(iv).at(4); })) return false;
  if (!throws<std::bad_alloc>([&] { iv.reserve(5); })) return false;
  if (iv.size() != 2) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
