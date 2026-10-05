// [new.delete.single]/3: operator new default behavior: "Executes a loop: Within the loop,
// the function first attempts to allocate the requested storage. ... Returns a pointer to the
// allocated storage if the attempt is successful. Otherwise, if the current new_handler
// ([get.new.handler]) is a null pointer value, throws bad_alloc. Otherwise, the function
// calls the current new_handler function. If the called function returns, the loop repeats.
// The loop terminates when an attempt to allocate the requested storage is successful or
// when a called new_handler function does not return."
// [new.delete.single]/7: nothrow forms return a null pointer when operator new throws.
// [set.new.handler]/3: set_new_handler "Returns: The previous new_handler." /2: "If new_p is
// the null pointer value, the null pointer value is stored". [get.new.handler]: returns the
// current one. An allocation of nearly SIZE_MAX bytes cannot succeed.
// UNSUPPORTED-SANITIZER: asan  ASan replaces the global allocation functions: its operator new neither calls the new_handler nor throws for impossible sizes, and its other forms do not forward to a program's replacement ([new.delete])
#include <new>
#include <cstddef>
#include <cstdint>
#include "check.hpp"

static int calls = 0;
static void give_up_after_three() {
  if (++calls == 3) std::set_new_handler(nullptr);
}
static void throwing_handler() {
  ++calls;
  throw std::bad_alloc();
}
struct Marker {};
static void throwing_other() {
  ++calls;
  throw Marker();
}

static const std::size_t huge = SIZE_MAX - 4096;

template <class F>
static bool throws_bad_alloc(F f) {
  try {
    f();
  } catch (const std::bad_alloc&) {
    return true;
  }
  return false;
}

int main() {
  CHECK(std::get_new_handler() == nullptr);
  CHECK(std::set_new_handler(give_up_after_three) == nullptr);
  CHECK(std::get_new_handler() == give_up_after_three);

  // the handler is called until it removes itself, then bad_alloc
  calls = 0;
  CHECK(throws_bad_alloc([] { (void)::operator new(huge); }));
  CHECK(calls == 3 && std::get_new_handler() == nullptr);

  std::set_new_handler(give_up_after_three);
  calls = 0;
  CHECK(throws_bad_alloc([] { (void)::operator new[](huge); }));
  CHECK(calls == 3);

  std::set_new_handler(give_up_after_three);
  calls = 0;
  CHECK(throws_bad_alloc([] { (void)::operator new(huge, std::align_val_t{64}); }));
  CHECK(calls == 3);

  // a handler that throws ends the loop; the exception propagates
  std::set_new_handler(throwing_other);
  calls = 0;
  bool marker = false;
  try {
    (void)::operator new(huge);
  } catch (Marker) {
    marker = true;
  }
  CHECK(marker && calls == 1);

  // nothrow: the handler still runs; bad_alloc becomes a null pointer
  CHECK(std::set_new_handler(throwing_handler) == throwing_other);
  calls = 0;
  CHECK(::operator new(huge, std::nothrow) == nullptr && calls == 1);
  calls = 0;
  CHECK(::operator new[](huge, std::nothrow) == nullptr && calls == 1);
  calls = 0;
  CHECK(::operator new(huge, std::align_val_t{32}, std::nothrow) == nullptr && calls == 1);

  // no handler: straight to bad_alloc / null
  std::set_new_handler(nullptr);
  calls = 0;
  CHECK(throws_bad_alloc([] { (void)::operator new(huge); }) && calls == 0);
  CHECK(::operator new(huge, std::nothrow) == nullptr);

  // the handler is not called when allocation succeeds
  std::set_new_handler(throwing_handler);
  calls = 0;
  void* p = ::operator new(16);
  CHECK(p != nullptr && calls == 0);
  ::operator delete(p);
  std::set_new_handler(nullptr);
  return 0;
}
