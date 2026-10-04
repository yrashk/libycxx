// [func.wrap.func.con]/1-2: function() and function(nullptr_t) are noexcept with
// postcondition !*this. /3: copy: "the target object of *this is a copy of the target object
// of f". /6: move: "the target of *this is equivalent to the target of f before the
// construction". /13: "*this has a target object of type FD direct-non-list-initialized with
// std::forward<F>(f)". /19-30 assignment; [func.wrap.func.mod] swap; [func.wrap.func.cap]
// operator bool; [func.wrap.func.inv]/1: "Returns: INVOKE<R>(f, std::forward<ArgTypes>(args)...)";
// [func.wrap.func.nullptr]: operator==(f, nullptr) returns !f.
#include <functional>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "check.hpp"

int twice(int x) { return 2 * x; }
struct Adder {
  int n;
  int operator()(int x) const { return x + n; }
};
struct Counter {
  int calls = 0;
  int operator()(int x) { return x + ++calls; }
};
struct S {
  int v;
  int get(int x) const { return v * x; }
};

using F = std::function<int(int)>;
static_assert(std::is_same_v<F::result_type, int>);
static_assert(std::is_nothrow_default_constructible_v<F>);
static_assert(std::is_nothrow_constructible_v<F, std::nullptr_t>);
static_assert(std::is_nothrow_move_constructible_v<F>);
static_assert(std::is_copy_constructible_v<F> && std::is_copy_assignable_v<F>);
static_assert(std::is_nothrow_assignable_v<F&, std::nullptr_t>);
static_assert(noexcept(std::declval<F&>().swap(std::declval<F&>())));
static_assert(noexcept(swap(std::declval<F&>(), std::declval<F&>())));
static_assert(noexcept(static_cast<bool>(std::declval<const F&>())));
static_assert(noexcept(std::declval<const F&>() == nullptr));
static_assert(std::is_same_v<decltype(std::declval<const F&>() == nullptr), bool>);
static_assert(!std::is_convertible_v<F, bool>);  // explicit operator bool
static_assert(std::is_invocable_r_v<int, const F&, int>);  // operator() is const

int main() {
  F empty;
  CHECK(!empty);
  CHECK(empty == nullptr && nullptr == empty);
  CHECK(!(empty != nullptr));
  F null_constructed(nullptr);
  CHECK(!null_constructed);

  F fp(twice);
  CHECK(fp && fp(4) == 8);
  F fo(Adder{3});
  CHECK(fo(4) == 7);
  F fl = [](int x) { return x - 1; };
  CHECK(fl(4) == 3);
  std::function<int(const S&, int)> fm(&S::get);
  CHECK(fm(S{3}, 5) == 15);
  std::function<int(const S&)> fd(&S::v);
  CHECK(fd(S{9}) == 9);

  // copy: independent copy of the target
  F c1 = Counter{};
  CHECK(c1(10) == 11);
  F c2 = c1;
  CHECK(c2(10) == 12);
  CHECK(c1(10) == 12);
  // const operator() still calls the stored (non-const) target as an lvalue
  const F& cc = c1;
  CHECK(cc(0) == 3);

  // move
  F m = std::move(c2);
  CHECK(m && m(0) == 3);

  // assignment
  F a;
  a = fp;
  CHECK(a(1) == 2);
  a = std::move(fo);
  CHECK(a(1) == 4);
  a = Adder{10};
  CHECK(a(1) == 11);
  a = twice;
  CHECK(a(5) == 10);
  a = nullptr;
  CHECK(!a);
  F& self = (a = fl);
  CHECK(&self == &a && a(1) == 0);

  // swap
  F s1(twice), s2(Adder{1});
  s1.swap(s2);
  CHECK(s1(5) == 6 && s2(5) == 10);
  swap(s1, s2);
  CHECK(s1(5) == 10 && s2(5) == 6);
  F s3;
  s3.swap(s1);
  CHECK(!s1 && s3(1) == 2);

  // INVOKE<R>: void result discards, R converts
  int hits = 0;
  std::function<void(int)> v = [&](int x) { hits += x; return 99; };
  v(3);
  CHECK(hits == 3);
  std::function<long(short)> conv = twice;
  CHECK(conv(short(21)) == 42L);
  return 0;
}
