// execution::task's promise ([task.promise]) and operation state ([task.state]):
//   [task.promise]/12.1: get_start_scheduler in the task is start_scheduler_type(SCHED);
//   /12.2: get_allocator is allocator_type(get_allocator(get_env(rcvr))) when the receiver has
//     one, else allocator_type();
//   /12.3, [task.state]/5: get_stop_token is the receiver's token when its type is
//     stop_token_type, else a token of the task's own stop source that reports the receiver
//     token's stop_requested();
//   /12.4, [task.state]/2.4: other forwarding queries go to the Environment object, constructed
//     from own-env (here env<>);
//   /3, /8: the coroutine frame is destroyed (so the coroutine's locals are destroyed) before
//     the receiver's set_value / set_stopped is called; /11 return_value;
//   /14-16: operator new with allocator_arg allocates through the allocator rebound to a type U
//     whose size and alignment are __STDCPP_DEFAULT_NEW_ALIGNMENT__, also for a member coroutine
//     (the overload with the object parameter first), and deallocates with it.
// REQUIRES: exceptions
#include <execution>
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <new>
#include <stop_token>
#include <type_traits>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using exec_test::done;

struct depth_t : std::forwarding_query_t {
  template <class E>
    requires requires(const E& e, const depth_t& q) { e.query(q); }
  constexpr auto operator()(const E& e) const noexcept {
    return e.query(*this);
  }
};
inline constexpr depth_t depth{};

struct Environment {
  using start_scheduler_type = ex::inline_scheduler;
  using allocator_type = std::pmr::polymorphic_allocator<std::byte>;
  int d = 5;
  Environment() = default;
  explicit Environment(const ex::env<>&) : d(6) {}
  int query(depth_t) const noexcept { return d; }
};
using Task = ex::task<int, Environment>;
static_assert(std::is_same_v<Task::allocator_type, std::pmr::polymorphic_allocator<std::byte>>);

Task start_sched() {
  auto s = co_await ex::read_env(ex::get_start_scheduler);
  static_assert(std::is_same_v<decltype(s), ex::inline_scheduler>);
  co_return 1;
}
std::pmr::memory_resource* seen_resource = nullptr;
Task allocator_query() {
  auto a = co_await ex::read_env(std::get_allocator);
  static_assert(std::is_same_v<decltype(a), std::pmr::polymorphic_allocator<std::byte>>);
  seen_resource = a.resource();
  co_return 2;
}
bool token_requested = false, token_possible = false;
std::inplace_stop_token seen_token;
Task stop_query() {
  auto t = co_await ex::read_env(std::get_stop_token);
  static_assert(std::is_same_v<decltype(t), std::inplace_stop_token>);
  token_requested = t.stop_requested();
  token_possible = t.stop_possible();
  seen_token = t;
  co_return 3;
}
Task env_query() { co_return co_await ex::read_env(depth); }

bool local_alive = false;
struct Local {
  Local() { local_alive = true; }
  ~Local() { local_alive = false; }
};
Task with_local() {
  Local l;
  co_return 7;
}
Task stopped_with_local() {
  Local l;
  co_await ex::just_stopped();
  co_return 0;
}
// A receiver that checks, when it completes, that the frame (and the local) is gone.
struct checking_receiver {
  using receiver_concept = ex::receiver_tag;
  done* how;
  bool* local_gone;
  void set_value(int) && noexcept { *how = done::value; *local_gone = !local_alive; }
  void set_error(std::exception_ptr) && noexcept { *how = done::error; }
  void set_stopped() && noexcept { *how = done::stopped; *local_gone = !local_alive; }
};

// Allocation through allocator_arg.
std::size_t value_size = 0, value_align = 0;
int allocs = 0, deallocs = 0;
template <class T>
struct Alloc {
  using value_type = T;
  Alloc() = default;
  template <class U>
  Alloc(const Alloc<U>&) noexcept {}
  T* allocate(std::size_t n) {
    ++allocs;
    value_size = sizeof(T);
    value_align = alignof(T);
    return std::allocator<T>().allocate(n);
  }
  void deallocate(T* p, std::size_t n) noexcept {
    ++deallocs;
    std::allocator<T>().deallocate(p, n);
  }
  template <class U>
  bool operator==(const Alloc<U>&) const noexcept { return true; }
};
using DefTask = ex::task<int, Environment>;
DefTask allocated(std::allocator_arg_t, Alloc<char>, int v) { co_return v; }
struct Holder {
  int base = 40;
  DefTask member(std::allocator_arg_t, Alloc<long>, int v) const { co_return base + v; }
};

template <class Env = ex::env<>>
int run_value(Task t, Env env = {}) {
  exec_test::record<int, int> rec;
  auto op = ex::connect(std::move(t), exec_test::receiver_for(rec, env));
  ex::start(op);
  CHECK(rec.how == done::value);
  return std::get<0>(*rec.values);
}

int main() {
  CHECK(run_value(start_sched()) == 1);

  // /12.2
  std::pmr::monotonic_buffer_resource mr;
  CHECK(run_value(allocator_query(), ex::prop(std::get_allocator, std::pmr::polymorphic_allocator<int>(&mr))) == 2);
  CHECK(seen_resource == &mr);
  CHECK(run_value(allocator_query()) == 2);
  CHECK(seen_resource == std::pmr::get_default_resource());

  // /12.3
  std::inplace_stop_source isrc;
  CHECK(run_value(stop_query(), ex::prop(std::get_stop_token, isrc.get_token())) == 3);
  CHECK(seen_token == isrc.get_token() && token_possible && !token_requested);
  std::stop_source ssrc;
  ssrc.request_stop();
  CHECK(run_value(stop_query(), ex::prop(std::get_stop_token, ssrc.get_token())) == 3);
  CHECK(token_requested);

  // /12.4: the Environment object, made from own-env (env<>).
  CHECK(run_value(env_query()) == 6);

  // /3 and /8
  {
    done how = done::none;
    bool gone = false;
    auto op = ex::connect(with_local(), checking_receiver{&how, &gone});
    ex::start(op);
    CHECK(how == done::value && gone);
  }
  {
    done how = done::none;
    bool gone = false;
    auto op = ex::connect(stopped_with_local(), checking_receiver{&how, &gone});
    ex::start(op);
    CHECK(how == done::stopped && gone);
  }

  // /14-16
  allocs = deallocs = 0;
  CHECK(run_value(allocated(std::allocator_arg, Alloc<char>(), 9)) == 9);
  CHECK(allocs == 1 && deallocs == 1);
  CHECK(value_size == __STDCPP_DEFAULT_NEW_ALIGNMENT__ && value_align == __STDCPP_DEFAULT_NEW_ALIGNMENT__);
  Holder h;
  allocs = deallocs = 0;
  CHECK(run_value(h.member(std::allocator_arg, Alloc<long>(), 2)) == 42);
  CHECK(allocs == 1 && deallocs == 1);
  return 0;
}
