// [exec.adapt.obj]:
//   /1: c | d is a closure e with e(sndr) equivalent to d(c(sndr)); its state entities are
//     decay-copies of c and d made when e is formed, so changing the originals afterwards does
//     not affect e; c | d is ill-formed when they cannot be made (a move-only closure as an
//     lvalue), and well-formed from rvalues;
//   /2: a type derived from sender_adaptor_closure<T> that is also a sender is not a closure;
//   /5: adaptor(args...) binds decay-copies of args; the closure's call is adaptor(sndr,
//     bound_args...) as a perfect forwarding call wrapper: called as an rvalue it passes the
//     bound arguments as rvalues (a move-only function works), as an lvalue as lvalues;
//     adaptor(args...) is ill-formed if a bound argument cannot be initialized (a move-only
//     lvalue).
#include <execution>
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

// A function object whose value is fixed when it is copied.
struct add {
  int* n;
  int operator()(int x) const noexcept { return x + *n; }
};
struct add_val {
  int n;
  int operator()(int x) const noexcept { return x + n; }
};

// A closure type that is also a sender: not a pipeable closure (/2).
struct both : ex::sender_adaptor_closure<both> {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t()>();
  }
  template <class S>
  auto operator()(S&& s) const {
    return std::forward<S>(s);
  }
};

template <class S, class C>
concept pipeable = requires(S&& s, C&& c) { std::forward<S>(s) | std::forward<C>(c); };
template <class C, class D>
concept composable = requires(C&& c, D&& d) { std::forward<C>(c) | std::forward<D>(d); };

int main() {
  // /1: composition, and its copies.
  {
    add_val a{1};
    auto c = ex::then(a);
    auto d = ex::then([](int x) noexcept { return x * 10; });
    auto e = c | d;
    c = ex::then(add_val{100}); // the composition keeps its own copy
    record<int, int> r1, r2;
    run(ex::just(2) | e, receiver_for(r1));
    run(e(ex::just(2)), receiver_for(r2));
    CHECK(std::get<0>(*r1.values) == 30 && std::get<0>(*r2.values) == 30);
    // ... associates with the senders as d(c(sndr)).
    record<int, int> r3;
    run(ex::just(2) | (ex::then(add_val{1}) | ex::then(add_val{2})) | ex::then(add_val{3}), receiver_for(r3));
    CHECK(std::get<0>(*r3.values) == 8);
  }
  // /1: move-only closures compose from rvalues only.
  {
    auto mo = [p = std::make_unique<int>(5)](int x) { return x + *p; };
    using MC = decltype(ex::then(std::move(mo)));
    using DC = decltype(ex::then(add_val{0}));
    static_assert(!std::is_copy_constructible_v<MC>);
    static_assert(composable<MC, DC>);
    static_assert(!composable<MC&, DC>);
    static_assert(composable<DC&, DC&>);
  }
  // /2
  static_assert(!pipeable<decltype(ex::just()), both>);
  static_assert(!pipeable<decltype(ex::just()), both&>);
  // /5: bound arguments are decay-copies...
  {
    int n = 1;
    add a{&n};
    auto c = ex::then(a);
    a.n = nullptr; // the closure has its own copy
    n = 7;
    record<int, int> rec;
    run(ex::just(1) | c, receiver_for(rec));
    CHECK(std::get<0>(*rec.values) == 8);
  }
  // ... passed as rvalues by an rvalue closure (a move-only function works), as lvalues by an
  // lvalue one (then needs a movable-value, so a move-only lvalue is rejected).
  {
    auto mo = [p = std::make_unique<int>(5)](int x) { return x + *p; };
    auto c = ex::then(std::move(mo));
    static_assert(pipeable<decltype(ex::just(1)), decltype(c)&&>);
    static_assert(!pipeable<decltype(ex::just(1)), decltype(c)&>);
    record<std::exception_ptr, int> rec;
    run(ex::just(1) | std::move(c), receiver_for(rec));
    CHECK(rec.how == done::value && std::get<0>(*rec.values) == 6);
  }
  // ... and adaptor(args...) is ill-formed when a bound argument cannot be initialized.
  {
    auto mo = [p = std::make_unique<int>(5)](int x) { return x + *p; };
    static_assert(!std::is_invocable_v<ex::then_t, decltype(mo)&>);
    static_assert(std::is_invocable_v<ex::then_t, decltype(mo)&&>);
    static_assert(!std::is_invocable_v<ex::bulk_t, const ex::sequenced_policy&, int, decltype(mo)&>);
  }
  return 0;
}
