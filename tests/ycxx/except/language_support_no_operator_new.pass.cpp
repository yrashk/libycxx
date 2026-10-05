// The core-language operations that the implementation supports at run time (throwing and
// catching, rethrowing, dynamic_cast and typeid with their bad_cast / bad_typeid exceptions,
// thread-safe initialization of block-scope statics, thread_local variables with dynamic
// initialization and destructors) never call a global allocation function, including the first
// time they happen in a thread:
//   [basic.stc.dynamic.allocation]/5: "A global allocation function is only called as the
//     result of a new expression, or called directly using the function call syntax, or called
//     indirectly to allocate storage for a coroutine state, or called indirectly through calls
//     to the functions in the C++ standard library. [Note 2: In particular, a global
//     allocation function is not called to allocate storage for objects with static storage
//     duration, for objects or references with thread storage duration, for objects of type
//     std::type_info, ..., or for an exception object ([except.throw]).]"
//   [except.throw]/4: "The memory for the exception object is allocated in an unspecified way,
//     except as noted in [basic.stc.dynamic.allocation]".
// The program replaces operator new (every form, [replacement.functions]); while a thread-local
// flag is set, any call is counted. The flag is set only around code with no new-expression and
// no call of a standard library function, in the main thread and in threads started for the
// purpose (their first exception, first static initialization and first thread_local).
// Large and over-aligned exception objects are included (no small fixed pool can hold them).
// FLAGS: -pthread
// REQUIRES: exceptions
#include <cstddef>
#include <cstdlib>
#include <new>
#include <thread>
#include <typeinfo>
#include "check.hpp"

static thread_local bool watching = false;  // constant-initialized
static int forbidden_calls = 0;              // read after the watched code, in the same thread

static void* allocate(std::size_t n, std::size_t align) {
  if (watching) __atomic_fetch_add(&forbidden_calls, 1, __ATOMIC_RELAXED);
  if (n == 0) n = 1;
  void* p = align <= alignof(std::max_align_t) ? std::malloc(n) : std::aligned_alloc(align, (n + align - 1) / align * align);
  if (!p) throw std::bad_alloc();
  return p;
}
void* operator new(std::size_t n) { return allocate(n, 0); }
void* operator new[](std::size_t n) { return allocate(n, 0); }
void* operator new(std::size_t n, std::align_val_t a) { return allocate(n, std::size_t(a)); }
void* operator new[](std::size_t n, std::align_val_t a) { return allocate(n, std::size_t(a)); }
void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
  try {
    return allocate(n, 0);
  } catch (...) {
    return nullptr;
  }
}
void* operator new[](std::size_t n, const std::nothrow_t& t) noexcept { return operator new(n, t); }
void* operator new(std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept {
  try {
    return allocate(n, std::size_t(a));
  } catch (...) {
    return nullptr;
  }
}
void* operator new[](std::size_t n, std::align_val_t a, const std::nothrow_t& t) noexcept { return operator new(n, a, t); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }

struct Base {
  virtual ~Base() {}
};
struct Left : virtual Base {
  int l = 1;
};
struct Right : virtual Base {
  int r = 2;
};
struct Bottom : Left, Right {
  int b = 3;
};
struct Unrelated : Base {};

struct Big {  // larger than any small emergency buffer
  char bytes[64 * 1024];
  int tag;
};
struct alignas(64) Aligned {  // (its alignment: except/exception_object_overaligned)
  int tag;
};

struct Sentinel {
  int* hits;
  ~Sentinel() { ++*hits; }
};

struct WithDtor {
  int v;
  explicit WithDtor(int x) : v(x) {}
  ~WithDtor() { v = -1; }
};
static int next_value() {
  static int n = 0;
  return ++n;
}

// Everything below runs with `watching` set; it uses no new-expression and no library function.
static int language_operations(int seed) {
  int ok = 0;
  // throw / catch of scalar, class, large and over-aligned objects; catch by base reference.
  try {
    throw seed;
  } catch (int v) {
    ok += v == seed;
  }
  try {
    throw Bottom();
  } catch (const Left& l) {
    ok += l.l == 1;
  }
  try {
    throw Big{{}, seed};
  } catch (const Big& b) {
    ok += b.tag == seed;
  }
  try {
    throw Aligned{seed};
  } catch (const Aligned& a) {
    ok += a.tag == seed;
  }
  // Rethrow, nested handling (an exception thrown and caught while another is active), and
  // unwinding through frames with destructors.
  int hits = 0;
  try {
    try {
      Sentinel s{&hits};
      throw Bottom();
    } catch (Base&) {
      try {
        throw 5;
      } catch (int) {
        ++hits;
      }
      throw;
    }
  } catch (const Right& r) {
    ok += r.r == 2 && hits == 2;
  }
  // dynamic_cast: down, cross, to void*, failed pointer cast, failed reference cast (bad_cast).
  Bottom bottom;
  Base* bp = &bottom;
  Left* lp = &bottom;
  ok += dynamic_cast<Bottom*>(bp) == &bottom;
  ok += dynamic_cast<Right*>(lp) == static_cast<Right*>(&bottom);
  ok += dynamic_cast<void*>(lp) == static_cast<void*>(&bottom);
  ok += dynamic_cast<Unrelated*>(bp) == nullptr;
  try {
    (void)dynamic_cast<Unrelated&>(*bp);
  } catch (const std::bad_cast&) {
    ++ok;
  }
  // typeid of a polymorphic glvalue, of a type, and of a null pointer dereference (bad_typeid).
  const std::type_info& t1 = typeid(*bp);
  const std::type_info& t2 = typeid(Bottom);
  ok += &t1 == &t1 && &t2 == &t2;  // (operator== is a library function: not called here)
  Base* null_base = nullptr;
  try {
    (void)typeid(*null_base);
  } catch (const std::bad_typeid&) {
    ++ok;
  }
  // Block-scope static and thread_local variables with dynamic initialization and destructors.
  static WithDtor s(next_value());
  thread_local WithDtor t(seed);
  ok += s.v == 1 && t.v >= 7;  // (initialized by the first call in this thread)
  return ok;
}
constexpr int all_ok = 13;

static int watched(int seed) {
  watching = true;
  const int ok = language_operations(seed);
  watching = false;
  return ok;
}

int main() {
  // First uses in the main thread.
  CHECK(watched(7) == all_ok);
  CHECK(forbidden_calls == 0);
  // First uses in other threads, several of them at once.
  int results[4] = {};
  {
    std::thread ts[4];
    for (int i = 0; i < 4; ++i) ts[i] = std::thread([&results, i] { results[i] = watched(100 + i); });
    for (auto& t : ts) t.join();
  }
  for (int r : results) CHECK(r == all_ok);
  CHECK(__atomic_load_n(&forbidden_calls, __ATOMIC_RELAXED) == 0);
  // Once more in the main thread (nothing cached by the first round changes the answer).
  CHECK(watched(9) == all_ok);
  CHECK(forbidden_calls == 0);
  return 0;
}
