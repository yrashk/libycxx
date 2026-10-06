// The concepts and type utilities of [exec]:
//   [exec.recv.concepts]: receiver needs receiver_concept derived from receiver_tag, get_env,
//     move construction (nothrow) and construction from the argument.
//   [exec.opstate.general]: operation_state needs operation_state_concept and start(o).
//   [exec.snd.concepts]: sender, sender_in, dependent_sender; a sender's completion signatures
//     come from a static member function template get_completion_signatures<Sndr, Env...>().
//   [exec.getcomplsigs]/3.4: without an environment, a sender whose signatures depend on it is
//     dependent (it throws dependent_sender_error).
//   [exec.cmplsig]: completion_signatures; value_types_of_t, error_types_of_t, sends_stopped.
//   [exec.sched]: scheduler.
//   [exec.snd.concepts]/6: tag_of_t; a library sender is usable in a structured binding
//     ([exec.snd.expos]/45).
//   [exec.adapt.obj]: a user's sender_adaptor_closure composes with the library's.
// REQUIRES: exceptions
// (a dependent user sender throws dependent_sender_error)
#include <execution>
#include <exception>
#include <functional>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>

namespace ex = std::execution;

struct rcvr {
  using receiver_concept = ex::receiver_tag;
  void set_value(int) && noexcept {}
  void set_error(std::exception_ptr) && noexcept {}
  void set_stopped() && noexcept {}
};
static_assert(ex::receiver<rcvr>);
static_assert(ex::receiver<rcvr&>);
static_assert(ex::receiver<const rcvr&>);

struct not_tagged {
  void set_value() && noexcept {}
};
static_assert(!ex::receiver<not_tagged>);
struct throwing_move {
  using receiver_concept = ex::receiver_tag;
  throwing_move() = default;
  throwing_move(throwing_move&&) noexcept(false) {}
};
static_assert(!ex::receiver<throwing_move>);
struct derived_tag : ex::receiver_tag {};
struct derived_rcvr {
  using receiver_concept = derived_tag;
};
static_assert(ex::receiver<derived_rcvr>);

struct op {
  using operation_state_concept = ex::operation_state_tag;
  void start() & noexcept {}
};
static_assert(ex::operation_state<op>);
struct op_no_tag {
  void start() & noexcept {}
};
static_assert(!ex::operation_state<op_no_tag>);
// start is ill-formed on an rvalue ([exec.opstate.start]/1).
static_assert(!std::is_invocable_v<ex::start_t, op>);
static_assert(std::is_invocable_v<ex::start_t, op&>);
// set_value is ill-formed on an lvalue receiver ([exec.set.value]).
static_assert(!std::is_invocable_v<ex::set_value_t, rcvr&, int>);
static_assert(std::is_invocable_v<ex::set_value_t, rcvr, int>);
static_assert(!std::is_invocable_v<ex::set_value_t, rcvr, std::string>);
static_assert(std::is_invocable_v<ex::set_error_t, rcvr, std::exception_ptr>);
static_assert(!std::is_invocable_v<ex::set_stopped_t, const rcvr>);

struct sndr {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>();
  }
};
static_assert(ex::sender<sndr>);
static_assert(ex::sender_in<sndr>);
static_assert(ex::sender_in<sndr, ex::env<>>);
static_assert(!ex::dependent_sender<sndr>);
static_assert(std::is_same_v<ex::completion_signatures_of_t<sndr>,
                             ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>>);
static_assert(std::is_same_v<ex::value_types_of_t<sndr>, std::variant<std::tuple<int>>>);
static_assert(std::is_same_v<ex::error_types_of_t<sndr>, std::variant<std::exception_ptr>>);
static_assert(ex::sends_stopped<sndr>);
static_assert(std::is_same_v<ex::value_types_of_t<sndr, ex::env<>, std::tuple, std::type_identity_t>, std::tuple<int>>);

// Several value completions, duplicates removed in the variant.
struct multi {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int, const char*), ex::set_value_t(), ex::set_value_t(int&, const char*)>();
  }
};
static_assert(std::is_same_v<ex::value_types_of_t<multi>, std::variant<std::tuple<int, const char*>, std::tuple<>>>);
static_assert(std::is_same_v<ex::error_types_of_t<multi, ex::env<>, std::variant>, std::variant<>>);
static_assert(!ex::sends_stopped<multi>);

