// [exec.spawn]:
//   /9.1, /10.1: with an allocator in env, spawn allocates its state with it and the work sees
//     env (write_env(..., senv) with senv = env);
//   /9.2: otherwise the allocator of the (wrapped) sender's attributes is used, and the work's
//     environment gains it: senv = JOIN-ENV(prop(get_allocator, alloc), env);
//   /6-8: try_associate is called when the state is constructed; without an association the
//     operation is not started and the state is destroyed and deallocated at once (/7, /8);
//     with one, the state is freed when the work completes, and the association released (so
//     join can complete);
//   /10.1: if constructing the state throws (here: connect), what was allocated is deallocated
//     and the exception propagates; no association was taken.
//   /11: spawn(sndr, token) is spawn(sndr, token, env<>()).
// REQUIRES: exceptions
#include <execution>
#include <cstddef>
#include <memory>
#include <new>
#include <utility>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;

int allocs[3], deallocs[3];
template <class T>
struct Alloc {
  using value_type = T;
  int id = 0;
  Alloc() = default;
  explicit Alloc(int i) : id(i) {}
  template <class U>
  Alloc(const Alloc<U>& o) noexcept : id(o.id) {}
  T* allocate(std::size_t n) {
    ++allocs[id];
    return static_cast<T*>(::operator new(n * sizeof(T)));
  }
  void deallocate(T* p, std::size_t) noexcept {
    ++deallocs[id];
    ::operator delete(p);
  }
  template <class U>
  bool operator==(const Alloc<U>& o) const noexcept {
    return id == o.id;
  }
};

int seen_alloc = -1;
int runs = 0;
// Records the id of the allocator in its environment; its attributes name allocator `attr_id`
// when that is not 0; its connect throws when told to.
struct probe {
  using sender_concept = ex::sender_tag;
  int attr_id = 0;
  bool throw_on_connect = false;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t()>();
  }
  struct attrs {
    int id;
    Alloc<int> query(std::get_allocator_t) const noexcept { return Alloc<int>(id); }
  };
  attrs get_env() const noexcept { return {attr_id}; }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    void start() & noexcept {
      ++runs;
      if constexpr (requires { std::get_allocator(ex::get_env(r)); })
        seen_alloc = std::get_allocator(ex::get_env(r)).id;
      ex::set_value(std::move(r));
    }
  };
  template <class R>
  op<R> connect(R r) const {
    if (throw_on_connect)
      throw 5;
    return {std::move(r)};
  }
};

int main() {
  // /9.1: the environment's allocator, which the work sees.
  {
    ex::simple_counting_scope scope;
    ex::spawn(probe{2}, scope.get_token(), ex::env{ex::prop(std::get_allocator, Alloc<int>(1))});
    CHECK(runs == 1 && seen_alloc == 1);
    CHECK(allocs[1] == 1 && deallocs[1] == 1 && allocs[2] == 0);
    (void)tt::sync_wait(scope.join());
  }
  // /9.2: the sender's allocator, added to the work's environment; /11.
  {
    runs = 0;
    seen_alloc = -1;
    ex::simple_counting_scope scope;
    ex::spawn(probe{2}, scope.get_token());
    CHECK(runs == 1 && seen_alloc == 2);
    CHECK(allocs[2] == 1 && deallocs[2] == 1);
    (void)tt::sync_wait(scope.join());
  }
  // The same through counting_scope's token, whose wrap forwards the attributes
  // ([exec.adapt.general]/3.2).
  {
    runs = 0;
    seen_alloc = -1;
    ex::counting_scope scope;
    ex::spawn(probe{2}, scope.get_token());
    CHECK(runs == 1 && seen_alloc == 2);
    CHECK(allocs[2] == 2 && deallocs[2] == 2);
    (void)tt::sync_wait(scope.join());
  }
  // /7: a closed scope: not started; the state is freed at once.
  {
    runs = 0;
    ex::simple_counting_scope scope;
    scope.close();
    ex::spawn(probe{0}, scope.get_token(), ex::env{ex::prop(std::get_allocator, Alloc<int>(1))});
    CHECK(runs == 0);
    CHECK(allocs[1] == 2 && deallocs[1] == 2);
    (void)tt::sync_wait(scope.join());
  }
  // /10.1: connect throws: deallocated, rethrown, no association left behind.
  {
    runs = 0;
    ex::simple_counting_scope scope;
    bool caught = false;
    try {
      ex::spawn(probe{0, true}, scope.get_token(), ex::env{ex::prop(std::get_allocator, Alloc<int>(1))});
    } catch (int e) {
      caught = e == 5;
    }
    CHECK(caught && runs == 0);
    CHECK(allocs[1] == 3 && deallocs[1] == 3);
    // join completes inline when started: nothing is associated
    // ([exec.counting.scopes.general]/4: complete-inline).
    exec_test::record<int> rec;
    exec_test::run(scope.join(), exec_test::receiver_for(rec, ex::env{ex::prop(ex::get_start_scheduler, ex::inline_scheduler())}));
    CHECK(rec.how == exec_test::done::value);
  }
  return 0;
}
