// [exec.read.env]:
//   /3: start evaluates TRY-SET-VALUE(rcvr, query(get_env(rcvr))) ([exec.snd.expos]/11): the
//     result is sent as the expression the query gives (an lvalue of the environment's object
//     for a query returning a reference); an exception from the query is sent as
//     set_error(exception_ptr), which is a completion only if the query can throw
//     ([exec.snd.expos]/47);
//   /5: check-types throws when Q()(env) is ill-formed or void: no completion signatures;
//   read_env(q) depends on the environment ([exec.snd.general]/2): a dependent sender.
// REQUIRES: exceptions
#include <execution>
#include <exception>
#include <type_traits>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

struct holder {
  int value = 0;
};

// A query answered by reference, one that can throw, and one that returns void.
struct ref_q_t {
  template <class Env>
  constexpr auto operator()(const Env& e) const noexcept -> decltype(e.query(*this)) {
    return e.query(*this);
  }
};
inline constexpr ref_q_t ref_q{};
struct throwing_q_t {
  template <class Env>
  constexpr auto operator()(const Env& e) const -> decltype(e.query(*this)) {
    return e.query(*this);
  }
};
inline constexpr throwing_q_t throwing_q{};
struct void_q_t {
  template <class Env>
  constexpr void operator()(const Env&) const noexcept {}
};
inline constexpr void_q_t void_q{};

struct my_env {
  holder* h;
  const int& query(ref_q_t) const noexcept { return h->value; }
  int query(throwing_q_t) const {
    if (h->value < 0)
      throw h->value;
    return h->value;
  }
};

using EP = std::exception_ptr;

static_assert(ex::dependent_sender<decltype(ex::read_env(ref_q))>);
static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(ex::read_env(ref_q)), my_env>,
                             ex::completion_signatures<ex::set_value_t(const int&)>>);
static_assert(same_sigs<ex::completion_signatures_of_t<decltype(ex::read_env(throwing_q)), my_env>,
                        ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(EP)>>);
static_assert(!ex::sender_in<decltype(ex::read_env(void_q)), my_env>);
static_assert(!ex::sender_in<decltype(ex::read_env(ref_q)), ex::env<>>);

// Records the address of the datum it is sent.
struct addr_receiver {
  using receiver_concept = ex::receiver_tag;
  holder* h;
  const int** addr;
  void set_value(const int& v) && noexcept { *addr = &v; }
  my_env get_env() const noexcept { return {h}; }
};

int main() {
  holder h{3};
  // The datum is the environment's object itself.
  {
    const int* addr = nullptr;
    run(ex::read_env(ref_q), addr_receiver{&h, &addr});
    CHECK(addr == &h.value);
  }
  // A query that throws.
  {
    record<EP, int> rec;
    run(ex::read_env(throwing_q), receiver_for(rec, my_env{&h}));
    CHECK(rec.how == done::value && std::get<0>(*rec.values) == 3);
    holder bad{-4};
    record<EP, int> rec2;
    run(ex::read_env(throwing_q), receiver_for(rec2, my_env{&bad}));
    CHECK(rec2.how == done::error && rec2.calls == 1);
    CHECK(throws_value([&] { std::rethrow_exception(*rec2.error); }, -4));
  }
  return 0;
}