// A dependent sender ([exec.getcomplsigs]/3.4).
struct dep {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    if constexpr (sizeof...(Env) == 0)
      throw ex::dependent_sender_error();
    return ex::completion_signatures<ex::set_value_t()>();
  }
};
static_assert(ex::sender<dep>);
static_assert(ex::dependent_sender<dep>);
static_assert(!ex::sender_in<dep>);
static_assert(ex::sender_in<dep, ex::env<>>);

// Not senders.
static_assert(!ex::sender<int>);
struct no_tag {};
static_assert(!ex::sender<no_tag>);

// read_env is dependent; just is not.
static_assert(ex::dependent_sender<decltype(ex::read_env(std::get_stop_token))>);
static_assert(!ex::dependent_sender<decltype(ex::just())>);
static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(ex::read_env(std::get_stop_token)), ex::env<>>,
                             ex::completion_signatures<ex::set_value_t(std::never_stop_token)>>);

// completion_signatures: only completion signatures are accepted.
template <class... Ts>
concept valid_cs = requires { typename ex::completion_signatures<Ts...>; };
static_assert(valid_cs<ex::set_value_t(), ex::set_value_t(int&, int&&), ex::set_error_t(int), ex::set_stopped_t()>);
static_assert(!valid_cs<int>);
static_assert(!valid_cs<ex::set_stopped_t(int)>);
static_assert(!valid_cs<ex::set_error_t()>);
static_assert(!valid_cs<ex::set_error_t(int, int)>);
static_assert(!valid_cs<void(int)>);

// scheduler
static_assert(ex::scheduler<ex::inline_scheduler>);
static_assert(ex::scheduler<decltype(std::declval<ex::run_loop&>().get_scheduler())>);
static_assert(ex::scheduler<ex::parallel_scheduler>);
static_assert(!ex::scheduler<int>);
static_assert(std::is_same_v<ex::completion_signatures_of_t<ex::schedule_result_t<ex::inline_scheduler>>,
                             ex::completion_signatures<ex::set_value_t()>>);

// tag_of_t and structured bindings of a library sender.
using then_sndr = decltype(ex::then(ex::just(1), [](int x) { return x; }));
static_assert(std::is_same_v<ex::tag_of_t<then_sndr>, ex::then_t>);
static_assert(std::is_same_v<ex::tag_of_t<decltype(ex::just())>, ex::just_t>);
static_assert(std::is_same_v<ex::tag_of_t<decltype(ex::when_all(ex::just(), ex::just()))>, ex::when_all_t>);
constexpr bool bindings() {
  auto s = ex::just(1, 2);
  auto& [tag, data] = s;
  (void)tag;
  return std::is_same_v<std::remove_cvref_t<decltype(tag)>, ex::just_t> && data.template get<1>() == 2;
}
static_assert(bindings());
static_assert(std::tuple_size_v<then_sndr> == 3);

// Pipes: a user closure, library closures, and their composition.
struct twice_t : ex::sender_adaptor_closure<twice_t> {
  template <ex::sender S>
  auto operator()(S&& s) const {
    return ex::then(std::forward<S>(s), [](int x) { return 2 * x; });
  }
};
inline constexpr twice_t twice{};
using piped = decltype(ex::just(3) | twice | ex::then([](int x) { return x + 1; }));
static_assert(std::is_same_v<ex::value_types_of_t<piped>, std::variant<std::tuple<int>>>);
using composed = decltype(twice | ex::then([](int x) { return x + 1; }));
static_assert(!ex::sender<composed>);
static_assert(ex::sender<decltype(ex::just(3) | (twice | ex::upon_stopped([] { return 0; })))>);
// A sender is not a closure; neither is a closure a sender.
static_assert(!std::is_invocable_v<std::bit_or<>, decltype(ex::just()), decltype(ex::just())>);

// A sender with a member type completion_signatures, as [exec.cmplsig]'s example writes it.
struct example_sender {
  using sender_concept = ex::sender_tag;
  using completion_signatures = ex::completion_signatures<ex::set_value_t(), ex::set_stopped_t()>;
};
static_assert(ex::sender_in<example_sender>);

int main() { return 0; }
