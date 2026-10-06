// [exec.adapt.general]/3.5: an adaptor whose children are all non-dependent is itself
// non-dependent ([exec.snd.concepts]/2: sender and not dependent_sender), so its completion
// signatures are known without an environment. With a dependent child (read_env, whose result
// needs the environment, [exec.snd.general]/2), check-types' get_completion_signatures of the
// child without an environment throws dependent_sender_error ([exec.getcomplsigs]/3.4), which
// leaves the adaptor dependent too. [exec.write.env]/5: write_env asks its child with JoinEnv...,
// the empty pack when it is asked without an environment, so writing the queried value does not
// make read_env non-dependent; with an environment it is answered from the written one.
#include <execution>
#include <type_traits>

namespace ex = std::execution;

struct q_t : std::forwarding_query_t {
  template <class Env>
  constexpr auto operator()(const Env& e) const noexcept -> decltype(e.query(*this)) {
    return e.query(*this);
  }
};
inline constexpr q_t q{};

template <class S>
constexpr bool non_dependent = ex::sender<S> && !ex::dependent_sender<S> && ex::sender_in<S>;

using J = decltype(ex::just(1));
using D = decltype(ex::read_env(q));
static_assert(non_dependent<J> && ex::dependent_sender<D>);

auto f = [](int x) noexcept { return x; };
auto g = [](int x) noexcept { return ex::just(x); };

static_assert(non_dependent<decltype(ex::then(J(), f))> && ex::dependent_sender<decltype(ex::then(D(), f))>);
static_assert(non_dependent<decltype(ex::upon_error(J(), f))> && ex::dependent_sender<decltype(ex::upon_error(D(), f))>);
static_assert(non_dependent<decltype(ex::let_value(J(), g))> && ex::dependent_sender<decltype(ex::let_value(D(), g))>);
static_assert(non_dependent<decltype(ex::when_all(J(), J()))> && ex::dependent_sender<decltype(ex::when_all(J(), D()))>);
static_assert(non_dependent<decltype(ex::into_variant(J()))> && ex::dependent_sender<decltype(ex::into_variant(D()))>);
static_assert(non_dependent<decltype(ex::stopped_as_optional(J()))> && ex::dependent_sender<decltype(ex::stopped_as_optional(D()))>);
static_assert(non_dependent<decltype(ex::stopped_as_error(J(), 1))> && ex::dependent_sender<decltype(ex::stopped_as_error(D(), 1))>);
static_assert(non_dependent<decltype(ex::bulk_chunked(J(), ex::seq, 2, [](int, int, int) noexcept {}))>);
static_assert(ex::dependent_sender<decltype(ex::bulk_chunked(D(), ex::seq, 2, [](int, int, int) noexcept {}))>);
static_assert(non_dependent<decltype(ex::schedule_from(J()))> && ex::dependent_sender<decltype(ex::schedule_from(D()))>);
static_assert(non_dependent<decltype(ex::when_all_with_variant(J()))>);

// write_env
using W = decltype(ex::write_env(D(), ex::prop(q, 1)));
static_assert(ex::dependent_sender<W>);
// q's result is prop's const int& ([exec.prop]), sent as that lvalue ([exec.read.env]/3).
static_assert(std::is_same_v<ex::completion_signatures_of_t<W, ex::env<>>, ex::completion_signatures<ex::set_value_t(const int&)>>);
