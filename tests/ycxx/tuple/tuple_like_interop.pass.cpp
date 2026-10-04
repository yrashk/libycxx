// [tuple.like]/1: tuple-like = specialization of array, complex, pair, tuple or ranges::subrange.
// [tuple.creation]/7: tuple_cat(Tuples&&...) for tuple-like arguments: CTypes are
//   tuple_element_t<k, remove_cvref_t<Ti>>, elements initialized from get<k>(std::forward<Ti>(tpi)).
// [tuple.rel]/1-7: operator==(const tuple<TTypes...>&, const UTuple&) and operator<=> for a
//   tuple-like UTuple with tuple_size_v<UTuple> == sizeof...(TTypes), the result of <=> is
//   common_comparison_category_t<synth-three-way-result<TTypes, Elems>...>; rewritten candidates
//   give the reversed forms.
// [tuple.cnstr]/28-31, [tuple.assign]: construction from and assignment from a tuple-like.
// [pairs.pair]/14-17, [pairs.pair] operator=(P&&): pair from / assigned from a pair-like
//   (tuple-like with size 2).
// [complex.tuple]: tuple_size<complex<T>> is 2, tuple_element is T, get<0> is the real part.
// [range.subrange.access]: get<0>/get<1> of a subrange return begin()/end() by value.
#include <tuple>
#include <utility>
#include <array>
#include <complex>
#include <ranges>
#include <compare>
#include <type_traits>
#include "check.hpp"

using std::tuple;

int arr[4] = {1, 2, 3, 4};

// tuple_cat element types.
using SR = std::ranges::subrange<int*>;
static_assert(std::is_same_v<decltype(std::tuple_cat(std::declval<SR>())), tuple<int*, int*>>);
static_assert(std::is_same_v<decltype(std::tuple_cat(std::declval<const SR&>())), tuple<int*, int*>>);
static_assert(std::is_same_v<decltype(std::tuple_cat(std::complex<float>())), tuple<float, float>>);
static_assert(std::is_same_v<decltype(std::tuple_cat(std::declval<const std::complex<double>&>(), std::pair<int, char>())),
                             tuple<double, double, int, char>>);
static_assert(std::is_same_v<decltype(std::tuple_cat(std::array<long, 1>(), SR(), tuple<>(), std::complex<double>())),
                             tuple<long, int*, int*, double, double>>);
using SRS = std::ranges::subrange<int*, int*, std::ranges::subrange_kind::sized>;
static_assert(std::is_same_v<decltype(std::tuple_cat(std::declval<SRS&>())), tuple<int*, int*>>);

// Comparisons with tuple-likes (constraints: same size, comparable elements).
template <class T, class U> concept Eq = requires(const T& t, const U& u) { t == u; u == t; t != u; };
template <class T, class U> concept Three = requires(const T& t, const U& u) { t <=> u; u <=> t; t < u; };
static_assert(Eq<tuple<int*, int*>, SR> && Three<tuple<int*, int*>, SR>);
static_assert(Eq<tuple<double, double>, std::complex<double>> && Three<tuple<double, double>, std::complex<double>>);
static_assert(Eq<tuple<int, long>, std::array<int, 2>> && Three<tuple<int, long>, std::pair<int, int>>);
static_assert(!Eq<tuple<int>, std::array<int, 2>> && !Three<tuple<int, int, int>, std::pair<int, int>>);
static_assert(!Eq<tuple<int, int>, std::complex<double>*>);
static_assert(std::is_same_v<decltype(tuple<double, double>() <=> std::complex<double>()), std::partial_ordering>);
static_assert(std::is_same_v<decltype(tuple<int, long>() <=> std::array<int, 2>()), std::strong_ordering>);
static_assert(std::is_same_v<decltype(tuple<int*, int*>() <=> SR()), std::strong_ordering>);
static_assert(std::is_same_v<decltype(tuple<float, int>() <=> std::pair<double, int>()), std::partial_ordering>);

// Construction from tuple-likes.
static_assert(std::is_constructible_v<tuple<double, double>, std::complex<double>>);
static_assert(std::is_convertible_v<std::complex<float>, tuple<double, double>>);
static_assert(std::is_constructible_v<tuple<int*, const int*>, SR>);
static_assert(!std::is_constructible_v<tuple<double>, std::complex<double>>);
static_assert(std::is_constructible_v<std::pair<double, double>, const std::complex<double>&>);
// [pairs.pair]/15.1 excludes subrange from pair(P&&), but subrange converts itself to a pair-like
// ([range.subrange.general]: operator PairLike()).
static_assert(std::is_convertible_v<SR, std::pair<int*, int*>>);
static_assert(std::is_assignable_v<tuple<double, double>&, std::complex<double>>);
static_assert(std::is_assignable_v<std::pair<double, double>&, const std::complex<double>&>);
static_assert(std::is_assignable_v<tuple<int*, int*>&, SR>);
static_assert(!std::is_assignable_v<tuple<int, int>&, std::complex<double>*>);

constexpr bool run() {
  SR sr(arr + 1, arr + 3);
  auto c = std::tuple_cat(sr, std::array<int, 1>{9});
  if (std::get<0>(c) != arr + 1 || std::get<1>(c) != arr + 3 || std::get<2>(c) != 9) return false;
  tuple<int*, int*> t(arr + 1, arr + 3);
  if (!(t == sr) || !(sr == t) || t != sr) return false;
  if ((t <=> sr) != 0) return false;
  tuple<int*, int*> t2(arr + 1, arr + 4);
  if (!(sr < t2) || !(t2 > sr) || (t2 <=> sr) <= 0) return false;
  std::complex<double> z(1.0, 2.0);
  auto cz = std::tuple_cat(z, std::pair<int, int>(3, 4));
  if (std::get<0>(cz) != 1.0 || std::get<1>(cz) != 2.0 || std::get<3>(cz) != 4) return false;
  tuple<double, double> td(1.0, 3.0);
  if (td == z || !(td != z) || !(z < td) || !((td <=> z) > 0)) return false;
  tuple<double, double> nan(__builtin_nan(""), 0.0);
  if ((nan <=> z) != std::partial_ordering::unordered) return false;
  tuple<double, double> from(z);
  if (std::get<0>(from) != 1.0 || std::get<1>(from) != 2.0) return false;
  td = std::complex<double>(5.0, 6.0);
  if (std::get<0>(td) != 5.0 || std::get<1>(td) != 6.0) return false;
  std::pair<double, double> pz(z);
  if (pz.first != 1.0 || pz.second != 2.0) return false;
  pz = std::complex<double>(7.0, 8.0);
  if (pz.first != 7.0 || pz.second != 8.0) return false;
  tuple<int*, int*> ts;
  ts = sr;
  if (std::get<0>(ts) != arr + 1 || std::get<1>(ts) != arr + 3) return false;
  tuple<int, long> tl(3, 4);
  if (!(tl == std::array<int, 2>{3, 4}) || !(std::array<int, 2>{3, 5} > tl)) return false;
  return true;
}
static_assert(run());

int main() { CHECK(run()); }
