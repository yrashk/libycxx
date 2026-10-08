// [exec.then]/4: complete evaluates TRY-SET-VALUE(rcvr, invoke(std::move(fn), args...)):
//   - fn is invoked as an rvalue (an f whose call operator is &&-qualified works, one that is
//     only &-qualified is not invocable: /5's check-types tests invocable<F, Ts...> with F an
//     rvalue, so the sender has no completion signatures);
//   - the datums are forwarded as the child sent them (an f taking int&& accepts just(1)'s);
//   - SET-VALUE(rcvr, expr) ([exec.snd.expos]/11) sends f's result as the expression it is: a
//     reference to an object is sent as that lvalue (set_value_t(int&), the same object), a
//     prvalue as set_value_t(T), and a void result as set_value_t();
//   /5: upon_error's f must accept every error type of the child; each gives a value completion.
// REQUIRES: exceptions
#include <execution>
#include <exception>
#include <string>
#include <type_traits>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

int global = 5;

struct rvalue_only {
  int operator()(int x) && noexcept { return x + 1; }
};
struct lvalue_only {
  int operator()(int x) & noexcept { return x + 1; }
};

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

struct takes_int_or_string {
  int operator()(int) const noexcept { return 1; }
  std::string operator()(const std::string&) const noexcept { return "s"; }
};
struct takes_int_only {
  int operator()(int) const noexcept { return 1; }
};

int main() {
  // rvalue invocation
  {
    record<int, int> rec;
    run(ex::just(1) | ex::then(rvalue_only()), receiver_for(rec));
    CHECK(rec.how == done::value && std::get<0>(*rec.values) == 2);
    using D = dep_sender<ex::set_value_t(int)>;
    static_assert(ex::sender_in<decltype(ex::then(D(), rvalue_only())), ex::env<>>);
    static_assert(!ex::sender_in<decltype(ex::then(D(), lvalue_only())), ex::env<>>);
  }
  // datums forwarded as sent
  {
    record<int, int> rec;
    run(ex::just(4) | ex::then([](int&& x) noexcept { return x * 2; }), receiver_for(rec));
    CHECK(std::get<0>(*rec.values) == 8);
  }
  // the result's value category
  {
    auto ref_f = []() noexcept -> int& { return global; };
    using L = decltype(ex::just() | ex::then(ref_f));
    static_assert(std::is_same_v<ex::completion_signatures_of_t<L>, ex::completion_signatures<ex::set_value_t(int&)>>);
    int* seen = nullptr;
    struct addr_rcvr {
      using receiver_concept = ex::receiver_tag;
      int** seen;
      void set_value(int& r) && noexcept { *seen = &r; }
    };
    run(ex::just() | ex::then(ref_f), addr_rcvr{&seen});
    CHECK(seen == &global);
    using P = decltype(ex::just() | ex::then([]() noexcept { return global; }));
    static_assert(std::is_same_v<ex::completion_signatures_of_t<P>, ex::completion_signatures<ex::set_value_t(int)>>);
    using V = decltype(ex::just() | ex::then([]() noexcept {}));
    static_assert(std::is_same_v<ex::completion_signatures_of_t<V>, ex::completion_signatures<ex::set_value_t()>>);
    using C = decltype(ex::just() | ex::then([]() noexcept -> const std::string& { static const std::string s = "c"; return s; }));
    static_assert(std::is_same_v<ex::completion_signatures_of_t<C>, ex::completion_signatures<ex::set_value_t(const std::string&)>>);
  }
  // upon_error over several error types
  {
    using D = dep_sender<ex::set_value_t(), ex::set_error_t(int), ex::set_error_t(std::string)>;
    using U = decltype(ex::upon_error(D(), takes_int_or_string()));
    static_assert(exec_test::same_sigs<ex::completion_signatures_of_t<U, ex::env<>>,
                                       ex::completion_signatures<ex::set_value_t(), ex::set_value_t(int), ex::set_value_t(std::string)>>);
    static_assert(!ex::sender_in<decltype(ex::upon_error(D(), takes_int_only())), ex::env<>>);
    // upon_stopped's f takes nothing.
    using S = dep_sender<ex::set_value_t(int), ex::set_stopped_t()>;
    static_assert(exec_test::same_sigs<ex::completion_signatures_of_t<decltype(ex::upon_stopped(S(), []() noexcept { return 0; })), ex::env<>>,
                                       ex::completion_signatures<ex::set_value_t(int)>>);
    static_assert(!ex::sender_in<decltype(ex::upon_stopped(S(), [](int) noexcept { return 0; })), ex::env<>>);
  }
  return 0;
}
