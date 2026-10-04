// [any.class.general]/3: "Implementations should avoid the use of dynamically allocated memory
// for a small contained value." [Example 1: "A contained value of type int could be stored in
// an internal buffer, not in separately-allocated memory."] This is recommended practice
// (a "should"); a failure here is a quality-of-implementation finding, not non-conformance.
#include <any>
#include <new>
#include "check.hpp"

extern "C" void* malloc(decltype(sizeof 0));
extern "C" void free(void*);

static int allocations = 0;
void* operator new(std::size_t n) {
  ++allocations;
  if (void* p = malloc(n ? n : 1)) return p;
  throw std::bad_alloc();
}
void operator delete(void* p) noexcept { free(p); }
void operator delete(void* p, std::size_t) noexcept { free(p); }

struct Small {
  void* p;
  Small() : p(nullptr) {}
  Small(const Small&) = default;
  Small(Small&&) noexcept = default;
};

int main() {
  allocations = 0;
  {
    std::any a = 42;
    std::any b = a;
    std::any c = std::move(b);
    std::any d = Small{};
    std::any e(static_cast<void*>(nullptr));
    a.swap(d);
  }
  CHECK(allocations == 0);
  return 0;
}
