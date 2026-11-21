// config.hpp's portable fallbacks for compiler features (DECISIONS §5), compiled by
// tools/check_foreign_compiler.py with every probe answering 0.
#include <concepts>
#include <simd>
#include <type_traits>
#include <utility>

static_assert(std::is_same_v<std::remove_reference_t<int>, int>);
static_assert(std::is_same_v<std::remove_reference_t<const int&>, const int>);
static_assert(std::is_same_v<std::remove_reference_t<int (&&)[2]>, int[2]>);
static_assert(std::is_same_v<std::remove_reference_t<void() &>, void() &>);
static_assert(std::is_same_v<std::common_reference_t<int&, const int&>, const int&>);
static_assert(std::equality_comparable_with<int&, const long&>);

static_assert(std::is_same_v<std::make_integer_sequence<int, 0>, std::integer_sequence<int>>);
static_assert(std::is_same_v<std::make_integer_sequence<int, 1>, std::integer_sequence<int, 0>>);
static_assert(std::is_same_v<std::make_index_sequence<5>, std::index_sequence<0, 1, 2, 3, 4>>);
static_assert(std::is_same_v<std::make_integer_sequence<unsigned char, 6>,
                             std::integer_sequence<unsigned char, 0, 1, 2, 3, 4, 5>>);
static_assert(std::is_same_v<std::make_integer_sequence<long long, 3>, std::integer_sequence<long long, 0, 1, 2>>);
static_assert(std::is_same_v<std::index_sequence_for<int, char, void>, std::index_sequence<0, 1, 2>>);

template <std::size_t... Is>
constexpr bool ascending(std::index_sequence<Is...>) {
  constexpr std::size_t values[] = {Is...};
  for (std::size_t i = 0; i != sizeof...(Is); ++i)
    if (values[i] != i)
      return false;
  return true;
}
// Deep enough that a one-element-per-level recursion would exceed the instantiation depth.
static_assert(std::make_index_sequence<10007>::size() == 10007);
static_assert(ascending(std::make_index_sequence<10007>{}));

// A negative length is ill-formed ([intseq.make]/1): the fallback's constraint rejects it.
template <int N>
concept makeable = requires { typename std::make_integer_sequence<int, N>; };
static_assert(makeable<0> && makeable<7> && !makeable<-1>);

// Clang ignores the simulated unknown attribute. The scalar layout retains all public lanes.
static_assert(std::is_same_v<__ycxx::__detail::__simd_vector<int, 8>, int>);
using storage = __ycxx::__detail::__simd_storage<int, 8>;
static_assert(!storage::__is_vector);
static_assert(std::is_same_v<storage::__chunk_type, int>);
using vec = std::simd::vec<int, 8>;
static_assert(vec::size() == 8 && vec::mask_type::size() == 8);
static_assert([] {
  vec input([](int i) { return i; });
  auto mask = input > 1;
  auto result = std::simd::select(mask, input + 1, -input);
  for (int i = 0; i < vec::size(); ++i)
    if (mask[i] != (i > 1) || result[i] != (i > 1 ? i + 1 : -i))
      return false;
  return true;
}());
