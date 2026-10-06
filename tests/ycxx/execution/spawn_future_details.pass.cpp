// [exec.spawn.future]:
//   /8: when the token's try_associate fails (a closed scope) the sender is not started and the
//     future completes with set_stopped;
//   /19-20: the state is allocated with get_allocator(env) when env has one, else with the
//     allocator of the wrapped sender's attributes, else allocator<void>; it is freed when the
//     future's result has been consumed (/11.3 then destroy, /14);
//   /5.2, /6: when decay-copying a result datum can throw and does, the future completes with
//     set_error(exception_ptr) carrying that exception;
//   /21: spawn_future(sndr, token) is spawn_future(sndr, token, env<>()).
// REQUIRES: exceptions
#include <execution>
#include <cstddef>
#include <exception>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;
using exec_test::done;

int allocs = 0, deallocs = 0;
template <class T>
struct Alloc {
  using value_type = T;
  int id = 0;
  Alloc() = default;
  explicit Alloc(int i) : id(i) {}
  template <class U>
  Alloc(const Alloc<U>& o) noexcept : id(o.id) {}
  T* allocate(std::size_t n) {
    ++allocs;
    return std::allocator<T>().allocate(n);
  }
  void deallocate(T* p, std::size_t n) noexcept {
    ++deallocs;
    std::allocator<T>().deallocate(p, n);
  }
  template <class U>
  bool operator==(const Alloc<U>& o) const noexcept { return id == o.id; }
};

bool started = false;
struct tracked {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    void start() & noexcept {
      started = true;
      ex::set_value(std::move(r), 3);
    }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r)};
  }
};
// A sender whose attributes carry an allocator (/19.2).
struct with_alloc_attr : tracked {
  auto get_env() const noexcept { return ex::prop(std::get_allocator, Alloc<int>(2)); }
};

struct ThrowingMove {
  ThrowingMove() = default;
  ThrowingMove(const ThrowingMove&) { throw std::runtime_error("copy"); }
  ThrowingMove(ThrowingMove&&) noexcept(false) { throw std::runtime_error("move"); }
};

int main() {
  // /8: a closed scope.
  {
    ex::counting_scope scope;
    scope.close();
    started = false;
    auto f = ex::spawn_future(tracked{}, scope.get_token());
    CHECK(!started);
    exec_test::record<int, int> rec;
    exec_test::run(std::move(f), exec_test::receiver_for(rec));
    CHECK(rec.how == done::stopped);
    tt::sync_wait(scope.join());
  }
  // /19.1: the environment's allocator.
  {
    ex::counting_scope scope;
    allocs = deallocs = 0;
    auto f = ex::spawn_future(tracked{}, scope.get_token(), ex::prop(std::get_allocator, Alloc<int>(1)));
    CHECK(allocs == 1 && deallocs == 0);
    auto r = tt::sync_wait(std::move(f));
    CHECK(r && std::get<0>(*r) == 3 && deallocs == 1);
    tt::sync_wait(scope.join());
  }
  // /19.2: the wrapped sender's allocator.
  {
    ex::counting_scope scope;
    allocs = deallocs = 0;
    auto f = ex::spawn_future(with_alloc_attr{}, scope.get_token());
    CHECK(allocs == 1);
    auto r = tt::sync_wait(std::move(f));
    CHECK(r && std::get<0>(*r) == 3 && deallocs == 1);
    tt::sync_wait(scope.join());
  }
  // /19.3 and /21: neither: the default allocator (no allocation through Alloc).
  {
    ex::counting_scope scope;
    allocs = 0;
    auto r = tt::sync_wait(ex::spawn_future(tracked{}, scope.get_token()));
    CHECK(r && allocs == 0);
    tt::sync_wait(scope.join());
  }
  // /5.2, /6: storing the result throws: set_error(exception_ptr).
  {
    ex::counting_scope scope;
    auto f = ex::spawn_future(ex::just() | ex::then([] { return ThrowingMove{}; }), scope.get_token());
    exec_test::record<std::exception_ptr> rec;
    exec_test::run(std::move(f), exec_test::receiver_for(rec));
    CHECK(rec.how == done::error && rec.error && *rec.error);
    bool right = false;
    try {
      std::rethrow_exception(*rec.error);
    } catch (const std::runtime_error&) {
      right = true;
    }
    CHECK(right);
    tt::sync_wait(scope.join());
  }
  return 0;
}
