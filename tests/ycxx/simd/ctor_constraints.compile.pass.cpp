// [simd.ctor]/2: the broadcast constructor basic_vec(U&&) (implicit) participates only if
//   (2.1) U converts to value_type and is neither arithmetic nor constexpr-wrapper-like, or
//   (2.2) U is arithmetic and the conversion to value_type is value-preserving ([simd.general]/8:
//         every value of U is representable in value_type), or
//   (2.3) U is constexpr-wrapper-like with an arithmetic value representable by value_type.
// [simd.ctor]/4,6: the converting constructor from basic_vec<U, UAbi> needs the same width and
//   is explicit iff the conversion is not value-preserving, or U has greater integer /
//   floating-point conversion rank than value_type.
// [simd.ctor]/8: the generator constructor (explicit) needs gen(integral_constant<simd-size-type,
//   i>()) convertible to value_type, value-preserving if arithmetic.
// [simd.ctor]/12: the range constructor needs a contiguous sized range of constant size equal to
//   size(), with a vectorizable value type explicitly convertible to T.
// [simd.ctor]/16-19: deduction from such a range gives vec<range_value_t<R>, size>, from a mask
//   decltype(+k).
// [simd.mask.ctor]: basic_mask(bool) and basic_mask(unsigned_integral) are explicit, the bitset
//   constructor is implicit, the converting constructor needs equal width and is explicit.
// COUNTERPART: libcxx:experimental/simd/simd.class/.*
#include <simd>
#include <array>
#include <bitset>
#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace simd = std::simd;
using VF = simd::vec<float, 4>;
using VD = simd::vec<double, 4>;
using VI = simd::vec<int, 4>;
using VU = simd::vec<unsigned, 4>;
using VS = simd::vec<short, 4>;
using VLL = simd::vec<long long, 4>;
using VUC = simd::vec<unsigned char, 4>;

template <class To, class From>
constexpr bool implicit = std::is_constructible_v<To, From> && std::is_convertible_v<From, To>;
template <class To, class From>
constexpr bool explicit_only = std::is_constructible_v<To, From> && !std::is_convertible_v<From, To>;
template <class To, class From>
constexpr bool none = !std::is_constructible_v<To, From>;

// broadcast, arithmetic
static_assert(implicit<VF, float> && implicit<VF, short> && implicit<VF, unsigned char>);
static_assert(none<VF, int> && none<VF, double> && none<VF, long long>);
static_assert(implicit<VD, float> && implicit<VD, int> && implicit<VD, unsigned> && none<VD, long long>);
static_assert(implicit<VI, int> && implicit<VI, short> && implicit<VI, unsigned short> && implicit<VI, char>);
static_assert(none<VI, unsigned> && none<VI, long long> && none<VI, float>);
static_assert(implicit<VU, unsigned> && implicit<VU, unsigned char> && none<VU, int> && none<VU, signed char>);
static_assert(implicit<VLL, int> && implicit<VLL, unsigned> && none<VLL, unsigned long long>);
static_assert(implicit<VI, const int&> && implicit<VI, int&>);
// broadcast, constexpr-wrapper-like: decided by the value
static_assert(implicit<VUC, std::constant_wrapper<5>> && implicit<VUC, std::constant_wrapper<255>>);
static_assert(none<VUC, std::constant_wrapper<256>> && none<VUC, std::constant_wrapper<-1>>);
static_assert(implicit<VF, std::constant_wrapper<16777216>> && implicit<VF, std::constant_wrapper<1.5>>);
static_assert(implicit<VS, std::integral_constant<int, 7>> && none<VS, std::integral_constant<int, 70000>>);
static_assert(implicit<VU, std::integral_constant<int, 3>> && none<VU, std::integral_constant<int, -3>>);
// broadcast, other class types convertible to value_type
struct ToFloat {
  operator float() const { return 1; }
};
struct ToDouble {
  operator double() const { return 1; }
};
struct Nothing {};
static_assert(implicit<VF, ToFloat> && implicit<VF, ToDouble> && none<VF, Nothing>);

