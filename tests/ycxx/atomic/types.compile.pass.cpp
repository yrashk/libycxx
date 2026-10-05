// [atomics.types.generic.general], [atomics.types.int]/1-2, [atomics.types.float]/1-2,
// [atomics.types.pointer]: member types value_type / difference_type; the copy operations are
// deleted; "The atomic integral specializations are standard-layout structs. They each have a
// trivial destructor." (likewise atomic<bool>, the floating-point specializations); every
// operation in the synopses is noexcept; [atomics.syn]: memory_order and the type aliases.
// COUNTERPART: libcxx:atomics/atomics.types.generic/integral(_typedefs)?.pass.cpp
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

template<class T, class Diff>
constexpr bool check_arith() {
  using A = std::atomic<T>;
  static_assert(std::is_same_v<typename A::value_type, T>);
  static_assert(std::is_same_v<typename A::difference_type, Diff>);
  static_assert(std::is_standard_layout_v<A>);
  static_assert(std::is_trivially_destructible_v<A>);
  static_assert(!std::is_copy_constructible_v<A>);
  static_assert(!std::is_move_constructible_v<A>);
  static_assert(!std::is_copy_assignable_v<A>);
  static_assert(!std::is_move_assignable_v<A>);
  static_assert(!std::is_assignable_v<volatile A&, const A&>);
  static_assert(std::is_nothrow_default_constructible_v<A>);
  static_assert(std::is_nothrow_constructible_v<A, T>);
  static_assert(std::is_same_v<decltype(A::is_always_lock_free), const bool>);
  A a;
  T v{};
  static_assert(noexcept(a.is_lock_free()));
  static_assert(noexcept(a.store(v)));
  static_assert(noexcept(a.load()));
  static_assert(noexcept(a.exchange(v)));
  static_assert(noexcept(a.compare_exchange_weak(v, v)));
  static_assert(noexcept(a.compare_exchange_strong(v, v)));
  static_assert(noexcept(a.compare_exchange_strong(v, v, std::memory_order::acq_rel, std::memory_order::acquire)));
  static_assert(noexcept(a.fetch_add(Diff{})));
  static_assert(noexcept(a.fetch_sub(Diff{})));
  static_assert(noexcept(a.wait(v)));
  static_assert(noexcept(a.notify_one()));
  static_assert(noexcept(a.notify_all()));
  static_assert(std::is_same_v<decltype(a.load()), T>);
  static_assert(std::is_same_v<decltype(a = v), T>);
  static_assert(std::is_same_v<decltype(a.exchange(v)), T>);
  static_assert(std::is_same_v<decltype(a.compare_exchange_strong(v, v)), bool>);
  static_assert(std::is_same_v<decltype(a.fetch_add(Diff{})), T>);
  static_assert(std::is_same_v<decltype(a += Diff{}), T>);
  static_assert(std::is_same_v<decltype(static_cast<T>(a)), T>);
  return true;
}
static_assert(check_arith<int, int>());
static_assert(check_arith<unsigned long long, unsigned long long>());
static_assert(check_arith<char, char>());
static_assert(check_arith<char8_t, char8_t>());
static_assert(check_arith<wchar_t, wchar_t>());
static_assert(check_arith<float, float>());
static_assert(check_arith<double, double>());
static_assert(check_arith<long double, long double>());
static_assert(check_arith<int*, std::ptrdiff_t>());
static_assert(check_arith<const char*, std::ptrdiff_t>());

// atomic<bool> uses the primary template: no difference_type, no arithmetic.
template<class A> concept has_difference_type = requires { typename A::difference_type; };
template<class A> concept has_fetch_add = requires(A& a) { a.fetch_add(1); };
template<class A> concept has_increment = requires(A& a) { ++a; a++; };
template<class A> concept has_fetch_and = requires(A& a) { a.fetch_and(1); };
static_assert(std::is_same_v<std::atomic<bool>::value_type, bool>);
static_assert(!has_difference_type<std::atomic<bool>>);
static_assert(!has_fetch_add<std::atomic<bool>>);
static_assert(std::is_standard_layout_v<std::atomic<bool>>);
static_assert(std::is_trivially_destructible_v<std::atomic<bool>>);
// floating-point: fetch_add but no bitwise operations and no ++/--
static_assert(has_fetch_add<std::atomic<double>>);
static_assert(!has_fetch_and<std::atomic<double>>);
static_assert(!has_increment<std::atomic<double>>);
// pointers: ++/-- but no bitwise operations
static_assert(has_increment<std::atomic<int*>>);
static_assert(!has_fetch_and<std::atomic<int*>>);
// a user type: no arithmetic
struct S { int a; char b; };
static_assert(!has_difference_type<std::atomic<S>>);
static_assert(!has_fetch_add<std::atomic<S>>);
static_assert(std::is_same_v<std::atomic<S>::value_type, S>);

