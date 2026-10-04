// [const.wrap.class]/2 Note 1: "The second template parameter to constant_wrapper is present to
// aid argument-dependent lookup in finding overloads for which constant_wrapper's wrapped value
// is a suitable argument, but for which the constant_wrapper itself is not." The type of the
// wrapped value is a template type argument, so its namespace and its hidden friends are
// associated with constant_wrapper<X> ([basic.lookup.argdep]/3); the call then converts the
// wrapper to the value through the conversion operator.
#include <utility>
#include <type_traits>

namespace N {
struct Thing {
  int v;
  friend constexpr int hidden(const Thing& t) { return t.v * 2; }
  friend constexpr Thing operator+(Thing a, Thing b) { return {a.v + b.v}; }
};
constexpr int probe(Thing t) { return t.v; }
}  // namespace N

static_assert(probe(std::cw<N::Thing{7}>) == 7);
static_assert(hidden(std::cw<N::Thing{4}>) == 8);
// both operands constexpr-params: cw-operators yields a constant_wrapper of the sum
static_assert(std::is_same_v<decltype(std::cw<N::Thing{1}> + std::cw<N::Thing{2}>),
                             std::constant_wrapper<N::Thing{3}>>);
// one operand a plain Thing: Thing's hidden friend is found and used
constexpr N::Thing two{2};
static_assert(std::is_same_v<decltype(std::cw<N::Thing{1}> + two), N::Thing>);
static_assert((std::cw<N::Thing{1}> + two).v == 3);
