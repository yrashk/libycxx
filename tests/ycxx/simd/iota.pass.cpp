// [simd.creation]/7-8: iota<T> for an arithmetic vectorizable T is T(); for an enabled basic_vec
// T it is T([](typename T::value_type i) { return i; }), i.e. 0, 1, ..., size()-1.
// Mandates: T::size() - 1 <= numeric_limits<value_type>::max().
#include <simd>
#include <type_traits>
#include "check.hpp"

namespace simd = std::simd;

static_assert(simd::iota<int> == 0 && simd::iota<double> == 0.0);
static_assert(std::is_same_v<decltype(simd::iota<int>), const int>);
static_assert(std::is_same_v<decltype(simd::iota<simd::vec<float, 3>>), const simd::vec<float, 3>>);

template <class V>
constexpr bool check() {
  for (int i = 0; i < V::size(); ++i)
    if (simd::iota<V>[i] != typename V::value_type(i)) return false;
  return true;
}
static_assert(check<simd::vec<int, 7>>() && check<simd::vec<float, 16>>() && check<simd::vec<unsigned char, 64>>());
static_assert(check<simd::vec<signed char, 64>>() && check<simd::vec<double>>());

// [simd.ctor]/2: the broadcast constructor takes an arithmetic From only when the conversion to
// value_type is value-preserving, so int -> short is excluded and vec<short> * 2 has no
// operator*; short(2) is accepted.
template <class V, class S> concept multipliable = requires(const V& v, S s) { v * s; };
static_assert(!multipliable<simd::vec<short, 9>, int> && multipliable<simd::vec<short, 9>, short>);

int main() {
  simd::vec<short, 9> v = simd::iota<simd::vec<short, 9>> * short(2);
  CHECK(v[0] == 0 && v[8] == 16);
  return 0;
}
