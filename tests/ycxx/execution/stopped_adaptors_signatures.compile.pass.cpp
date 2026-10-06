// The completion signatures of stopped_as_optional and stopped_as_error, from their lowerings:
//   [exec.stopped.opt]/4: let_stopped(then(child, make optional<V>), just(optional<V>())): values
//     become set_value_t(optional<V>), errors pass through, stopped is gone, and
//     set_error_t(exception_ptr) appears only when constructing optional<V> from the datum can
//     throw (then's noexcept(is_nothrow_constructible_v<V, Ts...>));
//   [exec.stopped.err]/3: let_stopped(child, [err]() noexcept(nothrow move) { return
//     just_error(std::move(err)); }): stopped becomes set_error_t(E) with E = decltype(auto(err)),
//     the rest passes through.
//   [exec.stopped.err]/2: stopped_as_error(sndr, err) needs a sender and a movable-value err.
#include <exception>
#include <execution>
#include <optional>
#include <string>
#include <type_traits>
#include "exec_support.hpp"

namespace ex = std::execution;
using exec_test::same_sigs;
using EP = std::exception_ptr;

template <class... Sigs>
struct sigs_sender {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<Sigs...>();
  }
};

struct throwing_copy {
  throwing_copy() = default;
  throwing_copy(const throwing_copy&) {}
  throwing_copy(throwing_copy&&) noexcept = default;
};

using S1 = sigs_sender<ex::set_value_t(int), ex::set_error_t(long), ex::set_stopped_t()>;
static_assert(same_sigs<ex::completion_signatures_of_t<decltype(ex::stopped_as_optional(S1())), ex::env<>>,
                        ex::completion_signatures<ex::set_value_t(std::optional<int>), ex::set_error_t(long)>>);
static_assert(!ex::sends_stopped<decltype(ex::stopped_as_optional(S1())), ex::env<>>);

// Constructing optional<V> from a const lvalue whose copy can throw.
using S2 = sigs_sender<ex::set_value_t(const throwing_copy&), ex::set_stopped_t()>;
static_assert(same_sigs<ex::completion_signatures_of_t<decltype(ex::stopped_as_optional(S2())), ex::env<>>,
                        ex::completion_signatures<ex::set_value_t(std::optional<throwing_copy>), ex::set_error_t(EP)>>);
// ... from an rvalue whose move cannot: no exception_ptr.
using S3 = sigs_sender<ex::set_value_t(throwing_copy), ex::set_stopped_t()>;
static_assert(same_sigs<ex::completion_signatures_of_t<decltype(ex::stopped_as_optional(S3())), ex::env<>>,
                        ex::completion_signatures<ex::set_value_t(std::optional<throwing_copy>)>>);

// stopped_as_error
static_assert(same_sigs<ex::completion_signatures_of_t<decltype(ex::stopped_as_error(S1(), std::string("e"))), ex::env<>>,
                        ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(long), ex::set_error_t(std::string)>>);
static_assert(!ex::sends_stopped<decltype(ex::stopped_as_error(S1(), 1.5)), ex::env<>>);
// E is the decayed type of err.
const char* const msg = "e";
static_assert(same_sigs<ex::completion_signatures_of_t<decltype(ex::stopped_as_error(S1(), msg)), ex::env<>>,
                        ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(long), ex::set_error_t(const char*)>>);
// Without a stopped completion, the error is never sent (let_stopped has nothing to transform).
using S4 = sigs_sender<ex::set_value_t(int)>;
static_assert(same_sigs<ex::completion_signatures_of_t<decltype(ex::stopped_as_error(S4(), 2.5)), ex::env<>>,
                        ex::completion_signatures<ex::set_value_t(int)>>);

// /2
struct not_movable {
  not_movable() = default;
  not_movable(not_movable&&) = delete;
};
static_assert(!std::is_invocable_v<ex::stopped_as_error_t, S1, not_movable>);
static_assert(!std::is_invocable_v<ex::stopped_as_error_t, int, int>);
static_assert(std::is_invocable_v<ex::stopped_as_error_t, S1, int>);
