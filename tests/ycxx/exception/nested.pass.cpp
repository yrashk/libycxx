// [except.nested]: nested_exception() noexcept captures current_exception(); nested_ptr()
// noexcept returns it; rethrow_nested() throws it. throw_with_nested(t): if U = decay_t<T> is
// a non-final class not derived from nested_exception, throws an exception "publicly derived
// from both U and nested_exception", otherwise throws std::forward<T>(t). rethrow_if_nested(e)
// rethrows the nested exception if e is polymorphic and has an accessible unambiguous
// nested_exception base, otherwise has no effect.
#include <exception>
#include <stdexcept>
#include <type_traits>
#include "check.hpp"

struct Plain {
  int v;
};
struct Poly {
  int v;
  explicit Poly(int x) : v(x) {}
  virtual ~Poly() = default;
};
struct Final final {
  int v;
  explicit Final(int x) : v(x) {}
  virtual ~Final() = default;
};
struct AlreadyNested : std::nested_exception {
  int v = 9;
};

static_assert(std::is_nothrow_default_constructible_v<std::nested_exception>);
static_assert(std::is_nothrow_copy_constructible_v<std::nested_exception>);
static_assert(std::has_virtual_destructor_v<std::nested_exception>);
static_assert(std::is_polymorphic_v<std::nested_exception>);
static_assert(noexcept(std::declval<const std::nested_exception&>().nested_ptr()));

int main() {
  {
    std::nested_exception outside;
    CHECK(outside.nested_ptr() == nullptr);
  }
  int stage = 0;
  try {
    try {
      throw std::runtime_error("inner");
    } catch (...) {
      std::throw_with_nested(Poly(3));
    }
  } catch (const Poly& p) {
    CHECK(p.v == 3);
    const std::nested_exception* n = dynamic_cast<const std::nested_exception*>(&p);
    CHECK(n != nullptr);
    CHECK(n->nested_ptr() != nullptr);
    stage = 1;
    try {
      std::rethrow_if_nested(p);
    } catch (const std::runtime_error& e) {
      CHECK(e.what()[0] == 'i');
      stage = 2;
    }
  }
  CHECK(stage == 2);

  // non-class and final types are thrown as-is
  try {
    std::throw_with_nested(7);
  } catch (const std::nested_exception&) {
    CHECK(false);
  } catch (int i) {
    CHECK(i == 7);
  }
  try {
    std::throw_with_nested(Final(4));
  } catch (const std::nested_exception&) {
    CHECK(false);
  } catch (const Final& f) {
    CHECK(f.v == 4);
  }
  // already derived from nested_exception: thrown as-is
  try {
    std::throw_with_nested(AlreadyNested{});
  } catch (const AlreadyNested& a) {
    CHECK(a.v == 9);
  }
  // rethrow_if_nested on a non-polymorphic type or without a nested_exception base: no effect
  std::rethrow_if_nested(Plain{1});
  std::rethrow_if_nested(Poly(1));
  std::rethrow_if_nested(std::runtime_error("x"));
  std::rethrow_if_nested(42);
  // rethrow_nested via the member
  try {
    try {
      throw 5;
    } catch (...) {
      std::nested_exception ne;
      try {
        ne.rethrow_nested();
      } catch (int i) {
        CHECK(i == 5);
        stage = 3;
      }
    }
  } catch (...) {
    CHECK(false);
  }
  CHECK(stage == 3);
  return 0;
}
