// [unique.ptr.single.general]: element_type is T, deleter_type is D, D defaults to
// default_delete<T>; pointer is remove_reference_t<D>::pointer if that is valid and denotes a
// type, otherwise element_type*. [unique.ptr.runtime.general]: the same for unique_ptr<T[]>
// (element_type is T, the default deleter is default_delete<T[]>). The copy constructor and
// copy assignment are deleted; the move operations are noexcept.
// COUNTERPART: libcxx:utilities/smartptr/unique.ptr/unique.ptr.class/unique.ptr.observers/dereference.single.pass.cpp
#include <memory>
#include <cstddef>
#include <type_traits>

struct Fancy {
  int* p = nullptr;
  Fancy() = default;
  Fancy(std::nullptr_t) {}
  explicit Fancy(int* q) : p(q) {}
  friend bool operator==(Fancy, Fancy) = default;
};
struct FancyDeleter {
  using pointer = Fancy;
  void operator()(Fancy) const {}
};
struct PlainDeleter {
  void operator()(int*) const {}
};

static_assert(std::is_same_v<std::unique_ptr<int>::element_type, int>);
static_assert(std::is_same_v<std::unique_ptr<int>::pointer, int*>);
static_assert(std::is_same_v<std::unique_ptr<int>::deleter_type, std::default_delete<int>>);
static_assert(std::is_same_v<std::unique_ptr<const int>::pointer, const int*>);
static_assert(std::is_same_v<std::unique_ptr<int, PlainDeleter>::pointer, int*>);
static_assert(std::is_same_v<std::unique_ptr<int, FancyDeleter>::pointer, Fancy>);
static_assert(std::is_same_v<std::unique_ptr<int, FancyDeleter&>::pointer, Fancy>);
static_assert(std::is_same_v<std::unique_ptr<int, const FancyDeleter&>::pointer, Fancy>);
static_assert(std::is_same_v<std::unique_ptr<int, FancyDeleter&>::deleter_type, FancyDeleter&>);

static_assert(std::is_same_v<std::unique_ptr<int[]>::element_type, int>);
static_assert(std::is_same_v<std::unique_ptr<int[]>::pointer, int*>);
static_assert(std::is_same_v<std::unique_ptr<int[]>::deleter_type, std::default_delete<int[]>>);
static_assert(std::is_same_v<std::unique_ptr<int[], FancyDeleter>::pointer, Fancy>);

template <class P>
constexpr bool move_only() {
  static_assert(!std::is_copy_constructible_v<P>);
  static_assert(!std::is_copy_assignable_v<P>);
  static_assert(std::is_nothrow_move_constructible_v<P>);
  static_assert(std::is_nothrow_move_assignable_v<P>);
  static_assert(std::is_nothrow_default_constructible_v<P>);
  static_assert(std::is_nothrow_constructible_v<P, std::nullptr_t>);
  static_assert(std::is_nothrow_assignable_v<P&, std::nullptr_t>);
  static_assert(std::is_nothrow_swappable_v<P>);
  return true;
}
static_assert(move_only<std::unique_ptr<int>>());
static_assert(move_only<std::unique_ptr<int[]>>());
static_assert(move_only<std::unique_ptr<int, PlainDeleter>>());

// explicit operator bool; noexcept observers
static_assert(!std::is_convertible_v<std::unique_ptr<int>, bool>);
static_assert(std::is_constructible_v<bool, std::unique_ptr<int>>);
static_assert(noexcept(std::declval<std::unique_ptr<int>&>().get()));
static_assert(noexcept(std::declval<std::unique_ptr<int>&>().release()));
static_assert(noexcept(std::declval<std::unique_ptr<int>&>().reset()));
static_assert(noexcept(std::declval<std::unique_ptr<int>&>().get_deleter()));
static_assert(noexcept(std::declval<std::unique_ptr<int>&>().operator->()));
static_assert(noexcept(*std::declval<std::unique_ptr<int>&>()));
static_assert(std::is_same_v<decltype(*std::declval<const std::unique_ptr<int>&>()), int&>);
static_assert(std::is_same_v<decltype(std::declval<const std::unique_ptr<int[]>&>()[0]), int&>);
static_assert(std::is_same_v<decltype(std::declval<const std::unique_ptr<int>&>().get_deleter()),
                             const std::default_delete<int>&>);

// The single-parameter pointer constructor is explicit.
static_assert(!std::is_convertible_v<int*, std::unique_ptr<int>>);
static_assert(std::is_constructible_v<std::unique_ptr<int>, int*>);
static_assert(!std::is_convertible_v<int*, std::unique_ptr<int[]>>);
static_assert(std::is_convertible_v<std::nullptr_t, std::unique_ptr<int>>);

// The array form provides neither operator* nor operator->, the single form no operator[].
template <class P>
concept has_star = requires(P p) { *p; };
template <class P>
concept has_arrow = requires(P p) { p.operator->(); };
template <class P>
concept has_index = requires(P p) { p[0]; };
static_assert(has_star<std::unique_ptr<int>> && has_arrow<std::unique_ptr<int>>);
static_assert(!has_index<std::unique_ptr<int>>);
static_assert(!has_star<std::unique_ptr<int[]>> && !has_arrow<std::unique_ptr<int[]>>);
static_assert(has_index<std::unique_ptr<int[]>>);

int main() {}