// [atomics.order]: enum class memory_order { relaxed = 0, acquire = 2, release = 3,
// acq_rel = 4, seq_cst = 5 }; [atomics.syn]: the inline constexpr variables name them.
static_assert(std::is_enum_v<std::memory_order>);
static_assert(std::memory_order_relaxed == std::memory_order::relaxed);
static_assert(std::memory_order_acquire == std::memory_order::acquire);
static_assert(std::memory_order_release == std::memory_order::release);
static_assert(std::memory_order_acq_rel == std::memory_order::acq_rel);
static_assert(std::memory_order_seq_cst == std::memory_order::seq_cst);
static_assert(!std::is_convertible_v<std::memory_order, int>);  // scoped enumeration
static_assert(static_cast<int>(std::memory_order::relaxed) == 0);
static_assert(static_cast<int>(std::memory_order::acquire) == 2);
static_assert(static_cast<int>(std::memory_order::release) == 3);
static_assert(static_cast<int>(std::memory_order::acq_rel) == 4);
static_assert(static_cast<int>(std::memory_order::seq_cst) == 5);

static_assert(std::is_same_v<std::atomic_bool, std::atomic<bool>>);
static_assert(std::is_same_v<std::atomic_char, std::atomic<char>>);
static_assert(std::is_same_v<std::atomic_schar, std::atomic<signed char>>);
static_assert(std::is_same_v<std::atomic_uchar, std::atomic<unsigned char>>);
static_assert(std::is_same_v<std::atomic_short, std::atomic<short>>);
static_assert(std::is_same_v<std::atomic_ushort, std::atomic<unsigned short>>);
static_assert(std::is_same_v<std::atomic_int, std::atomic<int>>);
static_assert(std::is_same_v<std::atomic_uint, std::atomic<unsigned>>);
static_assert(std::is_same_v<std::atomic_long, std::atomic<long>>);
static_assert(std::is_same_v<std::atomic_ulong, std::atomic<unsigned long>>);
static_assert(std::is_same_v<std::atomic_llong, std::atomic<long long>>);
static_assert(std::is_same_v<std::atomic_ullong, std::atomic<unsigned long long>>);
static_assert(std::is_same_v<std::atomic_char8_t, std::atomic<char8_t>>);
static_assert(std::is_same_v<std::atomic_char16_t, std::atomic<char16_t>>);
static_assert(std::is_same_v<std::atomic_char32_t, std::atomic<char32_t>>);
static_assert(std::is_same_v<std::atomic_wchar_t, std::atomic<wchar_t>>);
static_assert(std::is_same_v<std::atomic_int8_t, std::atomic<std::int8_t>>);
static_assert(std::is_same_v<std::atomic_uint64_t, std::atomic<std::uint64_t>>);
static_assert(std::is_same_v<std::atomic_int_least16_t, std::atomic<std::int_least16_t>>);
static_assert(std::is_same_v<std::atomic_uint_fast32_t, std::atomic<std::uint_fast32_t>>);
static_assert(std::is_same_v<std::atomic_intptr_t, std::atomic<std::intptr_t>>);
static_assert(std::is_same_v<std::atomic_uintptr_t, std::atomic<std::uintptr_t>>);
static_assert(std::is_same_v<std::atomic_size_t, std::atomic<std::size_t>>);
static_assert(std::is_same_v<std::atomic_ptrdiff_t, std::atomic<std::ptrdiff_t>>);
static_assert(std::is_same_v<std::atomic_intmax_t, std::atomic<std::intmax_t>>);
static_assert(std::is_same_v<std::atomic_uintmax_t, std::atomic<std::uintmax_t>>);
