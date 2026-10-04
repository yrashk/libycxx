// [coroutine.noop], [coroutine.handle.noop]: noop_coroutine() noexcept returns a
// noop_coroutine_handle; operator bool returns true, done() returns false, resume(),
// operator() and destroy() have no effect (all constexpr noexcept), promise() returns a
// noop_coroutine_promise&, address() is non-null and stable, and the handle converts to
// coroutine_handle<>. [coroutine.trivial.awaitables]: suspend_never/suspend_always.
#include <coroutine>
#include <type_traits>
#include "check.hpp"

using NH = std::noop_coroutine_handle;
static_assert(std::is_same_v<NH, std::coroutine_handle<std::noop_coroutine_promise>>);
static_assert(std::is_same_v<decltype(std::noop_coroutine()), NH>);
static_assert(noexcept(std::noop_coroutine()));
static_assert(!std::is_default_constructible_v<NH>);
static_assert(std::is_nothrow_convertible_v<NH, std::coroutine_handle<>>);
static_assert(noexcept(std::declval<const NH&>().done()));
static_assert(noexcept(std::declval<const NH&>().resume()));
static_assert(noexcept(std::declval<const NH&>()()));
static_assert(noexcept(std::declval<const NH&>().destroy()));
static_assert(noexcept(std::declval<const NH&>().promise()));
static_assert(noexcept(std::declval<const NH&>().address()));
static_assert(std::is_same_v<decltype(std::declval<const NH&>().promise()), std::noop_coroutine_promise&>);
static_assert(std::is_empty_v<std::noop_coroutine_promise>);

// trivial awaitables: all members constexpr noexcept
static_assert(std::suspend_never{}.await_ready());
static_assert(!std::suspend_always{}.await_ready());
static_assert(noexcept(std::suspend_never{}.await_ready()));
static_assert(noexcept(std::suspend_always{}.await_suspend(std::coroutine_handle<>())));
static_assert(noexcept(std::suspend_always{}.await_resume()));
static_assert(std::is_same_v<decltype(std::suspend_always{}.await_suspend(std::coroutine_handle<>())), void>);
static_assert(std::is_same_v<decltype(std::suspend_never{}.await_resume()), void>);
static_assert(std::is_trivially_copyable_v<std::suspend_always> && std::is_empty_v<std::suspend_never>);

int main() {
  NH h = std::noop_coroutine();
  CHECK(static_cast<bool>(h));
  CHECK(!h.done());
  CHECK(h.address() != nullptr);
  h.resume();
  h();
  h.destroy();
  CHECK(!h.done());
  std::coroutine_handle<> g = h;
  CHECK(g.address() == h.address());
  CHECK(!g.done());
  g.resume();  // resuming the no-op coroutine through the type-erased handle does nothing
  std::noop_coroutine_promise& p = h.promise();
  (void)p;
  NH h2 = h;
  CHECK(h2.address() == h.address());
  return 0;
}