// converting constructor
static_assert(implicit<VI, VS> && explicit_only<VS, VI>);
static_assert(implicit<VLL, VI> && explicit_only<VI, VLL>);
static_assert(implicit<VD, VF> && explicit_only<VF, VD>);
static_assert(explicit_only<VF, VI> && implicit<VD, VI> && explicit_only<VU, VI> && explicit_only<VI, VU>);
static_assert(implicit<VI, simd::vec<unsigned short, 4>> && implicit<VF, simd::vec<std::int16_t, 4>>);
static_assert(implicit<simd::vec<long long, 4>, simd::vec<long, 4>>);  // rank of long is lower
static_assert(explicit_only<simd::vec<long, 4>, simd::vec<long long, 4>> || sizeof(long) < sizeof(long long));
static_assert(none<VI, simd::vec<int, 8>> && none<VI, simd::vec<short, 2>>);
static_assert(implicit<VI, const VI&> && std::is_nothrow_constructible_v<VI, VS>);

// generator constructor
auto gen_int = [](auto i) { return int(i); };
auto gen_float = [](auto) { return 1.0f; };
auto gen_ic = [](auto i) { return i; };  // returns integral_constant: a class type
auto gen_void = [](auto) {};
static_assert(explicit_only<VI, decltype(gen_int)> && explicit_only<VD, decltype(gen_int)>);
static_assert(none<VF, decltype(gen_int)> && none<VS, decltype(gen_int)>);
static_assert(explicit_only<VF, decltype(gen_float)> && none<VI, decltype(gen_float)>);
static_assert(explicit_only<VF, decltype(gen_ic)> && explicit_only<VS, decltype(gen_ic)>);
static_assert(none<VI, decltype(gen_void)>);

// range constructor
static_assert(std::is_constructible_v<VI, std::array<int, 4>&> && std::is_constructible_v<VI, const std::array<int, 4>&>);
static_assert(std::is_constructible_v<VI, std::span<const int, 4>> && std::is_constructible_v<VI, const int (&)[4]>);
static_assert(!std::is_constructible_v<VI, std::array<int, 3>&> && !std::is_constructible_v<VI, std::array<int, 5>&>);
static_assert(!std::is_constructible_v<VI, std::vector<int>&> && !std::is_constructible_v<VI, std::span<int>>);
static_assert(std::is_constructible_v<VI, std::array<int, 4>&, decltype(simd::flag_aligned)>);
static_assert(std::is_constructible_v<VI, std::array<double, 4>&, decltype(simd::flag_convert)>);
static_assert(std::is_constructible_v<VI, std::array<int, 4>&, const VI::mask_type&>);

// deduction guides
static_assert(std::is_same_v<decltype(simd::basic_vec(std::array<float, 3>{})), simd::vec<float, 3>>);
static_assert(std::is_same_v<decltype(simd::basic_vec(std::declval<std::span<const short, 8>>())), simd::vec<short, 8>>);
static_assert(std::is_same_v<decltype(simd::basic_vec(std::array<int, 4>{}, simd::flag_aligned)), simd::vec<int, 4>>);
static_assert(std::is_same_v<decltype(simd::basic_vec(VF::mask_type())), decltype(+VF::mask_type())>);

// basic_mask constructors
using M = VI::mask_type;
static_assert(explicit_only<M, bool> && none<M, int>);
static_assert(implicit<M, std::bitset<4>> && none<M, std::bitset<5>>);
static_assert(explicit_only<M, unsigned> && explicit_only<M, unsigned char> && none<M, int>);
static_assert(explicit_only<M, simd::mask<char, 4>> && none<M, simd::mask<int, 8>>);
static_assert(explicit_only<M, decltype([](auto) { return true; })>);
static_assert(none<M, decltype([](auto) { return 1; })>);  // gen must return bool exactly

int main() {}
