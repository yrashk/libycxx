// [type.info]: type_info has constexpr bool operator==(const type_info&) const noexcept
// (C++23), before() and hash_code() noexcept, name() noexcept; it is not copy constructible or
// copy assignable. [expr.typeid]: top-level cv-qualifiers and references are ignored.
// [bad.cast], [bad.typeid]: derive from exception.
#include <typeinfo>
#include <cstddef>
#include <exception>
#include <type_traits>
#include "check.hpp"

struct Poly {
  virtual ~Poly() = default;
};
struct Der : Poly {};

static_assert(!std::is_copy_constructible_v<std::type_info>);
static_assert(!std::is_copy_assignable_v<std::type_info>);
static_assert(std::has_virtual_destructor_v<std::type_info>);
static_assert(noexcept(typeid(int) == typeid(int)));
static_assert(noexcept(typeid(int).before(typeid(long))));
static_assert(noexcept(typeid(int).hash_code()));
static_assert(noexcept(typeid(int).name()));
static_assert(std::is_same_v<decltype(typeid(int).hash_code()), std::size_t>);
static_assert(std::is_base_of_v<std::exception, std::bad_cast>);
static_assert(std::is_base_of_v<std::exception, std::bad_typeid>);

// constexpr comparison
static_assert(typeid(int) == typeid(int));
static_assert(typeid(int) != typeid(long));
static_assert(typeid(const int&) == typeid(int));
static_assert(typeid(int*) != typeid(const int*));

int main() {
  CHECK(typeid(int).hash_code() == typeid(const int).hash_code());
  CHECK(typeid(int).before(typeid(long)) != typeid(long).before(typeid(int)));
  CHECK(!typeid(int).before(typeid(int)));
  CHECK(typeid(int).name() != nullptr);
  Der d;
  Poly& p = d;
  CHECK(typeid(p) == typeid(Der));
  bool caught = false;
  try {
    Poly* np = nullptr;
    (void)typeid(*np);
  } catch (const std::bad_typeid& e) {
    caught = e.what() != nullptr;
  }
  CHECK(caught);
  caught = false;
  try {
    Poly base;
    (void)dynamic_cast<Der&>(base);
  } catch (const std::bad_cast&) {
    caught = true;
  }
  CHECK(caught);
  return 0;
}
