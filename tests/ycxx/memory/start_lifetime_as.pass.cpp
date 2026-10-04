// [obj.lifetime]/3-10: start_lifetime_as<T>(p) implicitly creates an object of type T at p
// whose object representation is the previous contents of the storage and returns a
// pointer to it; start_lifetime_as_array<T>(p, n) does the same for "array of n T", and
// for n == 0 returns a pointer that compares equal to p (p may be null then).
#include <cstddef>
#include <cstring>
#include <memory>
#include <type_traits>
#include "check.hpp"

struct Pod { int a; float b; short c[3]; };

// Return types for each cv-qualified overload.
static_assert(std::is_same_v<decltype(std::start_lifetime_as<int>(std::declval<void*>())), int*>);
static_assert(std::is_same_v<decltype(std::start_lifetime_as<int>(std::declval<const void*>())), const int*>);
static_assert(std::is_same_v<decltype(std::start_lifetime_as<int>(std::declval<volatile void*>())), volatile int*>);
static_assert(std::is_same_v<decltype(std::start_lifetime_as<int>(std::declval<const volatile void*>())), const volatile int*>);
static_assert(std::is_same_v<decltype(std::start_lifetime_as_array<Pod>(std::declval<void*>(), 1)), Pod*>);
static_assert(std::is_same_v<decltype(std::start_lifetime_as_array<Pod>(std::declval<const void*>(), 1)), const Pod*>);
static_assert(std::is_same_v<decltype(std::start_lifetime_as_array<int>(std::declval<volatile void*>(), 1)), volatile int*>);
static_assert(std::is_same_v<decltype(std::start_lifetime_as_array<int>(std::declval<const volatile void*>(), 1)), const volatile int*>);
static_assert(noexcept(std::start_lifetime_as<int>(std::declval<void*>())));
static_assert(noexcept(std::start_lifetime_as_array<int>(std::declval<void*>(), 1)));

int main() {
  {
    alignas(int) unsigned char buf[sizeof(int)];
    const int v = 0x12345678;
    std::memcpy(buf, &v, sizeof v);
    int* p = std::start_lifetime_as<int>(buf);
    CHECK(static_cast<void*>(p) == static_cast<void*>(buf));
    CHECK(*p == 0x12345678);                  // value from the prior object representation
    *p = 7;
    CHECK(*p == 7);
  }
  {
    alignas(Pod) unsigned char buf[sizeof(Pod)];
    Pod src{1, 2.5f, {3, 4, 5}};
    std::memcpy(buf, &src, sizeof src);
    const void* cv = buf;
    const Pod* p = std::start_lifetime_as<Pod>(cv);
    CHECK(static_cast<const void*>(p) == cv);
    CHECK(p->a == 1 && p->b == 2.5f && p->c[0] == 3 && p->c[2] == 5);
  }
  {
    alignas(double) unsigned char buf[4 * sizeof(double)];
    const double src[4] = {1.0, -2.0, 3.5, 0.25};
    std::memcpy(buf, src, sizeof src);
    double* p = std::start_lifetime_as_array<double>(buf, 4);
    CHECK(static_cast<void*>(p) == static_cast<void*>(buf));
    CHECK(p[0] == 1.0 && p[1] == -2.0 && p[2] == 3.5 && p[3] == 0.25);
    p[3] = 9.0;
    CHECK(p[3] == 9.0);
  }
  {
    // Arrays of an array type.
    alignas(int) unsigned char buf[2 * sizeof(int[3])];
    const int src[2][3] = {{1, 2, 3}, {4, 5, 6}};
    std::memcpy(buf, src, sizeof src);
    int (*rows)[3] = std::start_lifetime_as_array<int[3]>(buf, 2);
    CHECK(rows[1][2] == 6 && rows[0][1] == 2);
    int (*whole)[2][3] = std::start_lifetime_as<int[2][3]>(buf);
    CHECK((*whole)[1][0] == 4);
  }
  {
    // n == 0: no effects; result compares equal to p, which may be null.
    alignas(int) unsigned char buf[sizeof(int)];
    CHECK(static_cast<void*>(std::start_lifetime_as_array<int>(buf, 0)) == static_cast<void*>(buf));
    CHECK(std::start_lifetime_as_array<int>(static_cast<void*>(nullptr), 0) == nullptr);
    CHECK(std::start_lifetime_as_array<int>(static_cast<const void*>(nullptr), 0) == nullptr);
  }
  {
    // Storage obtained from operator new.
    void* raw = ::operator new(sizeof(int) * 3);
    int init[3] = {7, 8, 9};
    std::memcpy(raw, init, sizeof init);
    int* p = std::start_lifetime_as_array<int>(raw, 3);
    CHECK(p[0] + p[1] + p[2] == 24);
    ::operator delete(raw);
  }
  {
    // volatile overload.
    alignas(int) unsigned char buf[sizeof(int)];
    const int v = 41;
    std::memcpy(buf, &v, sizeof v);
    volatile void* vp = buf;
    volatile int* p = std::start_lifetime_as<int>(vp);
    CHECK(*p == 41);
  }
  return 0;
}
