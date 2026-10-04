// [atomics.lockfree]/1: the ATOMIC_..._LOCK_FREE macros are 0 (never), 1 (sometimes) or 2
// (always lock-free), "The properties also apply to the corresponding (partial)
// specializations of the atomic template"; /2: on a hosted implementation at least one signed
// integral specialization and its unsigned counterpart is always lock-free; /3: "In any given
// program execution, the result of the lock-free query is the same for all atomic objects of the
// same type." [atomics.types.operations]/4-5: is_always_lock_free / is_lock_free.
// [atomics.alias]: atomic_signed_lock_free / atomic_unsigned_lock_free are always lock-free
// signed / unsigned integral specializations. [atomics.flag]/2: atomic_flag is lock-free.
// [atomics.nonmembers]: atomic_is_lock_free.
#include <atomic>
#include <concepts>
#include <type_traits>
#include "check.hpp"

#define IN_RANGE(M) static_assert((M) == 0 || (M) == 1 || (M) == 2, #M)
IN_RANGE(ATOMIC_BOOL_LOCK_FREE);
IN_RANGE(ATOMIC_CHAR_LOCK_FREE);
IN_RANGE(ATOMIC_CHAR8_T_LOCK_FREE);
IN_RANGE(ATOMIC_CHAR16_T_LOCK_FREE);
IN_RANGE(ATOMIC_CHAR32_T_LOCK_FREE);
IN_RANGE(ATOMIC_WCHAR_T_LOCK_FREE);
IN_RANGE(ATOMIC_SHORT_LOCK_FREE);
IN_RANGE(ATOMIC_INT_LOCK_FREE);
IN_RANGE(ATOMIC_LONG_LOCK_FREE);
IN_RANGE(ATOMIC_LLONG_LOCK_FREE);
IN_RANGE(ATOMIC_POINTER_LOCK_FREE);

// 2 means always lock-free, 0 means never.
template<class T, int M>
void consistent() {
  using A = std::atomic<T>;
  static_assert(M != 2 || A::is_always_lock_free);
  static_assert(M != 0 || !A::is_always_lock_free);
  A a{}, b{};
  CHECK(a.is_lock_free() == b.is_lock_free());
  if constexpr (A::is_always_lock_free) CHECK(a.is_lock_free());
  if constexpr (M == 0) CHECK(!a.is_lock_free());
  CHECK(std::atomic_is_lock_free(&a) == a.is_lock_free());
  const volatile A* cv = &a;
  CHECK(cv->is_lock_free() == a.is_lock_free());
}

using ssigned = std::atomic_signed_lock_free::value_type;
using sunsigned = std::atomic_unsigned_lock_free::value_type;
static_assert(std::is_same_v<std::atomic_signed_lock_free, std::atomic<ssigned>>);
static_assert(std::is_same_v<std::atomic_unsigned_lock_free, std::atomic<sunsigned>>);
static_assert(std::signed_integral<ssigned>);
static_assert(std::unsigned_integral<sunsigned>);
static_assert(std::atomic_signed_lock_free::is_always_lock_free);
static_assert(std::atomic_unsigned_lock_free::is_always_lock_free);

int main() {
  consistent<bool, ATOMIC_BOOL_LOCK_FREE>();
  consistent<char, ATOMIC_CHAR_LOCK_FREE>();
  consistent<signed char, ATOMIC_CHAR_LOCK_FREE>();
  consistent<unsigned char, ATOMIC_CHAR_LOCK_FREE>();
  consistent<char8_t, ATOMIC_CHAR8_T_LOCK_FREE>();
  consistent<char16_t, ATOMIC_CHAR16_T_LOCK_FREE>();
  consistent<char32_t, ATOMIC_CHAR32_T_LOCK_FREE>();
  consistent<wchar_t, ATOMIC_WCHAR_T_LOCK_FREE>();
  consistent<short, ATOMIC_SHORT_LOCK_FREE>();
  consistent<unsigned short, ATOMIC_SHORT_LOCK_FREE>();
  consistent<int, ATOMIC_INT_LOCK_FREE>();
  consistent<unsigned, ATOMIC_INT_LOCK_FREE>();
  consistent<long, ATOMIC_LONG_LOCK_FREE>();
  consistent<unsigned long, ATOMIC_LONG_LOCK_FREE>();
  consistent<long long, ATOMIC_LLONG_LOCK_FREE>();
  consistent<unsigned long long, ATOMIC_LLONG_LOCK_FREE>();
  consistent<int*, ATOMIC_POINTER_LOCK_FREE>();
  consistent<void*, ATOMIC_POINTER_LOCK_FREE>();

  // at least one signed/unsigned pair is always lock-free
  static_assert((std::atomic<signed char>::is_always_lock_free && std::atomic<unsigned char>::is_always_lock_free) ||
                (std::atomic<short>::is_always_lock_free && std::atomic<unsigned short>::is_always_lock_free) ||
                (std::atomic<int>::is_always_lock_free && std::atomic<unsigned>::is_always_lock_free) ||
                (std::atomic<long>::is_always_lock_free && std::atomic<unsigned long>::is_always_lock_free) ||
                (std::atomic<long long>::is_always_lock_free && std::atomic<unsigned long long>::is_always_lock_free));

  std::atomic_signed_lock_free s(-3);
  CHECK(s.fetch_add(4) == -3);
  CHECK(s.load() == 1);
  std::atomic_unsigned_lock_free u(3);
  u.wait(4);  // returns at once: the value differs
  CHECK(u.exchange(9) == 3);
  return 0;
}
