// [exec.when.all]/12, /14, /17: when_all decay-copies its children's result datums into its
// state. copy-fail is exception_ptr exactly when one of those decay-copies can throw (/12), and
// is then among the errors (/14):
//   - a value whose decay-copy throws: TRY-EMPLACE-VALUE completes as set_error with
//     current_exception() (/17), which requests a stop of the other children;
//   - an error whose decay-copy throws: TRY-EMPLACE-ERROR stores current_exception() (/17).
// When no decay-copy can throw, no set_error(exception_ptr) is potentially evaluated, so it is
// not among the completion signatures ([exec.snd.expos]/47), even for datums sent by lvalue
// reference; [exec.into.variant]/6 likewise (TRY-SET-VALUE of a decayed-tuple that cannot
// throw), so such senders connect to a receiver that has no set_error(exception_ptr).
// REQUIRES: exceptions
#include <execution>
#include <exception>
#include <stop_token>
#include <tuple>
#include <type_traits>
#include <variant>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

bool copy_throws = false;
struct thrower {
  int v = 0;
  thrower() = default;
  thrower(int x) : v(x) {}
  thrower(const thrower& o) : v(o.v) {
    if (copy_throws)
      throw 42;
  }
  thrower(thrower&&) noexcept = default;
};

// Completes with Tag and an lvalue of *obj.
template <class Tag, class T>
struct ref_sender {
  using sender_concept = ex::sender_tag;
  T* obj;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<Tag(T&)>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    T* obj;
    void start() & noexcept { Tag()(std::move(r), *obj); }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r), obj};
  }
};

// Completes with set_value(), recording whether a stop was requested when it started.
struct probe {
  using sender_concept = ex::sender_tag;
  bool* stop_seen;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t()>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    bool* stop_seen;
    void start() & noexcept {
      *stop_seen = std::get_stop_token(ex::get_env(r)).stop_requested();
      ex::set_value(std::move(r));
    }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r), stop_seen};
  }
};

// A receiver of values only.
template <class T>
struct values_only {
  using receiver_concept = ex::receiver_tag;
  T* out;
  void set_value(T v) && noexcept { *out = std::move(v); }
};

bool holds_42(const std::exception_ptr& e) {
  try {
    std::rethrow_exception(e);
  } catch (int i) {
    return i == 42;
  } catch (...) {
  }
  return false;
}

int main() {
  thrower t{7};
  using VS = ref_sender<ex::set_value_t, thrower>;
  using ES = ref_sender<ex::set_error_t, thrower>;
  static_assert(same_sigs<ex::completion_signatures_of_t<decltype(ex::when_all(VS{&t}))>,
                          ex::completion_signatures<ex::set_value_t(thrower), ex::set_error_t(std::exception_ptr)>>);
  static_assert(same_sigs<ex::completion_signatures_of_t<decltype(ex::when_all(ES{&t}))>,
                          ex::completion_signatures<ex::set_error_t(thrower), ex::set_error_t(std::exception_ptr)>>);

  // A value copied without an exception.
  {
    record<std::exception_ptr, thrower> rec;
    run(ex::when_all(VS{&t}), receiver_for(rec));
    CHECK(rec.how == done::value && std::get<0>(*rec.values).v == 7);
  }
  // A value whose copy throws: set_error(exception_ptr); the next child sees a stop request.
  {
    copy_throws = true;
    bool stop_seen = false;
    record<std::exception_ptr, thrower> rec;
    run(ex::when_all(VS{&t}, probe{&stop_seen}), receiver_for(rec));
    copy_throws = false;
    CHECK(rec.how == done::error && rec.calls == 1 && holds_42(*rec.error));
    CHECK(stop_seen);
  }
  // An error whose copy throws: set_error(exception_ptr); a later error does not replace it.
  {
    copy_throws = true;
    record<std::exception_ptr> rec;
    run(ex::when_all(ES{&t}, ex::just_error(std::exception_ptr())), receiver_for(rec));
    copy_throws = false;
    CHECK(rec.how == done::error && rec.calls == 1 && holds_42(*rec.error));
  }
  // Datums whose decay-copies cannot throw, sent by reference: no exception_ptr error.
  {
    int i = 3;
    const double d = 2.5;
    using IS = ref_sender<ex::set_value_t, int>;
    using DS = ref_sender<ex::set_value_t, const double>;
    using W = decltype(ex::when_all(IS{&i}, DS{&d}));
    static_assert(std::is_same_v<ex::completion_signatures_of_t<W>, ex::completion_signatures<ex::set_value_t(int, double)>>);
    std::tuple<int, double> out{};
    struct two_values {
      using receiver_concept = ex::receiver_tag;
      std::tuple<int, double>* out;
      void set_value(int a, double b) && noexcept { *out = {a, b}; }
    };
    run(ex::when_all(IS{&i}, DS{&d}), two_values{&out});
    CHECK(std::get<0>(out) == 3 && std::get<1>(out) == 2.5);

    using V = decltype(ex::into_variant(IS{&i}));
    static_assert(std::is_same_v<ex::completion_signatures_of_t<V>,
                                 ex::completion_signatures<ex::set_value_t(std::variant<std::tuple<int>>)>>);
    std::variant<std::tuple<int>> vout;
    run(ex::into_variant(IS{&i}), values_only<std::variant<std::tuple<int>>>{&vout});
    CHECK(std::get<0>(std::get<0>(vout)) == 3);
  }
  return 0;
}
