// Support for the execution tests: a receiver that records how its operation completed, with an
// environment of the test's choice. Written from [exec.recv] only.
#pragma once

#include <exception>
#include <execution>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace exec_test {
namespace ex = std::execution;

enum class done { none, value, error, stopped };

// Records the completion of an operation whose value completions send Vs... and whose error
// completions send E (any other error type is recorded as an error without its datum).
template <class E, class... Vs>
struct record {
  done how = done::none;
  int calls = 0;
  std::optional<std::tuple<Vs...>> values;
  std::optional<E> error;
};

template <class Rec, class Env = ex::env<>>
struct recording_receiver {
  using receiver_concept = ex::receiver_tag;
  Rec* rec;
  Env env{};

  template <class... As>
    requires requires(As&&... as) { std::tuple(static_cast<As&&>(as)...); }
  void set_value(As&&... as) && noexcept {
    rec->how = done::value;
    ++rec->calls;
    if constexpr (requires { rec->values.emplace(static_cast<As&&>(as)...); })
      rec->values.emplace(static_cast<As&&>(as)...);
  }
  template <class Err>
  void set_error(Err&& e) && noexcept {
    rec->how = done::error;
    ++rec->calls;
    if constexpr (requires { rec->error.emplace(static_cast<Err&&>(e)); })
      rec->error.emplace(static_cast<Err&&>(e));
  }
  void set_stopped() && noexcept {
    rec->how = done::stopped;
    ++rec->calls;
  }
  const Env& get_env() const noexcept { return env; }
};

template <class Rec, class Env = ex::env<>>
recording_receiver<Rec, Env> receiver_for(Rec& r, Env env = {}) {
  return {&r, std::move(env)};
}

// connect + start, for an operation that completes before start returns.
template <class Sndr, class Rcvr>
void run(Sndr&& s, Rcvr r) {
  auto op = ex::connect(std::forward<Sndr>(s), std::move(r));
  ex::start(op);
}

template <class F>
bool throws_value(F f, int v) {
  try {
    f();
  } catch (int e) {
    return e == v;
  } catch (...) {
  }
  return false;
}
// same_sigs<A, B>: A and B are completion_signatures specializations with the same set of
// signatures ([exec.async.ops]: a set; the order of the template arguments is not specified).
template <class Sig, class Sigs>
inline constexpr bool has_sig = false;
template <class Sig, class... Sigs>
inline constexpr bool has_sig<Sig, ex::completion_signatures<Sigs...>> = (std::is_same_v<Sig, Sigs> || ...);
template <class A, class B>
inline constexpr bool sigs_subset = false;
template <class... As, class B>
inline constexpr bool sigs_subset<ex::completion_signatures<As...>, B> = (has_sig<As, B> && ...);
template <class A, class B>
inline constexpr bool same_sigs = sigs_subset<A, B> && sigs_subset<B, A>;

} // namespace exec_test
