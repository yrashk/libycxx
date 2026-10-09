// [atomics.types.operations], [atomics.types.int], [atomics.types.pointer], [atomics.flag],
// [atomics.ref.ops]: in C++26 the non-volatile operations of atomic, atomic_flag and
// atomic_ref are constexpr ("constexpr T load(...) const noexcept;" etc.), as are the
// non-member functions and the fences ([atomics.syn]).
// FLAGS: -latomic -Wno-deprecated-declarations
// (-latomic: the toolchain's out-of-line atomics for types that are not lock-free; Clang does not link it implicitly)
#include <atomic>

constexpr bool integral() {
  std::atomic<int> a(1);
  a.store(2);
  if (a.load() != 2) return false;
  if (a.exchange(3) != 2) return false;
  int e = 3;
  if (!a.compare_exchange_strong(e, 4)) return false;
  e = 0;
  if (a.compare_exchange_strong(e, 5) || e != 4) return false;
  while (!a.compare_exchange_weak(e, 5)) {}
  if (a.fetch_add(2) != 5 || a.fetch_sub(1) != 7) return false;
  if (a.fetch_and(3) != 6 || a.fetch_or(8) != 2 || a.fetch_xor(1) != 10) return false;
  if (++a != 12 || a-- != 12 || (a += 4) != 15 || static_cast<int>(a) != 15) return false;
  a.wait(0);
  a.notify_one();
  a.notify_all();
  std::atomic_store(&a, 1);
  if (std::atomic_load(&a) != 1 || std::atomic_fetch_add(&a, 1) != 1) return false;
  std::atomic_thread_fence(std::memory_order::seq_cst);
  std::atomic_signal_fence(std::memory_order::seq_cst);
  return a.load() == 2;
}
static_assert(integral());

constexpr bool floating() {
  std::atomic<double> d(1.0);
  d.fetch_add(0.5);
  d -= 0.25;
  return d.load() == 1.25;
}
static_assert(floating());

constexpr bool pointer() {
  int arr[4] = {};
  std::atomic<int*> p(arr);
  p.fetch_add(2);
  ++p;
  return p.load() == arr + 3 && p.fetch_sub(3) == arr + 3 && p.load() == arr;
}
static_assert(pointer());

constexpr bool user() {
  struct S { int a; int b; };
  std::atomic<S> s(S{1, 2});
  S old = s.exchange(S{3, 4});
  return old.a == 1 && s.load().b == 4;
}
static_assert(user());

constexpr bool flag() {
  std::atomic_flag f;
  if (f.test() || f.test_and_set() || !f.test()) return false;
  f.clear();
  return !f.test();
}
static_assert(flag());

constexpr bool ref() {
  alignas(std::atomic_ref<int>::required_alignment) int x = 1;
  std::atomic_ref<int> r(x);
  r.fetch_add(4);
  r.store(r.load() * 2);
  int e = 10;
  return r.compare_exchange_strong(e, 11) && r.load() == 11;
}
static_assert(ref());

// [depr.atomics.order]/2: template<class T> constexpr T kill_dependency(T y) noexcept;
static_assert(std::kill_dependency(7) == 7);

constinit std::atomic<int> global(7);
constinit std::atomic_flag gflag;

int main() { return global.load() == 7 && !gflag.test() ? 0 : 1; }
