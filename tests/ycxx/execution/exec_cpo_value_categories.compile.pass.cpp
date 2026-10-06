// The value-category rules of the completion functions, start and get_env:
//   [exec.set.value]/1, [exec.set.error]/1, [exec.set.stopped]/1: set_value(rcvr, vs...),
//     set_error(rcvr, err) and set_stopped(rcvr) are ill-formed if rcvr is an lvalue or an
//     rvalue of const type; otherwise they call the member, and their type is void (Mandates);
//   [exec.opstate.start]/1: start(op) is ill-formed if op is an rvalue;
//   [exec.get.env]/1.2: get_env(o) of an object without a get_env member is env<>{}; /1.1: the
//     member is called on AS-CONST(o) (a const member is found for a non-const object);
//   [exec.general]/5: the completion functions, start and get_env are noexcept expressions
//     (MANDATE-NOTHROW).
#include <execution>
#include <type_traits>
#include <utility>

namespace ex = std::execution;

struct rcvr {
  using receiver_concept = ex::receiver_tag;
  void set_value(int) && noexcept {}
  void set_error(int) && noexcept {}
  void set_stopped() && noexcept {}
};

struct op {
  using operation_state_concept = ex::operation_state_tag;
  void start() & noexcept {}
};

struct no_env {};
struct custom_env {
  int value() const noexcept { return 1; }
};
struct has_env {
  custom_env get_env() const noexcept { return {}; }
};

template <class R>
concept can_set_value = requires(R&& r) { ex::set_value(std::forward<R>(r), 1); };
template <class R>
concept can_set_error = requires(R&& r) { ex::set_error(std::forward<R>(r), 1); };
template <class R>
concept can_set_stopped = requires(R&& r) { ex::set_stopped(std::forward<R>(r)); };
template <class O>
concept can_start = requires(O&& o) { ex::start(std::forward<O>(o)); };

static_assert(can_set_value<rcvr>);
static_assert(!can_set_value<rcvr&>);
static_assert(!can_set_value<const rcvr&>);
static_assert(!can_set_value<const rcvr>);
static_assert(can_set_error<rcvr>);
static_assert(!can_set_error<rcvr&>);
static_assert(!can_set_error<const rcvr>);
static_assert(can_set_stopped<rcvr>);
static_assert(!can_set_stopped<rcvr&>);
static_assert(!can_set_stopped<const rcvr>);

static_assert(std::is_void_v<decltype(ex::set_value(std::declval<rcvr>(), 1))>);
static_assert(noexcept(ex::set_value(std::declval<rcvr>(), 1)));
static_assert(noexcept(ex::set_error(std::declval<rcvr>(), 1)));
static_assert(noexcept(ex::set_stopped(std::declval<rcvr>())));

static_assert(can_start<op&>);
static_assert(!can_start<op>);
static_assert(!can_start<op&&>);
static_assert(noexcept(ex::start(std::declval<op&>())));
static_assert(std::is_void_v<decltype(ex::start(std::declval<op&>()))>);

static_assert(std::is_same_v<decltype(ex::get_env(std::declval<no_env&>())), ex::env<>>);
static_assert(std::is_same_v<decltype(ex::get_env(std::declval<has_env&>())), custom_env>);
static_assert(std::is_same_v<decltype(ex::get_env(std::declval<const has_env&>())), custom_env>);
static_assert(noexcept(ex::get_env(std::declval<no_env&>())));
static_assert(noexcept(ex::get_env(std::declval<has_env&>())));

int main() { return 0; }
