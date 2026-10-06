// [task.members]/5: task::connect(rcvr) mandates that allocator_type(get_allocator(get_env(rcvr)))
// or allocator_type() is well-formed. Here allocator_type has no default constructor and the
// receiver's environment has no allocator: connecting is ill-formed.
// EXPECT-ERROR: static assert.*allocator
#include <execution>
#include <cstddef>
#include <memory>

namespace ex = std::execution;

template <class T>
struct no_default_alloc {
  using value_type = T;
  int id;
  explicit no_default_alloc(int i) : id(i) {}
  template <class U>
  no_default_alloc(const no_default_alloc<U>& o) noexcept : id(o.id) {}
  T* allocate(std::size_t n) { return std::allocator<T>().allocate(n); }
  void deallocate(T* p, std::size_t n) noexcept { std::allocator<T>().deallocate(p, n); }
  template <class U>
  bool operator==(const no_default_alloc<U>&) const noexcept {
    return true;
  }
};

struct alloc_env {
  using start_scheduler_type = ex::inline_scheduler;
  using allocator_type = no_default_alloc<std::byte>;
};

ex::task<void, alloc_env> body(std::allocator_arg_t, no_default_alloc<std::byte>) { co_return; }

struct rcvr {
  using receiver_concept = ex::receiver_tag;
  void set_value() && noexcept {}
  void set_error(std::exception_ptr) && noexcept {}
  void set_stopped() && noexcept {}
};

void f() {
  auto op = ex::connect(body(std::allocator_arg, no_default_alloc<std::byte>(1)), rcvr{});
  (void)op;
}
