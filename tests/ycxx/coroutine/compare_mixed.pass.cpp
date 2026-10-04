// [coroutine.syn], [coroutine.handle.compare]: the comparisons are non-template namespace-scope
// functions taking coroutine_handle<> by value:
//   constexpr bool operator==(coroutine_handle<> x, coroutine_handle<> y) noexcept;
//     Returns: x.address() == y.address().
//   constexpr strong_ordering operator<=>(coroutine_handle<> x, coroutine_handle<> y) noexcept;
//     Returns: compare_three_way()(x.address(), y.address()).
// So handles with different promise types, noop_coroutine_handle and nullptr (coroutine_handle<>
// is implicitly constructible from nullptr_t, [coroutine.handle.con]) all compare through
// implicit conversions to coroutine_handle<>, and the operators are found by ordinary lookup
// after `using namespace std` or by qualified call as well as by ADL (namespace std is
// associated with every coroutine_handle specialization).
#include <coroutine>
#include <compare>
#include <functional>
#include <type_traits>
#include "check.hpp"

struct TaskA {
  struct promise_type {
    TaskA get_return_object() { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };
  std::coroutine_handle<promise_type> h;
};
struct TaskB {
  struct promise_type {
    int x = 0;
    TaskB get_return_object() { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };
  std::coroutine_handle<promise_type> h;
};
TaskA coro_a() { co_return; }
TaskB coro_b() { co_return; }

using HA = std::coroutine_handle<TaskA::promise_type>;
using HB = std::coroutine_handle<TaskB::promise_type>;
using H = std::coroutine_handle<>;
using NH = std::noop_coroutine_handle;

template <class X, class Y> concept EqComparable = requires(X x, Y y) { { x == y } -> std::same_as<bool>; { x != y } -> std::same_as<bool>; };
template <class X, class Y> concept Ordered = requires(X x, Y y) { { x <=> y } -> std::same_as<std::strong_ordering>; x < y; x >= y; };

static_assert(EqComparable<HA, HB> && Ordered<HA, HB>);
static_assert(EqComparable<HA, H> && Ordered<H, HB>);
static_assert(EqComparable<HA, NH> && Ordered<NH, HB>);
static_assert(EqComparable<NH, NH> && Ordered<NH, H>);
static_assert(EqComparable<HA, std::nullptr_t> && EqComparable<std::nullptr_t, HB> && EqComparable<H, std::nullptr_t>);
static_assert(Ordered<HA, std::nullptr_t> && Ordered<std::nullptr_t, H>);
static_assert(noexcept(HA() == HB()) && noexcept(HA() <=> HB()));
// Callable as std::operator== directly (namespace-scope functions, not hidden friends).
static_assert(std::is_same_v<decltype(std::operator==(HA(), HB())), bool>);
static_assert(std::is_same_v<decltype(std::operator<=>(H(), HB())), std::strong_ordering>);

constexpr bool constexpr_compare() {
  HA a;
  HB b;
  return a == b && a == nullptr && nullptr == b && (a <=> b) == 0 && !(a != b) && a <= b;
}
static_assert(constexpr_compare());

int main() {
  TaskA ta = coro_a();
  TaskB tb = coro_b();
  NH n = std::noop_coroutine();
  CHECK(ta.h != tb.h);
  CHECK(!(ta.h == tb.h));
  CHECK(ta.h != nullptr && nullptr != tb.h);
  CHECK(ta.h != n && n != tb.h);
  CHECK(n == n);   // ([coroutine.noop]/2: different noop_coroutine() calls may or may not compare equal)
  H ga = ta.h;
  CHECK(ga == ta.h && ta.h == ga);
  CHECK((ga <=> ta.h) == 0);
  const bool a_less = std::less<void*>()(ta.h.address(), tb.h.address());
  CHECK((ta.h < tb.h) == a_less);
  CHECK((ta.h <=> tb.h) == (a_less ? std::strong_ordering::less : std::strong_ordering::greater));
  CHECK((tb.h > ta.h) == a_less);
  CHECK((ta.h <=> nullptr) != 0);
  ta.h.destroy();
  tb.h.destroy();
}
