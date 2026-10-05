// [new.delete.single]/7: the nothrow operator new "Default behavior: Calls operator new(size),
// or operator new(size, alignment), respectively. If the call returns normally, returns the
// result of that call. Otherwise, returns a null pointer."
// [new.delete.single]/14: "The functions that have a size parameter forward their other
// parameters to the corresponding function without a size parameter."
// [new.delete.single]/20: nothrow operator delete "Calls operator delete(ptr), or operator
// delete(ptr, alignment), respectively."
// [new.delete.array]: operator new[] "Returns operator new(size), or operator new(size,
// alignment)"; nothrow new[] "Calls operator new[](size), or operator new[](size, alignment)";
// operator delete[] without size "forward[s] their parameters to the corresponding operator
// delete (single-object) function", with size to the one without; nothrow delete[] "Calls
// operator delete[](ptr), or operator delete[](ptr, alignment)".
// Only the four basic functions are replaced here ([dcl.fct.def.replace]); every other form
// must reach them through its default behaviour.
// UNSUPPORTED-SANITIZER: asan  ASan replaces the global allocation functions: its operator new neither calls the new_handler nor throws for impossible sizes, and its other forms do not forward to a program's replacement ([new.delete])
// REQUIRES: exceptions
#include <new>
#include <cstddef>
#include <cstdlib>
#include "check.hpp"

static int news = 0, aligned_news = 0, deletes = 0, aligned_deletes = 0;
static bool fail_next = false;

void* operator new(std::size_t n) {
  ++news;
  if (fail_next) {
    fail_next = false;
    throw std::bad_alloc();
  }
  if (void* p = std::malloc(n ? n : 1)) return p;
  throw std::bad_alloc();
}
void* operator new(std::size_t n, std::align_val_t a) {
  ++aligned_news;
  if (fail_next) {
    fail_next = false;
    throw std::bad_alloc();
  }
  const std::size_t al = static_cast<std::size_t>(a);
  const std::size_t sz = (n + al - 1) / al * al;
  if (void* p = std::aligned_alloc(al, sz ? sz : al)) return p;
  throw std::bad_alloc();
}
void operator delete(void* p) noexcept {
  ++deletes;
  std::free(p);
}
void operator delete(void* p, std::align_val_t) noexcept {
  ++aligned_deletes;
  std::free(p);
}

static bool counts(int n, int an, int d, int ad) {
  bool ok = news == n && aligned_news == an && deletes == d && aligned_deletes == ad;
  news = aligned_news = deletes = aligned_deletes = 0;
  return ok;
}

int main() {
  const auto al = std::align_val_t{64};
  counts(0, 0, 0, 0);

  void* p = ::operator new[](10);
  CHECK(counts(1, 0, 0, 0));
  ::operator delete[](p);
  CHECK(counts(0, 0, 1, 0));

  p = ::operator new(10, std::nothrow);
  CHECK(p && counts(1, 0, 0, 0));
  ::operator delete(p, std::nothrow);
  CHECK(counts(0, 0, 1, 0));

  p = ::operator new[](10, std::nothrow);
  CHECK(p && counts(1, 0, 0, 0));
  ::operator delete[](p, std::nothrow);
  CHECK(counts(0, 0, 1, 0));

  p = ::operator new(10);
  ::operator delete(p, std::size_t(10));
  CHECK(counts(1, 0, 1, 0));
  p = ::operator new[](10);
  ::operator delete[](p, std::size_t(10));
  CHECK(counts(1, 0, 1, 0));

  p = ::operator new[](10, al);
  CHECK(counts(0, 1, 0, 0));
  ::operator delete[](p, al);
  CHECK(counts(0, 0, 0, 1));
  p = ::operator new(10, al, std::nothrow);
  CHECK(p && counts(0, 1, 0, 0));
  ::operator delete(p, al, std::nothrow);
  CHECK(counts(0, 0, 0, 1));
  p = ::operator new[](10, al, std::nothrow);
  CHECK(p && counts(0, 1, 0, 0));
  ::operator delete[](p, al, std::nothrow);
  CHECK(counts(0, 0, 0, 1));
  p = ::operator new(10, al);
  ::operator delete(p, std::size_t(10), al);
  CHECK(counts(0, 1, 0, 1));
  p = ::operator new[](10, al);
  ::operator delete[](p, std::size_t(10), al);
  CHECK(counts(0, 1, 0, 1));

  // nothrow forms turn the replaced function's bad_alloc into a null pointer
  fail_next = true;
  CHECK(::operator new(10, std::nothrow) == nullptr);
  CHECK(counts(1, 0, 0, 0));
  fail_next = true;
  CHECK(::operator new[](10, std::nothrow) == nullptr);
  CHECK(counts(1, 0, 0, 0));
  fail_next = true;
  CHECK(::operator new(10, al, std::nothrow) == nullptr);
  CHECK(counts(0, 1, 0, 0));
  fail_next = true;
  CHECK(::operator new[](10, al, std::nothrow) == nullptr);
  CHECK(counts(0, 1, 0, 0));
  // and the throwing array form propagates it
  fail_next = true;
  bool caught = false;
  try {
    (void)::operator new[](10);
  } catch (const std::bad_alloc&) {
    caught = true;
  }
  CHECK(caught && counts(1, 0, 0, 0));
  return 0;
}
