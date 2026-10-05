// [atomics.ref.generic.general], [atomics.ref.int], [atomics.ref.float], [atomics.ref.pointer]:
// value_type is remove_cv_t<T>; difference_type for the arithmetic specializations;
// "explicit atomic_ref(T&&) = delete;", copy assignment deleted, the copy constructor noexcept;
// required_alignment >= alignof(T) and, with is_always_lock_free, the same for similar types
// ([atomics.ref.ops]/1,3). The constructor from T& is explicit.
// COUNTERPART: libcxx:atomics/atomics.ref/member_types.compile.pass.cpp
#include <atomic>
#include <cstddef>
#include <type_traits>

template<class T, class Diff>
void check() {
  using R = std::atomic_ref<T>;
  static_assert(std::is_same_v<typename R::value_type, T>);
  if constexpr (!std::is_void_v<Diff>) static_assert(std::is_same_v<typename R::difference_type, Diff>);
  static_assert(std::is_same_v<decltype(R::required_alignment), const std::size_t>);
  static_assert(R::required_alignment >= alignof(T));
  static_assert(std::is_same_v<decltype(R::is_always_lock_free), const bool>);
  static_assert(std::is_constructible_v<R, T&>);
  static_assert(!std::is_convertible_v<T&, R>);
  static_assert(!std::is_constructible_v<R, T&&>);
  static_assert(!std::is_constructible_v<R, const T&>);
  static_assert(std::is_nothrow_copy_constructible_v<R>);
  static_assert(!std::is_copy_assignable_v<R>);
  static_assert(!std::is_move_assignable_v<R>);
  static_assert(std::atomic_ref<const T>::required_alignment == R::required_alignment);
  static_assert(std::atomic_ref<const T>::is_always_lock_free == R::is_always_lock_free);
  static_assert(std::is_same_v<typename std::atomic_ref<const T>::value_type, T>);
  if constexpr (R::is_always_lock_free) {
    static_assert(std::atomic_ref<volatile T>::required_alignment == R::required_alignment);
    static_assert(std::is_same_v<typename std::atomic_ref<volatile T>::value_type, T>);
  }
  T v{};
  R r(v);
  static_assert(noexcept(r.load()));
  static_assert(noexcept(r.store(v)));
  static_assert(noexcept(r.exchange(v)));
  static_assert(noexcept(r.compare_exchange_strong(v, v)));
  static_assert(noexcept(r.is_lock_free()));
  static_assert(std::is_same_v<decltype(r = v), T>);
  static_assert(std::is_same_v<decltype(r.load()), T>);
}

struct S { int a; short b; };
template void check<int, int>();
template void check<unsigned char, unsigned char>();
template void check<long long, long long>();
template void check<double, double>();
template void check<float, float>();
template void check<int*, std::ptrdiff_t>();
template void check<S, void>();

template<class R> concept has_fetch_add = requires(R r) { r.fetch_add(1); };
template<class R> concept has_increment = requires(R r) { ++r; r--; };
template<class R> concept has_fetch_and = requires(R r) { r.fetch_and(1); };
static_assert(has_fetch_add<std::atomic_ref<int>> && has_fetch_and<std::atomic_ref<int>>);
static_assert(has_fetch_add<std::atomic_ref<double>> && !has_fetch_and<std::atomic_ref<double>>);
static_assert(!has_increment<std::atomic_ref<double>>);
static_assert(has_increment<std::atomic_ref<int*>> && !has_fetch_and<std::atomic_ref<int*>>);
static_assert(!has_fetch_add<std::atomic_ref<S>>);
static_assert(!has_fetch_add<std::atomic_ref<bool>>);
