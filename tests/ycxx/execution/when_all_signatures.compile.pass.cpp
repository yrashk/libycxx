// when_all's and when_all_with_variant's well-formedness and completion signatures:
//   [exec.when.all]/2: ill-formed with no argument or with an argument that is not a sender.
//   /9: a child with two value completions, or a datum that cannot be decay-copied, makes
//     check-types exit with an exception: no completion signatures (not sender_in). (With
//     non-dependent children that is ill-formed: make-sender's Mandates, [exec.snd.expos]/24.4;
//     dependent children are used here.)
//   /15.3: set_stopped_t() is a completion only if some child sends stopped.
//   /15.1: the value completion sends the decayed datums of all children in order, as rvalues.
//   /18-19: when_all_with_variant(sndrs...) is a sender whose tag is when_all_with_variant_t;
//     when_all_with_variant.transform_sender(set_value, sndr, env) is
//     when_all(into_variant(child)...), and is ill-formed for any other sender; children with
//     several value completions are accepted.
// REQUIRES: exceptions
#include <execution>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include "exec_support.hpp"

namespace ex = std::execution;
using exec_test::same_sigs;

// /2
static_assert(!std::is_invocable_v<ex::when_all_t>);
static_assert(!std::is_invocable_v<ex::when_all_t, decltype(ex::just()), int>);
static_assert(!std::is_invocable_v<ex::when_all_with_variant_t>);
static_assert(!std::is_invocable_v<ex::when_all_with_variant_t, int>);

// A sender with the completions Sigs...
template <class... Sigs>
struct sigs_sender {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<Sigs...>();
  }
};

// The same, as a dependent sender: its completions need an environment.
template <class... Sigs>
struct dep_sender {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    if constexpr (sizeof...(Env) == 0)
      return (throw ex::dependent_sender_error(), ex::completion_signatures<Sigs...>());
    else
      return ex::completion_signatures<Sigs...>();
  }
};

// /9: two value completions.
using two_values = sigs_sender<ex::set_value_t(int), ex::set_value_t(double)>;
using dep_two_values = dep_sender<ex::set_value_t(int), ex::set_value_t(double)>;
static_assert(ex::dependent_sender<dep_two_values> && ex::sender_in<dep_two_values, ex::env<>>);
static_assert(ex::dependent_sender<decltype(ex::when_all(std::declval<dep_two_values>()))>);
static_assert(!ex::sender_in<decltype(ex::when_all(std::declval<dep_two_values>())), ex::env<>>);
static_assert(!ex::sender_in<decltype(ex::when_all(ex::just(), std::declval<dep_two_values>())), ex::env<>>);
// /9: a datum that cannot be decay-copied (a non-copyable lvalue).
using ref_unique = dep_sender<ex::set_value_t(std::unique_ptr<int>&)>;
static_assert(!ex::sender_in<decltype(ex::when_all(std::declval<ref_unique>())), ex::env<>>);
using err_unique = dep_sender<ex::set_error_t(const std::unique_ptr<int>&)>;
static_assert(!ex::sender_in<decltype(ex::when_all(std::declval<err_unique>())), ex::env<>>);
// ... while an rvalue of it can.
using rv_unique = sigs_sender<ex::set_value_t(std::unique_ptr<int>&&)>;
static_assert(ex::sender_in<decltype(ex::when_all(std::declval<rv_unique>())), ex::env<>>);
static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(ex::when_all(std::declval<rv_unique>())), ex::env<>>,
                             ex::completion_signatures<ex::set_value_t(std::unique_ptr<int>)>>);

// /15.3: no stopped completion unless a child has one.
static_assert(!ex::sends_stopped<decltype(ex::when_all(ex::just(1), ex::just_error(2))), ex::env<>>);
static_assert(ex::sends_stopped<decltype(ex::when_all(ex::just(1), sigs_sender<ex::set_value_t(), ex::set_stopped_t()>())), ex::env<>>);
// ... and the errors are the children's decayed errors.
static_assert(same_sigs<ex::completion_signatures_of_t<decltype(ex::when_all(sigs_sender<ex::set_value_t(int), ex::set_error_t(const long&)>(),
                                                                            sigs_sender<ex::set_value_t(), ex::set_error_t(std::string&&)>())),
                                                      ex::env<>>,
                        ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(long), ex::set_error_t(std::string)>>);

// /15.1: the decayed datums of all children, in order.
static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(ex::when_all(sigs_sender<ex::set_value_t(const int&, char)>(), ex::just(),
                                                                                 sigs_sender<ex::set_value_t(std::string&&)>())),
                                                           ex::env<>>,
                             ex::completion_signatures<ex::set_value_t(int, char, std::string)>>);

// /18-19
using WV = decltype(ex::when_all_with_variant(std::declval<two_values>(), ex::just(1)));
static_assert(ex::sender<WV>);
static_assert(ex::sender_in<WV, ex::env<>>);
static_assert(std::is_same_v<ex::value_types_of_t<WV, ex::env<>>,
                             std::variant<std::tuple<std::variant<std::tuple<int>, std::tuple<double>>, std::variant<std::tuple<int>>>>>);
consteval bool tag_is_when_all_with_variant() {
  auto s = ex::when_all_with_variant(ex::just(1));
  auto&& [tag, data, child] = s;
  return std::is_same_v<std::remove_cvref_t<decltype(tag)>, ex::when_all_with_variant_t>;
}
static_assert(tag_is_when_all_with_variant());
using Transformed = decltype(ex::when_all_with_variant.transform_sender(ex::set_value, ex::when_all_with_variant(ex::just(1), ex::just('c')), ex::env<>()));
using Expected = decltype(ex::when_all(ex::into_variant(ex::just(1)), ex::into_variant(ex::just('c'))));
static_assert(std::is_same_v<std::remove_cvref_t<Transformed>, Expected>);
template <class S>
concept transformable = requires(S&& s) { ex::when_all_with_variant.transform_sender(ex::set_value, std::forward<S>(s), ex::env<>()); };
static_assert(transformable<decltype(ex::when_all_with_variant(ex::just(1)))>);
static_assert(!transformable<decltype(ex::when_all(ex::just(1)))>);
static_assert(!transformable<decltype(ex::just(1))>);
