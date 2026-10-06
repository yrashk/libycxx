// execution::task customized through its Environment parameter:
//   [task.class]/2.1, [task.promise]/13: a coroutine without allocator_arg allocates its frame
//     with allocator_type(), here Environment::allocator_type; /19: and deallocates it with it;
//   [task.class]/2.3, [task.state]/5: with stop_source_type = stop_source, get_stop_token in the
//     task is a stop_token: the receiver's own when it has that type, else one of the task's
//     source that reports the receiver token's stop_requested() and stop_possible();
//   [task.state]/1, /2.3-2.4: own-env-t is Environment::env_type<decltype(get_env(rcvr))>,
//     constructed from get_env(rcvr), and the Environment object is constructed from it;
//     [task.promise]/12.4: forwarding queries are answered by that Environment object;
//   [task.state]/4.3: without get_start_scheduler in the receiver's environment, the start
//     scheduler is start_scheduler_type() (here inline_scheduler).
// REQUIRES: exceptions
#include <execution>
#include <cstddef>
#include <memory>
#include <new>
#include <stop_token>
#include <type_traits>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

int allocs = 0, deallocs = 0;
template <class T>
struct counting_alloc {
  using value_type = T;
  counting_alloc() = default;
  template <class U>
  counting_alloc(const counting_alloc<U>&) noexcept {}
  T* allocate(std::size_t n) {
    ++allocs;
    return static_cast<T*>(::operator new(n * sizeof(T)));
  }
  void deallocate(T* p, std::size_t) noexcept {
    ++deallocs;
    ::operator delete(p);
  }
  template <class U>
  bool operator==(const counting_alloc<U>&) const noexcept {
    return true;
  }
};

struct alloc_env {
  using start_scheduler_type = ex::inline_scheduler;
  using allocator_type = counting_alloc<std::byte>;
};
static_assert(std::is_same_v<ex::task<int, alloc_env>::allocator_type, counting_alloc<std::byte>>);

ex::task<int, alloc_env> add(int a, int b) { co_return a + b; }

struct stop_env {
  using start_scheduler_type = ex::inline_scheduler;
  using stop_source_type = std::stop_source;
};
static_assert(std::is_same_v<ex::task<void, stop_env>::stop_token_type, std::stop_token>);

ex::task<bool, stop_env> sees_stop() {
  auto tok = co_await ex::read_env(std::get_stop_token);
  static_assert(std::is_same_v<decltype(tok), std::stop_token>);
  co_return tok.stop_requested();
}
ex::task<bool, stop_env> sees_possible() {
  auto tok = co_await ex::read_env(std::get_stop_token);
  co_return tok.stop_possible();
}
std::stop_token captured;
ex::task<void, stop_env> capture_token() { captured = co_await ex::read_env(std::get_stop_token); }

// own-env-t and the Environment object.
struct q_t {
  template <class Env>
  constexpr auto operator()(const Env& e) const noexcept -> decltype(e.query(*this)) {
    return e.query(*this);
  }
};
inline constexpr q_t q{};
struct fq_t : std::forwarding_query_t {
  template <class Env>
  constexpr auto operator()(const Env& e) const noexcept -> decltype(e.query(*this)) {
    return e.query(*this);
  }
};
inline constexpr fq_t fq{};
int own_env_made = 0;
struct custom_env {
  using start_scheduler_type = ex::inline_scheduler;
  template <class E>
  struct env_type {
    int v;
    explicit env_type(const E& e) : v(q(e)) { ++own_env_made; }
  };
  int v;
  template <class E>
  explicit custom_env(const env_type<E>& o) : v(o.v * 10) {}
  int query(fq_t) const noexcept { return v; }
};
ex::task<int, custom_env> read_fq() { co_return co_await ex::read_env(fq); }

int main() {
  // [task.promise]/13
  {
    record<std::exception_ptr, int> rec;
    {
      auto op = ex::connect(add(2, 3), receiver_for(rec));
      CHECK(allocs == 1 && deallocs == 0);
      ex::start(op);
    }
    CHECK(rec.how == done::value && std::get<0>(*rec.values) == 5);
    CHECK(allocs == 1 && deallocs == 1);
  }
  // [task.state]/5: the receiver's token is an inplace_stop_token, not a stop_token.
  {
    std::inplace_stop_source src;
    using Env = ex::env<ex::prop<std::get_stop_token_t, std::inplace_stop_token>>;
    record<std::exception_ptr, bool> r1, r2;
    run(sees_stop(), receiver_for(r1, Env{ex::prop(std::get_stop_token, src.get_token())}));
    CHECK(r1.how == done::value && std::get<0>(*r1.values) == false);
    run(sees_possible(), receiver_for(r2, Env{ex::prop(std::get_stop_token, src.get_token())}));
    CHECK(r2.how == done::value && std::get<0>(*r2.values) == true);
    src.request_stop();
    record<std::exception_ptr, bool> r3;
    run(sees_stop(), receiver_for(r3, Env{ex::prop(std::get_stop_token, src.get_token())}));
    CHECK(r3.how == done::value && std::get<0>(*r3.values) == true);
    // An env<> receiver's never_stop_token: not possible.
    record<std::exception_ptr, bool> r4;
    run(sees_possible(), receiver_for(r4));
    CHECK(r4.how == done::value && std::get<0>(*r4.values) == false);
  }
  // ... and it is the receiver's own stop_token when that has the type.
  {
    std::stop_source ss;
    using Env = ex::env<ex::prop<std::get_stop_token_t, std::stop_token>>;
    record<std::exception_ptr> rec;
    run(capture_token(), receiver_for(rec, Env{ex::prop(std::get_stop_token, ss.get_token())}));
    CHECK(rec.how == done::value && captured == ss.get_token());
  }
  // [task.state]/1, /2.3-2.4, [task.promise]/12.4
  {
    record<std::exception_ptr, int> rec;
    run(read_fq(), receiver_for(rec, ex::env{ex::prop(q, 4)}));
    CHECK(rec.how == done::value && std::get<0>(*rec.values) == 40 && own_env_made == 1);
  }
  return 0;
}
