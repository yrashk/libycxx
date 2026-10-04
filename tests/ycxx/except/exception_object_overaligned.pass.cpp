// [except.throw]/3: "Throwing an exception initializes a temporary object, called the exception
// object"; /4: "The memory for the exception object is allocated in an unspecified way".
// [basic.align]/1: "Object types have alignment requirements which place restrictions on the
// addresses at which an object of that type may be allocated." /3: whether extended alignments
// are supported (and in which contexts) is implementation-defined. Both compilers accept
// alignas(32) and alignas(64) class types in throw-expressions (GCC itself emits aligned vector
// stores into a freshly allocated alignas(64) exception object), so every object of them,
// including an exception object and the object referred to by an exception_ptr ([propagation]/7: make_exception_ptr / current_exception
// refer to an exception object), must meet that alignment. [except.handle]/16: a handler
// catching by reference refers to the exception object itself.
// [except.nested]/8: throw_with_nested throws an object of a type derived from both U and
// nested_exception, which therefore has at least U's alignment.
#include <exception>
#include <cstddef>
#include <cstdint>
#include "check.hpp"

template <std::size_t A> struct alignas(A) Over {
  int v;
  explicit Over(int x) : v(x) {}
  virtual ~Over() = default;
};
template <std::size_t A> struct alignas(A) OverFinal final { int v; };

template <class T> bool is_aligned(const T* p) { return reinterpret_cast<std::uintptr_t>(p) % alignof(T) == 0; }

template <std::size_t A> void run() {
  static_assert(alignof(Over<A>) == A);
  bool caught = false;
  try {
    throw Over<A>(7);
  } catch (Over<A>& e) {
    CHECK(is_aligned(&e) && e.v == 7);
    caught = true;
    try {
      throw;   // the same exception object
    } catch (const Over<A>& again) {
      CHECK(&again == &e);
    }
  }
  CHECK(caught);
  // Several live exception objects at once.
  try {
    throw OverFinal<A>{1};
  } catch (OverFinal<A>& a) {
    CHECK(is_aligned(&a));
    try {
      throw OverFinal<A>{2};
    } catch (OverFinal<A>& b) {
      CHECK(is_aligned(&b) && &a != &b && b.v == 2);
    }
    CHECK(a.v == 1);
  }
  // exception_ptr.
  std::exception_ptr p = std::make_exception_ptr(Over<A>(9));
  try {
    std::rethrow_exception(p);
  } catch (Over<A>& e) {
    CHECK(is_aligned(&e) && e.v == 9);
  }
  try {
    throw OverFinal<A>{4};
  } catch (...) {
    p = std::current_exception();
  }
  try {
    std::rethrow_exception(p);
  } catch (OverFinal<A>& e) {
    CHECK(is_aligned(&e) && e.v == 4);
  }
  // throw_with_nested: the thrown object derives from Over<A>.
  try {
    try {
      throw 1;
    } catch (...) {
      std::throw_with_nested(Over<A>(5));
    }
  } catch (Over<A>& e) {
    CHECK(is_aligned(&e) && e.v == 5);
    CHECK(dynamic_cast<std::nested_exception*>(&e) != nullptr);
  }
}

int main() {
  run<32>();
  run<64>();
  // (Larger extended alignments are not tested: the Itanium ABI's __cxa_allocate_exception
  // receives no alignment, so their support is an implementation-defined limit.)
  return 0;
}
