// iterator_concept and iterator_category of the view iterators, over bases whose
// iterator_traits<>::iterator_category differs from their concept (iota_view's iterator: random
// access concept, input_iterator_tag category; vector: contiguous concept,
// random_access_iterator_tag category):
// [range.transform.iterator]/2: category input_iterator_tag unless the function returns a
//   reference, then C (random_access_iterator_tag for a contiguous C).
// [range.elements.iterator]/1-/2: concept from Base; category input_iterator_tag if
//   get<N>(*current_) is an rvalue, else C (capped at random access).
// [range.zip.transform.iterator]/1, [range.adjacent.transform.iterator]/1: input_iterator_tag
//   for a non-reference result, otherwise the weakest of the bases' categories.
// [range.stride.iterator]/2: C, capped at random access.
// [range.join.iterator]/2: declared only for a glvalue inner range; bidirectional if both
//   categories are and the inner range is common, else forward, else input.
// [range.join.with.iterator]/2: input_iterator_tag when the common reference of the inner and
//   pattern references is not a reference; bidirectional needs common inner and pattern ranges.
// [range.lazy.split.inner]/1: forward_iterator_tag if C is at least forward, else C.
// [range.filter.iterator]/3: bidirectional if C is, forward if C is, else C.
// [range.enumerate.iterator], [range.slide.iterator], [range.cartesian.iterator],
// [range.adjacent.iterator], [range.chunk.fwd.iter]: iterator_category is input_iterator_tag;
// [range.zip.iterator]/2: input_iterator_tag, present only if all bases are forward;
// [range.split.iterator]: concept forward_iterator_tag, category input_iterator_tag.
#include <forward_list>
#include <iterator>
#include <list>
#include <ranges>
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>

namespace rg = std::ranges;
namespace vw = std::views;
using std::bidirectional_iterator_tag, std::forward_iterator_tag, std::input_iterator_tag;
using std::random_access_iterator_tag;

template <class R>
using cat = std::iterator_traits<rg::iterator_t<R>>::iterator_category;
template <class R>
using con = rg::iterator_t<R>::iterator_concept;
template <class R>
concept has_cat = requires { typename rg::iterator_t<R>::iterator_category; };

template <class I>
concept it_has_cat = requires { typename I::iterator_category; };
template <class T>
T& val();
using Vec = std::vector<int>&;
using Lst = std::list<int>&;
using FL = std::forward_list<int>&;
using Iota = rg::iota_view<int, int>;
using VP = std::vector<std::pair<int, int>>&;
using VV = std::vector<std::vector<int>>&;
using VFL = std::vector<std::forward_list<int>>&;

inline int g = 0;
inline constexpr auto ref_fn = [](auto&&...) -> int& { return g; };
inline constexpr auto val_fn = [](auto&&...) { return 0; };

// the bases
static_assert(std::is_same_v<cat<Iota>, input_iterator_tag> && std::is_same_v<con<Iota>, random_access_iterator_tag>);

// transform
static_assert(std::is_same_v<cat<decltype(val<Vec>() | vw::transform(ref_fn))>, random_access_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(val<Lst>() | vw::transform(ref_fn))>, bidirectional_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(val<Lst>() | vw::transform(val_fn))>, input_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(Iota(0, 3) | vw::transform(ref_fn))>, input_iterator_tag>);
static_assert(std::is_same_v<con<decltype(Iota(0, 3) | vw::transform(ref_fn))>, random_access_iterator_tag>);

// elements
static_assert(std::is_same_v<cat<decltype(val<VP>() | vw::keys)>, random_access_iterator_tag>);
static_assert(std::is_same_v<con<decltype(val<VP>() | vw::keys)>, random_access_iterator_tag>);
inline constexpr auto mkpair = [](int i) { return std::pair<int, int>(i, i); };
using EP = decltype(val<Vec>() | vw::transform(mkpair) | vw::values);  // get<1> of a prvalue
static_assert(std::is_same_v<cat<EP>, input_iterator_tag> && std::is_same_v<con<EP>, random_access_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(val<std::list<std::pair<int, int>>&>() | vw::keys)>, bidirectional_iterator_tag>);

// zip_transform, adjacent_transform
static_assert(std::is_same_v<cat<decltype(vw::zip_transform(ref_fn, val<Vec>(), val<Lst>()))>, bidirectional_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(vw::zip_transform(ref_fn, val<Vec>(), val<FL>()))>, forward_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(vw::zip_transform(ref_fn, val<Vec>(), Iota(0, 2)))>, input_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(vw::zip_transform(val_fn, val<Vec>(), val<Vec>()))>, input_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(vw::zip_transform(ref_fn, val<Vec>(), val<Vec>()))>, random_access_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(val<Lst>() | vw::adjacent_transform<2>(ref_fn))>, bidirectional_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(val<Vec>() | vw::adjacent_transform<3>(val_fn))>, input_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(val<FL>() | vw::pairwise_transform(ref_fn))>, forward_iterator_tag>);

