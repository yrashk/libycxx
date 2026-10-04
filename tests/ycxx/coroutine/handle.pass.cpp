// [coroutine.handle.general], [coroutine.handle.con], [coroutine.handle.export.import],
// [coroutine.handle.observers], [coroutine.handle.conv], [coroutine.handle.compare],
// [coroutine.handle.hash]: a default/nullptr-constructed handle has address() == nullptr;
// from_address(address()) == *this; operator bool is address() != nullptr; conversion to
// coroutine_handle<>; == compares addresses; <=> is compare_three_way()(x.address(),
// y.address()) of type strong_ordering; hash<coroutine_handle<P>> is enabled.
#include <coroutine>
#include <compare>
#include <cstddef>
#include <functional>
#include <type_traits>
#include "check.hpp"

struct Task {
  struct promise_type {
    int value = 0;
    Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
    std::suspend_always yield_value(int v) {
      value = v;
      return {};
    }
  };
  std::coroutine_handle<promise_type> h;
};

Task counter() {
  co_yield 1;
  co_yield 2;
}

using H = std::coroutine_handle<>;
using HP = std::coroutine_handle<Task::promise_type>;
static_assert(std::is_same_v<std::coroutine_handle<>, std::coroutine_handle<void>>);
static_assert(std::is_nothrow_default_constructible_v<H>);
static_assert(std::is_nothrow_constructible_v<H, std::nullptr_t>);
static_assert(std::is_convertible_v<std::nullptr_t, HP>);
static_assert(std::is_nothrow_convertible_v<HP, H>);
static_assert(!std::is_convertible_v<H, HP>);
static_assert(!std::is_convertible_v<H, bool>);  // explicit operator bool
static_assert(noexcept(H().address()));
static_assert(std::is_same_v<decltype(H() <=> H()), std::strong_ordering>);
static_assert(noexcept(H() == H()));
static_assert(noexcept(H() <=> H()));
static_assert(std::is_same_v<decltype(std::declval<HP&>() = nullptr), HP&>);
static_assert(std::is_same_v<decltype(HP::from_address(nullptr)), HP>);
static_assert(std::is_same_v<decltype(std::declval<const HP&>().promise()), Task::promise_type&>);

constexpr bool test_constexpr() {
  H h;
  H n(nullptr);
  HP hp;
  return h.address() == nullptr && n.address() == nullptr && !h && !hp && h == n && (h <=> n) == 0 &&
         H::from_address(nullptr).address() == nullptr && static_cast<H>(hp) == h;
}
static_assert(test_constexpr());

int main() {
  CHECK(test_constexpr());
  Task t = counter();
  HP hp = t.h;
  CHECK(static_cast<bool>(hp));
  CHECK(!hp.done());
  H h = hp;
  CHECK(h.address() == hp.address());
  CHECK(h == hp);
  CHECK(HP::from_address(hp.address()) == hp);
  CHECK(H::from_address(h.address()) == h);
  CHECK(&HP::from_promise(hp.promise()).promise() == &hp.promise());
  hp.resume();
  CHECK(hp.promise().value == 1);
  h();  // operator() resumes too
  CHECK(hp.promise().value == 2);
  h.resume();
  CHECK(hp.done());
  CHECK(h.done());
  CHECK((h <=> H()) != 0);
  CHECK(std::hash<H>{}(h) == std::hash<H>{}(H::from_address(h.address())));
  CHECK(std::hash<HP>{}(hp) == std::hash<HP>{}(hp));
  hp.destroy();
  HP copy = hp;
  HP& r = (copy = nullptr);
  CHECK(&r == &copy);
  CHECK(copy.address() == nullptr);
  return 0;
}
