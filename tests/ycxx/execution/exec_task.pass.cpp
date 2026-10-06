// execution::task ([exec.task]):
//   task<T, Environment> is a sender for coroutines: co_return v is set_value(v) (none for
//   void); an exception escaping the body is set_error(exception_ptr) when error_types has that
//   signature; co_yield with_error(e) completes with set_error(Cerr(e)) for the one Cerr of
//   error_types e converts to; a stopped completion of an awaited sender completes the task with
//   set_stopped. Its completion signatures: SET-VALUE-SIG(T), error_types, set_stopped_t().
//   The nested types come from Environment, else the defaults ([task.class]/2).
//   Awaiting a sender resumes on the task's start scheduler ([task.promise]/6, affine).
//   The frame is allocated with an allocator passed as allocator_arg, alloc ([task.promise]/14).
// FLAGS: -pthread
// REQUIRES: exceptions
#include <execution>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <type_traits>
#include "check.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;

static_assert(ex::sender<ex::task<int>>);
static_assert(ex::sender<ex::task<>>);
static_assert(ex::sender<ex::task<int&>>);
static_assert(std::is_same_v<ex::task<int>::allocator_type, std::allocator<std::byte>>);
static_assert(std::is_same_v<ex::task<int>::start_scheduler_type, ex::task_scheduler>);
static_assert(std::is_same_v<ex::task<int>::stop_source_type, std::inplace_stop_source>);
static_assert(std::is_same_v<ex::task<int>::stop_token_type, std::inplace_stop_token>);
static_assert(std::is_same_v<ex::task<int>::error_types, ex::completion_signatures<ex::set_error_t(std::exception_ptr)>>);
static_assert(std::is_same_v<ex::completion_signatures_of_t<ex::task<int>>,
                             ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>>);
static_assert(std::is_same_v<ex::completion_signatures_of_t<ex::task<>>,
                             ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>>);
static_assert(!std::is_copy_constructible_v<ex::task<>>);
static_assert(std::is_nothrow_move_constructible_v<ex::task<>>);

struct err_env {
  using error_types = ex::completion_signatures<ex::set_error_t(std::error_code)>;
};
static_assert(std::is_same_v<ex::completion_signatures_of_t<ex::task<int, err_env>>,
                             ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::error_code), ex::set_stopped_t()>>);

struct inline_env {
  using start_scheduler_type = ex::inline_scheduler;
};

ex::task<int> leaf(int x) { co_return x * 2; }
ex::task<int> sum() {
  int a = co_await leaf(10);
  int b = co_await ex::just(5);
  co_return a + b;
}
ex::task<> thrower() {
  throw std::runtime_error("t");
  co_return;
}
ex::task<int, err_env> yields_error() {
  co_yield ex::with_error(std::make_error_code(std::errc::invalid_argument));
  co_return 1;
}
ex::task<int> stops() {
  co_await ex::just_stopped();
  co_return 1;
}
int global = 4;
ex::task<int&> ref() { co_return global; }
ex::task<std::thread::id> where() { co_return tt::get_id(); }

// Awaiting a sender that completes on another thread resumes on the start scheduler.
ex::task<std::pair<std::thread::id, std::thread::id>> resumes_home(decltype(std::declval<ex::run_loop&>().get_scheduler()) other) {
  auto there = co_await (ex::schedule(other) | ex::then([] { return tt::get_id(); }));
  co_return std::pair(there, tt::get_id());
}
ex::task<int, inline_env> inline_task() { co_return co_await ex::just(9); }

// The stop token of the task's environment follows the receiver's.
ex::task<bool> sees_stop() { co_return (co_await ex::read_env(std::get_stop_token)).stop_requested(); }

struct counting_alloc_state {
  static inline int allocations = 0;
};
template <class T>
struct counting_alloc {
  using value_type = T;
  counting_alloc() = default;
  template <class U>
  counting_alloc(const counting_alloc<U>&) noexcept {}
  T* allocate(std::size_t n) {
    ++counting_alloc_state::allocations;
    return std::allocator<T>().allocate(n);
  }
  void deallocate(T* p, std::size_t n) noexcept { std::allocator<T>().deallocate(p, n); }
  bool operator==(const counting_alloc&) const = default;
};
ex::task<int> with_alloc(std::allocator_arg_t, counting_alloc<int>, int v) { co_return v; }

int main() {
  auto r = tt::sync_wait(sum());
  CHECK(r && std::get<0>(*r) == 25);

  bool caught = false;
  try {
    tt::sync_wait(thrower());
  } catch (const std::runtime_error&) {
    caught = true;
  }
  CHECK(caught);

  caught = false;
  try {
    tt::sync_wait(yields_error());
  } catch (const std::system_error& e) {
    caught = e.code() == std::errc::invalid_argument;
  }
  CHECK(caught);

  CHECK(!tt::sync_wait(stops()));

  auto rr = tt::sync_wait(ref());
  CHECK(rr && std::get<0>(*rr) == 4);

  auto w = tt::sync_wait(where());
  CHECK(w && std::get<0>(*w) == tt::get_id());

  {
    ex::run_loop loop;
    std::thread t([&] { loop.run(); });
    const auto tid = t.get_id();
    auto h = tt::sync_wait(resumes_home(loop.get_scheduler()));
    loop.finish();
    t.join();
    CHECK(h && std::get<0>(*h).first == tid && std::get<0>(*h).second == tt::get_id());
  }

  auto it = tt::sync_wait(inline_task());
  CHECK(it && std::get<0>(*it) == 9);

  auto ss = tt::sync_wait(sees_stop());
  CHECK(ss && !std::get<0>(*ss));

  const int before = counting_alloc_state::allocations;
  auto a = tt::sync_wait(with_alloc(std::allocator_arg, counting_alloc<int>(), 3));
  CHECK(a && std::get<0>(*a) == 3);
  CHECK(counting_alloc_state::allocations == before + 1);

  // A task not connected is destroyed with its frame; a moved-from task is empty.
  {
    auto t1 = leaf(1);
    auto t2 = std::move(t1);
    (void)t2;
  }
  return 0;
}
