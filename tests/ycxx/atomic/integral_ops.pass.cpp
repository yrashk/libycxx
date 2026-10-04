// [atomics.types.operations], [atomics.types.int]/6-7,15, [atomics.types.memop]:
// store/load/exchange; fetch_key returns the value "immediately before the effects";
// "operator op=: Equivalent to: return fetch_key(operand) op operand;"; ++a / a++ return the
// new / old value; operator= returns desired; the conversion is load().
#include <atomic>
#include <climits>
#include "check.hpp"

template<class T>
void run() {
  std::atomic<T> a;
  CHECK(a.load() == T());         // value-initialized: atomic() initializes with T()
  std::atomic<T> b(T(5));
  CHECK(b.load() == T(5));
  CHECK((b = T(7)) == T(7));
  CHECK(static_cast<T>(b) == T(7));
  b.store(T(9), std::memory_order::relaxed);
  CHECK(b.load(std::memory_order::acquire) == T(9));
  CHECK(b.exchange(T(3)) == T(9));
  CHECK(b.load() == T(3));

  CHECK(b.fetch_add(T(4)) == T(3));
  CHECK(b.load() == T(7));
  CHECK(b.fetch_sub(T(2), std::memory_order::acq_rel) == T(7));
  CHECK(b.load() == T(5));
  CHECK(b.fetch_and(T(4)) == T(5));
  CHECK(b.load() == T(4));
  CHECK(b.fetch_or(T(3)) == T(4));
  CHECK(b.load() == T(7));
  CHECK(b.fetch_xor(T(5)) == T(7));
  CHECK(b.load() == T(2));

  CHECK((b += T(10)) == T(12));
  CHECK((b -= T(2)) == T(10));
  CHECK((b &= T(6)) == T(2));
  CHECK((b |= T(5)) == T(7));
  CHECK((b ^= T(1)) == T(6));
  CHECK(++b == T(7));
  CHECK(b++ == T(7));
  CHECK(b.load() == T(8));
  CHECK(--b == T(7));
  CHECK(b-- == T(7));
  CHECK(b.load() == T(6));

  volatile std::atomic<T> v(T(1));
  if constexpr (std::atomic<T>::is_always_lock_free) {
    CHECK(v.fetch_add(T(1)) == T(1));
    CHECK((v += T(1)) == T(3));
    CHECK(v.load() == T(3));
    CHECK(++v == T(4));
  }
}

int main() {
  run<char>();
  run<signed char>();
  run<unsigned char>();
  run<short>();
  run<unsigned short>();
  run<int>();
  run<unsigned>();
  run<long>();
  run<unsigned long>();
  run<long long>();
  run<unsigned long long>();
  run<char8_t>();
  run<char16_t>();
  run<char32_t>();
  run<wchar_t>();

  // unsigned arithmetic wraps
  std::atomic<unsigned> u(0);
  CHECK(u.fetch_sub(1) == 0);
  CHECK(u.load() == UINT_MAX);
  CHECK(++u == 0);
  return 0;
}
