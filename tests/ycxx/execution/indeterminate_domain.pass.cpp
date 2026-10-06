// [exec.domain.indeterminate]: indeterminate_domain<Domains...> is default constructible and
// constructible from anything (noexcept); /3: its transform_sender returns
// default_domain().transform_sender(...) (so a sender is returned unchanged, an lvalue by
// reference), with the same exception specification. /4: common_type of two indeterminate
// domains collects their domains without duplicates; with a domain D that is not one, the common
// type is D for indeterminate_domain<> and indeterminate_domain<Domains..., D> (deduplicated)
// otherwise. [exec.get.delegation.scheduler]/2-3: get_delegation_scheduler is a forwarding
// query whose result must be a scheduler; sync_wait's environment provides one.
#include <execution>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace ex = std::execution;

struct D1 {};
struct D2 {};
struct D3 {};
template <class... Ds>
using ID = ex::indeterminate_domain<Ds...>;

static_assert(std::is_default_constructible_v<ID<D1>> && std::is_nothrow_constructible_v<ID<D1>, int>);
static_assert(std::is_nothrow_constructible_v<ID<>, D2&>);

// /4
static_assert(std::is_same_v<std::common_type_t<ID<D1>, ID<D2>>, ID<D1, D2>>);
static_assert(std::is_same_v<std::common_type_t<ID<D1, D2>, ID<D2, D3>>, ID<D1, D2, D3>>);
static_assert(std::is_same_v<std::common_type_t<ID<D1>, ID<D1>>, ID<D1>>);
static_assert(std::is_same_v<std::common_type_t<ID<>, D1>, D1>);
static_assert(std::is_same_v<std::common_type_t<D1, ID<>>, D1>);
static_assert(std::is_same_v<std::common_type_t<ID<D1>, D2>, ID<D1, D2>>);
static_assert(std::is_same_v<std::common_type_t<ID<D1, D2>, D1>, ID<D1, D2>>);

// /3
using J = decltype(ex::just(1));
static_assert(std::is_same_v<decltype(ID<D1, D2>::transform_sender(ex::set_value, std::declval<J&>(), ex::env<>())), J&>);
static_assert(std::is_same_v<decltype(ID<D1>::transform_sender(ex::start, std::declval<J>(), ex::env<>())), J>);
static_assert(noexcept(ID<D1>::transform_sender(ex::set_value, std::declval<J&>(), ex::env<>())) ==
              noexcept(ex::default_domain().transform_sender(ex::set_value, std::declval<J&>(), ex::env<>())));

// [exec.get.delegation.scheduler]
static_assert(std::forwarding_query(ex::get_delegation_scheduler));

int main() {
  J j = ex::just(1);
  CHECK(&ID<D1, D2>::transform_sender(ex::set_value, j, ex::env<>()) == &j);
  // sync_wait's environment has a delegation scheduler (its run_loop's).
  auto r = std::this_thread::sync_wait(ex::read_env(ex::get_delegation_scheduler) |
                                       ex::let_value([](auto sch) { return ex::schedule(sch) | ex::then([] { return 3; }); }));
  CHECK(r && std::get<0>(*r) == 3);
  auto r2 = std::this_thread::sync_wait(ex::read_env(ex::get_delegation_scheduler));
  static_assert(ex::scheduler<std::tuple_element_t<0, std::remove_cvref_t<decltype(*r2)>>>);
  CHECK(r2.has_value());
  return 0;
}
