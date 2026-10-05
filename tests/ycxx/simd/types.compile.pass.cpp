// [simd.syn], [simd.overview], [simd.mask.overview], [simd.traits], [simd.iterator], [simd.flags]:
// - vec<T, N> = basic_vec<T, deduce-abi-t<T, N>>, of width N for N in [1, 64]; vec<T> uses the
//   native ABI; mask<T, N> = vec<T, N>::mask_type = basic_mask<sizeof(T), Abi>;
// - enabled specializations are trivially copyable, size() is a static integral_constant equal
//   to mask_type::size(); value_type, abi_type, iterator/const_iterator (simd-iterator: a random
//   access iterator whose sentinel is default_sentinel_t, value_type V::value_type,
//   difference_type simd-size-type, iterator_category input_iterator_tag);
// - rebind_t<T, V> and resize_t<N, V> keep the width / element type and change the other;
//   alignment<V, U> has value for a basic_vec V and vectorizable U only;
// - flag_default/flag_convert/flag_aligned/flag_overaligned<N> are flags specializations and
//   operator| combines them (consteval).
// COUNTERPART: libcxx:experimental/simd/simd.traits/(memory_alignment|simd_size).*
#include <simd>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <string>
#include <type_traits>

namespace simd = std::simd;

template <class T, int N>
constexpr bool check_vec() {
  using V = simd::vec<T, N>;
  using M = typename V::mask_type;
  static_assert(V::size() == N && V::size == N);
  static_assert(std::is_same_v<std::remove_cvref_t<decltype(V::size)>, std::integral_constant<std::remove_cvref_t<decltype(V::size())>, N>>);
  static_assert(std::is_signed_v<decltype(V::size())>);  // simd-size-type
  static_assert(std::is_same_v<typename V::value_type, T>);
  static_assert(std::is_same_v<V, simd::basic_vec<T, typename V::abi_type>>);
  static_assert(std::is_same_v<M, simd::basic_mask<sizeof(T), typename V::abi_type>>);
  static_assert(std::is_same_v<simd::mask<T, N>, M>);
  static_assert(std::is_same_v<typename M::value_type, bool> && std::is_same_v<typename M::abi_type, typename V::abi_type>);
  static_assert(M::size() == N);
  static_assert(std::is_trivially_copyable_v<V> && std::is_trivially_copyable_v<M>);
  static_assert(std::is_nothrow_default_constructible_v<V> && std::is_nothrow_default_constructible_v<M>);
  using I = typename V::iterator;
  using CI = typename V::const_iterator;
  static_assert(std::random_access_iterator<I> && std::random_access_iterator<CI>);
  static_assert(std::is_same_v<std::iter_value_t<I>, T> && std::is_same_v<std::iter_reference_t<I>, T>);
  static_assert(std::is_same_v<typename std::iterator_traits<I>::iterator_category, std::input_iterator_tag>);
  static_assert(std::is_same_v<std::iter_difference_t<I>, decltype(V::size())>);
  static_assert(std::sized_sentinel_for<std::default_sentinel_t, I>);
  static_assert(std::is_convertible_v<I, CI> && !std::is_convertible_v<CI, I>);
  static_assert(std::is_same_v<decltype(std::declval<V&>().end()), std::default_sentinel_t>);
  static_assert(std::ranges::random_access_range<V> && std::ranges::sized_range<V>);
  static_assert(std::ranges::random_access_range<const V>);
  static_assert(std::is_same_v<std::ranges::range_value_t<M>, bool>);
  static_assert(std::ranges::random_access_range<M>);
  // rebind / resize
  static_assert(std::is_same_v<simd::rebind_t<double, V>, simd::vec<double, N>>);
  static_assert(std::is_same_v<simd::rebind_t<short, M>, simd::mask<short, N>>);
  static_assert(std::is_same_v<simd::resize_t<3, V>, simd::vec<T, 3>>);
  static_assert(std::is_same_v<simd::resize_t<1, M>, simd::mask<T, 1>>);
  static_assert(simd::alignment_v<V> > 0 && simd::alignment_v<V, char> > 0);
  static_assert(std::is_base_of_v<std::integral_constant<std::size_t, simd::alignment_v<V>>, simd::alignment<V>>);
  return true;
}

static_assert(check_vec<int, 4>());
static_assert(check_vec<float, 1>());
static_assert(check_vec<double, 3>());
static_assert(check_vec<unsigned char, 64>());
static_assert(check_vec<long long, 7>());
static_assert(check_vec<char32_t, 16>());
static_assert(check_vec<signed char, 33>());

// The native width is used by default and is at least 1.
static_assert(simd::vec<float>::size() >= 1);
static_assert(std::is_same_v<simd::vec<int>, simd::basic_vec<int>>);
static_assert(std::is_same_v<simd::mask<int>, simd::vec<int>::mask_type>);
static_assert(simd::mask<float>::size() == simd::vec<float>::size());
// Masks of equal element size and width are the same type.
static_assert(std::is_same_v<simd::mask<int, 4>, simd::mask<float, 4>>);
static_assert(std::is_same_v<simd::mask<unsigned, 4>, simd::mask<int, 4>>);

// [simd.traits]: rebind/resize/alignment have no member for non-data-parallel arguments.
template <class T, class V>
concept has_rebind = requires { typename simd::rebind<T, V>::type; };
template <int N, class V>
concept has_resize = requires { typename simd::resize<N, V>::type; };
template <class V, class U>
concept has_alignment = requires { simd::alignment<V, U>::value; };
static_assert(has_rebind<int, simd::vec<float, 4>> && !has_rebind<int, float> && !has_rebind<std::string, simd::vec<float, 4>>);
static_assert(has_resize<2, simd::vec<float, 4>> && !has_resize<2, float>);
static_assert(has_alignment<simd::vec<float, 4>, float> && has_alignment<simd::vec<float, 4>, short>);
static_assert(!has_alignment<simd::vec<float, 4>::mask_type, bool> && !has_alignment<simd::vec<float, 4>, std::string>);

// [simd.flags]
static_assert(std::is_same_v<decltype(simd::flag_default), const simd::flags<>>);
static_assert(std::is_empty_v<simd::flags<>>);
constexpr auto f1 = simd::flag_convert | simd::flag_aligned;
constexpr auto f2 = simd::flag_aligned | simd::flag_convert;
constexpr auto f3 = simd::flag_default | simd::flag_overaligned<32>;
static_assert(std::is_empty_v<decltype(f1)> && std::is_empty_v<decltype(f3)>);
template <class... F>
constexpr bool is_flags(simd::flags<F...>) { return true; }
static_assert(is_flags(f1) && is_flags(f2) && is_flags(f3) && is_flags(simd::flag_convert | simd::flag_convert));
template <std::size_t N>
concept overaligned_ok = requires { simd::flag_overaligned<N>; };
static_assert(overaligned_ok<16> && !overaligned_ok<24>);

int main() {}
