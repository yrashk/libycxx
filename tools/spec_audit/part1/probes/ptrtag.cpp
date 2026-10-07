// [ptrtag]: <memory>'s pointer tagging (all freestanding): signatures, constexpr (with the tag 0),
// noexcept, constraints, layout, the tuple interface and the deduction guides. A non-zero tag
// during constant evaluation is probed by ptrtag.pair.cons.cpp (compiler-blocked).
// FREESTANDING
#include <compare>
#include <concepts>
#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>

struct alignas(16) N {
  int v;
};
struct alignas(16) D : N {};
struct alignas(16) X {
  int x;
};
struct Two : X, N {};
enum class E : unsigned char { a, b };

// [ptrtag.bits]
static_assert(std::is_same_v<decltype(std::max_pointer_bits_available), const unsigned>);
static_assert(std::max_pointer_bits_available > 0);
static_assert(std::is_same_v<decltype(std::pointer_bits_available(8)), unsigned>);
static_assert(std::pointer_bits_available(1) == 0 && std::pointer_bits_available(16) == 4);
template <std::size_t A>
concept pba_constant = requires { typename std::integral_constant<unsigned, std::pointer_bits_available(A)>; };
static_assert(pba_constant<4> && !pba_constant<6>); // Constant When: power of two

// [ptrtag.pair.general]
using P = std::pointer_tag_pair<N*>;
static_assert(std::is_same_v<P, std::pointer_tag_pair<N*, 4, unsigned>>);
static_assert(std::is_same_v<P::pointer_type, N*> && std::is_same_v<P::element_type, N> &&
              std::is_same_v<P::tagged_pointer_type, void*> && std::is_same_v<P::tag_type, unsigned>);
static_assert(std::is_same_v<std::pointer_tag_pair<const int*>::tagged_pointer_type, const void*>);
static_assert(std::is_same_v<decltype(P::bits_requested), const unsigned> && P::bits_requested == 4);
static_assert(std::is_trivially_copyable_v<P> && std::copyable<P> && sizeof(P) == sizeof(N*) && alignof(P) == alignof(N*));
static_assert(std::is_constructible_v<P, D*, unsigned> && !std::is_constructible_v<P, Two*, unsigned>);
static_assert(!std::is_constructible_v<std::pointer_tag_pair<char*, 1>, char*, unsigned>);
static_assert(std::is_constructible_v<P, std::nullptr_t, unsigned>);
static_assert(std::is_same_v<decltype(std::pointer_tag_pair(static_cast<N*>(nullptr), E::a)), std::pointer_tag_pair<N*, 4, E>>);

// [ptrtag.pair.cons], [ptrtag.pair.overalign], [ptrtag.pair.accessors], [ptrtag.pair.swap],
// [ptrtag.pair.comp], [ptrtag.pair.get] in constant evaluation (tag 0)
N g[2];
constexpr bool ce() {
  P d;
  P a(&g[0], 0u), b(&g[1], 0u);
  auto o = std::pointer_tag_pair<char*, 4>::from_overaligned<16>(static_cast<char*>(nullptr), 0u);
  a.swap(b);
  auto [p, t] = a;
  return d.pointer() == nullptr && d.tag() == 0u && a.pointer() == &g[1] && (b <=> a) < 0 && b != a &&
         o.pointer() == nullptr && p == &g[1] && t == 0u && std::get<0>(b) == &g[0];
}
static_assert(ce());
static_assert(std::is_nothrow_default_constructible_v<P>);
static_assert(noexcept(std::declval<P&>().pointer()) && noexcept(std::declval<P&>().tag()) &&
              noexcept(std::declval<P&>().swap(std::declval<P&>())) && noexcept(std::declval<P&>().tagged_pointer()) &&
              noexcept(P::from_tagged(nullptr)));
static_assert(std::is_same_v<decltype(std::declval<P&>() <=> std::declval<P&>()), std::strong_ordering>);
static_assert(noexcept(std::declval<P&>() == std::declval<P&>()));

// [memory.syn], [ptrtag.pair.get]
static_assert(std::tuple_size_v<P> == 2 && std::tuple_size_v<const P> == 2);
static_assert(std::is_same_v<std::tuple_element_t<0, const P>, N*> && std::is_same_v<std::tuple_element_t<1, const P>, unsigned>);
static_assert(std::is_same_v<decltype(std::get<1>(std::declval<P>())), unsigned> && noexcept(std::get<0>(std::declval<P>())));