// stride
static_assert(std::is_same_v<cat<decltype(val<Vec>() | vw::stride(2))>, random_access_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(val<Lst>() | vw::stride(2))>, bidirectional_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(Iota(0, 9) | vw::stride(2))>, input_iterator_tag>);
static_assert(std::is_same_v<con<decltype(Iota(0, 9) | vw::stride(2))>, random_access_iterator_tag>);

// join
static_assert(std::is_same_v<cat<decltype(val<VV>() | vw::join)>, bidirectional_iterator_tag>);
static_assert(std::is_same_v<con<decltype(val<VV>() | vw::join)>, bidirectional_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(val<VFL>() | vw::join)>, forward_iterator_tag>);
using JT = decltype(val<VV>() | vw::transform([](std::vector<int>& v) -> auto& { return v; }) | vw::join);
static_assert(std::is_same_v<cat<JT>, bidirectional_iterator_tag>);
inline constexpr auto take2 = [](std::vector<int>& v) { return v | vw::take_while([](int x) { return x > 0; }); };
using JNC = decltype(val<VV>() | vw::transform(take2) | vw::join);  // prvalue inner ranges
static_assert(!has_cat<JNC>);
using JIo = decltype(Iota(0, 2) | vw::transform([](int) -> std::vector<int>& { static std::vector<int> v; return v; }) | vw::join);
static_assert(std::is_same_v<cat<JIo>, input_iterator_tag>);  // OUTERC is input_iterator_tag

// join_with
static_assert(std::is_same_v<cat<decltype(val<VV>() | vw::join_with(0))>, bidirectional_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(val<VV>() | vw::join_with(val<FL>()))>, forward_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(val<VV>() | vw::join_with(val<std::vector<long>&>()))>, input_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(val<VFL>() | vw::join_with(0))>, forward_iterator_tag>);

// lazy_split's inner iterator
using LS = decltype(val<Vec>() | vw::lazy_split(0));
using LSI = rg::iterator_t<rg::range_reference_t<LS>>;
static_assert(std::is_same_v<std::iterator_traits<LSI>::iterator_category, forward_iterator_tag>);
static_assert(std::is_same_v<LSI::iterator_concept, forward_iterator_tag>);
using LSIo = rg::iterator_t<rg::range_reference_t<decltype(Iota(0, 9) | vw::lazy_split(3))>>;
static_assert(std::is_same_v<std::iterator_traits<LSIo>::iterator_category, input_iterator_tag>);
using LSIn = rg::iterator_t<rg::range_reference_t<decltype(vw::istream<int>(val<std::istringstream&>()) | vw::lazy_split(3))>>;
static_assert(!it_has_cat<LSIn>);  // Base is not forward

// filter
static_assert(std::is_same_v<cat<decltype(val<Vec>() | vw::filter(val_fn))>, bidirectional_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(val<FL>() | vw::filter(val_fn))>, forward_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(Iota(0, 3) | vw::filter(val_fn))>, input_iterator_tag>);
static_assert(std::is_same_v<con<decltype(Iota(0, 3) | vw::filter(val_fn))>, bidirectional_iterator_tag>);

// always input_iterator_tag
static_assert(std::is_same_v<cat<decltype(val<Vec>() | vw::enumerate)>, input_iterator_tag>);
static_assert(std::is_same_v<con<decltype(val<Vec>() | vw::enumerate)>, random_access_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(val<Lst>() | vw::slide(2))>, input_iterator_tag>);
static_assert(std::is_same_v<con<decltype(val<Lst>() | vw::slide(2))>, bidirectional_iterator_tag>);
static_assert(std::is_same_v<con<decltype(val<FL>() | vw::slide(2))>, forward_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(vw::cartesian_product(val<Vec>(), val<Vec>()))>, input_iterator_tag>);
static_assert(std::is_same_v<con<decltype(vw::cartesian_product(val<Vec>(), val<Vec>()))>, random_access_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(val<Vec>() | vw::adjacent<2>)>, input_iterator_tag>);
static_assert(std::is_same_v<con<decltype(val<FL>() | vw::adjacent<2>)>, forward_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(val<Vec>() | vw::chunk(2))>, input_iterator_tag>);
static_assert(std::is_same_v<con<decltype(val<Vec>() | vw::chunk(2))>, random_access_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(vw::zip(val<Vec>(), val<Lst>()))>, input_iterator_tag>);
static_assert(std::is_same_v<con<decltype(vw::zip(val<Vec>(), val<Lst>()))>, bidirectional_iterator_tag>);
using ZI = decltype(vw::zip(val<Vec>(), vw::istream<int>(val<std::istringstream&>())));
static_assert(!has_cat<ZI> && std::is_same_v<con<ZI>, input_iterator_tag>);
static_assert(std::is_same_v<cat<decltype(val<Vec>() | vw::split(0))>, input_iterator_tag>);
static_assert(std::is_same_v<con<decltype(val<Vec>() | vw::split(0))>, forward_iterator_tag>);

int main() {}
