// [valarray.sub]: on a non-const valarray, subscripting with a slice, gslice, mask or index
// array returns slice_array<T>, gslice_array<T>, mask_array<T> and indirect_array<T>. (The const
// overloads return valarray<T>, which [valarray.syn]/3 lets an implementation replace with
// another type, so they are only checked for constructibility of valarray<T> from them.)
#include <valarray>
#include <type_traits>
#include <utility>

using V = std::valarray<long>;
static_assert(std::is_constructible_v<V, decltype(std::declval<const V&>()[std::slice()])>);
static_assert(std::is_constructible_v<V, decltype(std::declval<const V&>()[std::gslice()])>);
static_assert(std::is_constructible_v<V, decltype(std::declval<const V&>()[std::declval<const std::valarray<bool>&>()])>);
static_assert(std::is_constructible_v<V, decltype(std::declval<const V&>()[std::declval<const std::valarray<std::size_t>&>()])>);
static_assert(std::is_same_v<decltype(std::declval<V&>()[std::slice()]), std::slice_array<long>>);
static_assert(std::is_same_v<decltype(std::declval<V&>()[std::gslice()]), std::gslice_array<long>>);
static_assert(std::is_same_v<decltype(std::declval<V&>()[std::declval<const std::valarray<bool>&>()]), std::mask_array<long>>);
static_assert(std::is_same_v<decltype(std::declval<V&>()[std::declval<const std::valarray<std::size_t>&>()]), std::indirect_array<long>>);
static_assert(std::is_same_v<decltype(std::declval<const V&>()[0]), const long&>);
