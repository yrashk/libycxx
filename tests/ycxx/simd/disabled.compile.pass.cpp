// [simd.overview]/1: basic_vec<T, Abi> with a non-vectorizable T is disabled; a disabled
// specialization is a complete type with deleted default constructor, destructor, copy
// constructor and copy assignment, and only the value_type, abi_type and mask_type members.
// [simd.mask.overview]/1: basic_mask<Bytes, Abi> is disabled if no vectorizable type has size
// Bytes, with the same deleted members (only value_type and abi_type present).
// Querying these properties must not make the program ill-formed.
#include <simd>
#include <string>
#include <type_traits>

namespace simd = std::simd;
using Abi = simd::vec<int>::abi_type;

struct S {
  int i;
};
using DV = simd::basic_vec<S>;
static_assert(sizeof(DV) > 0);  // complete
static_assert(!std::is_default_constructible_v<DV> && !std::is_destructible_v<DV>);
static_assert(!std::is_copy_constructible_v<DV> && !std::is_copy_assignable_v<DV>);
static_assert(std::is_same_v<DV::value_type, S>);
using DV2 = simd::basic_vec<std::string, Abi>;
static_assert(!std::is_default_constructible_v<DV2> && !std::is_copy_constructible_v<DV2>);
using DV3 = simd::basic_vec<bool>;  // bool is not a vectorizable type ([simd.general]/2)
static_assert(!std::is_default_constructible_v<DV3>);

using DM = simd::basic_mask<3, Abi>;
static_assert(sizeof(DM) > 0);
static_assert(!std::is_default_constructible_v<DM> && !std::is_destructible_v<DM>);
static_assert(!std::is_copy_constructible_v<DM> && !std::is_copy_assignable_v<DM>);
static_assert(std::is_same_v<DM::value_type, bool>);

int main() {}
