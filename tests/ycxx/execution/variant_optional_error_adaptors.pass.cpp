// [exec.into.variant]/5-6: into_variant(sndr) completes with one value: a
// value_types_of_t<child, env> variant holding the decayed tuple of the child's value datums;
// errors and stopped pass through.
// [exec.stopped.opt]/3-4: stopped_as_optional(sndr) maps a value v to optional<V>(in_place, v)
// and stopped to a disengaged optional<V>; errors pass through; it requires a single value
// completion whose type is not void (else check-types throws).
// [exec.stopped.err]/2-3: stopped_as_error(sndr, err) maps stopped to set_error(err); values
// pass through; err must be a movable-value.
#include <execution>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using exec_test::done;

// A sender with three value completions, chosen at run time.
struct multi {
  using sender_concept = ex::sender_tag;
  int which;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_value_t(int, const std::string&), ex::set_value_t(),
                                     ex::set_error_t(long), ex::set_stopped_t()>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    int which;
    std::string s{"text"};
    void start() & noexcept {
      switch (which) {
        case 0: ex::set_value(std::move(r), 1); break;
        case 1: ex::set_value(std::move(r), 2, s); break;
        case 2: ex::set_value(std::move(r)); break;
        case 3: ex::set_error(std::move(r), 3L); break;
        default: ex::set_stopped(std::move(r));
      }
    }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r), which};
  }
};

// One int value, an int error or stopped.
struct int_or {
  using sender_concept = ex::sender_tag;
  int which;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(int), ex::set_stopped_t()>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    int which;
    void start() & noexcept {
      if (which == 0) ex::set_value(std::move(r), 1);
      else if (which == 1) ex::set_error(std::move(r), 4);
      else ex::set_stopped(std::move(r));
    }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r), which};
  }
};

using V = std::variant<std::tuple<int>, std::tuple<int, std::string>, std::tuple<>>;
static_assert(std::is_same_v<ex::value_types_of_t<multi, ex::env<>>, V>);

template <class S>
concept optional_ok = ex::sender_in<decltype(ex::stopped_as_optional(std::declval<S>())), ex::env<>>;

int main() {
  // into_variant
  for (int w = 0; w < 5; ++w) {
    exec_test::record<long, V> rec;
    exec_test::run(ex::into_variant(multi{w}), exec_test::receiver_for(rec));
    if (w < 3) {
      CHECK(rec.how == done::value);
      const V& v = std::get<0>(*rec.values);
      CHECK(static_cast<int>(v.index()) == w);
      if (w == 0) CHECK(std::get<0>(v) == std::tuple<int>(1));
      if (w == 1) CHECK(std::get<1>(v) == std::make_tuple(2, std::string("text")));
    } else if (w == 3) {
      CHECK(rec.how == done::error && *rec.error == 3L);
    } else {
      CHECK(rec.how == done::stopped);
    }
  }
  {
    auto r = std::this_thread::sync_wait(ex::just(1, 'c') | ex::into_variant);
    static_assert(std::is_same_v<decltype(r), std::optional<std::tuple<std::variant<std::tuple<int, char>>>>>);
    CHECK(r && std::get<0>(std::get<0>(*r)) == std::make_tuple(1, 'c'));
  }

  // stopped_as_optional
  // (For a non-dependent child without one non-void value completion, stopped_as_optional(sndr)
  // is ill-formed: [exec.snd.expos]/24.4.)
  static_assert(optional_ok<decltype(ex::just(1))>);
  {
    auto r = std::this_thread::sync_wait(ex::just(std::string("v")) | ex::stopped_as_optional);
    CHECK(r && std::get<0>(*r) == std::optional<std::string>("v"));
    auto s = std::this_thread::sync_wait(int_or{2} | ex::stopped_as_optional);
    CHECK(s && !std::get<0>(*s).has_value());
    auto v = std::this_thread::sync_wait(int_or{0} | ex::stopped_as_optional);
    CHECK(v && std::get<0>(*v) == std::optional<int>(1));
    exec_test::record<int, std::optional<int>> rec;
    exec_test::run(int_or{1} | ex::stopped_as_optional, exec_test::receiver_for(rec));
    CHECK(rec.how == done::error && *rec.error == 4);
    using SO = decltype(int_or{0} | ex::stopped_as_optional);
    static_assert(std::is_same_v<ex::value_types_of_t<SO, ex::env<>>, std::variant<std::tuple<std::optional<int>>>>);
    static_assert(!ex::sends_stopped<SO, ex::env<>>);
  }

  // stopped_as_error
  {
    exec_test::record<std::string, int> rec;  // (the int error is recorded without its datum)
    exec_test::run(int_or{2} | ex::stopped_as_error(std::string("cancelled")),
                   exec_test::receiver_for(rec));
    CHECK(rec.how == done::error && *rec.error == "cancelled");
    exec_test::record<std::string, int> rec2;
    exec_test::run(ex::just(8) | ex::stopped_as_error(std::string("x")), exec_test::receiver_for(rec2));
    CHECK(rec2.how == done::value && std::get<0>(*rec2.values) == 8);
    using S = decltype(ex::just_stopped() | ex::stopped_as_error(5));
    static_assert(std::is_same_v<ex::completion_signatures_of_t<S, ex::env<>>, ex::completion_signatures<ex::set_error_t(int)>>);
  }
  return 0;
}
