// into_variant ([exec.into.variant]), stopped_as_optional ([exec.stopped.opt]),
// stopped_as_error ([exec.stopped.err]):
//   into_variant(sndr) sends one value: value_types_of_t<sndr, env> holding the decayed values of
//   whichever value completion happened; errors and stopped pass through.
//   stopped_as_optional(sndr), for a sender with one value datum type V, sends optional<V>: the
//   value, or nullopt for stopped; it never completes with stopped.
//   stopped_as_error(sndr, err) turns stopped into set_error(err).
//   All three are pipeable; stopped_as_optional and into_variant are closures themselves.
#include <execution>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;
using namespace exec_test;

struct two_values {
  using sender_concept = ex::sender_tag;
  bool first;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_value_t(std::string, char)>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    bool first;
    void start() & noexcept {
      if (first)
        ex::set_value(std::move(r), 1);
      else
        ex::set_value(std::move(r), std::string("s"), 'c');
    }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r), first};
  }
};

// Completes with a value or with stopped.
struct maybe_stopped {
  using sender_concept = ex::sender_tag;
  bool stop;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_stopped_t()>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    bool stop;
    void start() & noexcept {
      if (stop)
        ex::set_stopped(std::move(r));
      else
        ex::set_value(std::move(r), 3);
    }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r), stop};
  }
};

int main() {
  // into_variant
  {
    using V = std::variant<std::tuple<int>, std::tuple<std::string, char>>;
    auto s = ex::into_variant(two_values{false});
    static_assert(std::is_same_v<ex::value_types_of_t<decltype(s), ex::env<>, std::tuple, std::type_identity_t>, std::tuple<V>>);
    auto r = tt::sync_wait(std::move(s));
    CHECK(r && std::get<0>(*r).index() == 1 && std::get<0>(std::get<1>(std::get<0>(*r))) == "s");
    auto r1 = tt::sync_wait(two_values{true} | ex::into_variant);
    CHECK(r1 && std::get<0>(std::get<0>(std::get<0>(*r1))) == 1);
    record<int, std::variant<std::tuple<int>>> rec;
    run(ex::into_variant(ex::just_error(4)), receiver_for(rec));
    CHECK(rec.how == done::error && *rec.error == 4);
  }
  // sync_wait_with_variant is sync_wait of into_variant.
  {
    auto r = tt::sync_wait_with_variant(two_values{true});
    static_assert(std::is_same_v<decltype(r), std::optional<std::variant<std::tuple<int>, std::tuple<std::string, char>>>>);
    CHECK(r && r->index() == 0);
  }
  // stopped_as_optional
  {
    auto r = tt::sync_wait(ex::just(4) | ex::stopped_as_optional);
    static_assert(std::is_same_v<decltype(r), std::optional<std::tuple<std::optional<int>>>>);
    CHECK(r && std::get<0>(*r) == std::optional<int>(4));
    auto r2 = tt::sync_wait(ex::stopped_as_optional(maybe_stopped{true}));
    CHECK(r2 && !std::get<0>(*r2).has_value());
    using S = decltype(ex::stopped_as_optional(ex::just(std::string())));
    static_assert(!ex::sends_stopped<S>);
  }
  // stopped_as_error
  {
    record<int> rec;
    run(ex::just_stopped() | ex::stopped_as_error(42), receiver_for(rec));
    CHECK(rec.how == done::error && *rec.error == 42);
    record<int, int> rec2;
    run(ex::just(1) | ex::stopped_as_error(42), receiver_for(rec2));
    CHECK(rec2.how == done::value && std::get<0>(*rec2.values) == 1);
    using S = decltype(ex::just_stopped() | ex::stopped_as_error(std::string()));
    static_assert(std::is_same_v<ex::completion_signatures_of_t<S, ex::env<>>, ex::completion_signatures<ex::set_error_t(std::string)>>);
  }
  return 0;
}
