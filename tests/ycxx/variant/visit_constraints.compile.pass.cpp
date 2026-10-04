// [variant.visit]/1-2: as-variant is defined only for (cv / ref) variant<Ts...>; visit is
// constrained on "Vi is a valid type for all 0<=i<n", where Vi is
// decltype(as-variant(std::forward<Variants_i>(vars_i))). So an argument that is not a
// variant (and not derived from exactly one variant specialization) removes the overload,
// while a class publicly derived from a variant is accepted.
#include <variant>
#include <utility>

struct Vis {
  int operator()(auto...) const { return 0; }
};
struct FromVariant : std::variant<int, long> {
  using variant::variant;
};
struct TwoVariants : std::variant<int>, std::variant<long> {};
struct NotVariant {
  using type = int;
};

template <class... A>
concept visitable = requires(A&&... a) { std::visit(Vis{}, std::forward<A>(a)...); };
template <class R, class... A>
concept visitable_r = requires(A&&... a) { std::visit<R>(Vis{}, std::forward<A>(a)...); };

static_assert(visitable<std::variant<int>&>);
static_assert(visitable<const std::variant<int, long>&&>);
static_assert(visitable<FromVariant&>);
static_assert(visitable<std::variant<int>&, FromVariant>);
static_assert(!visitable<int>);
static_assert(!visitable<NotVariant&>);
static_assert(!visitable<std::variant<int>&, int>);
static_assert(!visitable<TwoVariants&>);  // as-variant would be ambiguous
static_assert(visitable_r<long, std::variant<int>&>);
static_assert(!visitable_r<long, int&>);
