// [atomics.nonmembers]: "A non-member function template whose name matches the pattern
// atomic_f or the pattern atomic_f_explicit invokes the member function f, with the value of
// the first parameter as the object expression and the values of the remaining parameters (if
// any) as the arguments of the member function call, in order." The value parameters are of
// type typename atomic<T>::value_type / difference_type (non-deduced).
#include <atomic>
#include "check.hpp"

int main() {
  std::atomic<long> a(0);
  std::atomic_store(&a, 5);
  CHECK(std::atomic_load(&a) == 5);
  std::atomic_store_explicit(&a, 6, std::memory_order::release);
  CHECK(std::atomic_load_explicit(&a, std::memory_order::acquire) == 6);
  CHECK(std::atomic_exchange(&a, 7) == 6);
  CHECK(std::atomic_exchange_explicit(&a, 8, std::memory_order::acq_rel) == 7);
  long e = 0;
  CHECK(!std::atomic_compare_exchange_strong(&a, &e, 9));
  CHECK(e == 8);
  CHECK(std::atomic_compare_exchange_strong(&a, &e, 9));
  e = 9;
  while (!std::atomic_compare_exchange_weak(&a, &e, 10)) CHECK(e == 9);
  e = 10;
  CHECK(std::atomic_compare_exchange_strong_explicit(&a, &e, 11, std::memory_order::seq_cst,
                                                     std::memory_order::relaxed));
  e = 11;
  while (!std::atomic_compare_exchange_weak_explicit(&a, &e, 12, std::memory_order::acq_rel,
                                                     std::memory_order::acquire))
    CHECK(e == 11);
  CHECK(a.load() == 12);
  CHECK(std::atomic_fetch_add(&a, 3) == 12);  // int argument converts to long (non-deduced)
  CHECK(std::atomic_fetch_sub(&a, 5) == 15);
  CHECK(std::atomic_fetch_and(&a, 6) == 10);
  CHECK(std::atomic_fetch_or(&a, 1) == 2);
  CHECK(std::atomic_fetch_xor(&a, 2) == 3);
  CHECK(std::atomic_fetch_add_explicit(&a, 1, std::memory_order::relaxed) == 1);
  CHECK(std::atomic_fetch_sub_explicit(&a, 1, std::memory_order::relaxed) == 2);
  CHECK(std::atomic_fetch_and_explicit(&a, 1, std::memory_order::relaxed) == 1);
  CHECK(std::atomic_fetch_or_explicit(&a, 4, std::memory_order::relaxed) == 1);
  CHECK(std::atomic_fetch_xor_explicit(&a, 4, std::memory_order::relaxed) == 5);
  CHECK(a.load() == 1);

  volatile std::atomic<int> v(1);
  std::atomic_store(&v, 2);
  CHECK(std::atomic_load(&v) == 2);
  CHECK(std::atomic_fetch_add(&v, 1) == 2);

  int arr[4] = {};
  std::atomic<int*> p(arr);
  CHECK(std::atomic_fetch_add(&p, 2) == arr);
  CHECK(std::atomic_fetch_sub_explicit(&p, 1, std::memory_order::relaxed) == arr + 2);
  CHECK(p.load() == arr + 1);

  std::atomic<double> d(1.0);
  CHECK(std::atomic_fetch_add(&d, 0.5) == 1.0);
  CHECK(std::atomic_fetch_sub(&d, 1.0) == 1.5);
  CHECK(std::atomic_load(&d) == 0.5);

  std::atomic<bool> b(false);
  CHECK(!std::atomic_exchange(&b, true));
  CHECK(std::atomic_load(&b));
  return 0;
}
