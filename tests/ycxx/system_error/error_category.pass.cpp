// [syserr.errcat.overview]: error_category has a constexpr noexcept default constructor, a
// virtual destructor, deleted copy operations, pure virtual name() and message(), and
// non-virtual operator== and operator<=>.
// [syserr.errcat.virtuals]/2-4: default_error_condition(ev) returns error_condition(ev, *this);
// equivalent(code, condition) returns default_error_condition(code) == condition;
// equivalent(code, condition) (error_code overload) returns *this == code.category() &&
// code.value() == condition.
// [syserr.errcat.nonvirtuals]/1-2: operator== returns this == &rhs; operator<=> returns
// compare_three_way()(this, &rhs).
// [syserr.errcat.overview] Note 1: two objects of the same category type are distinct
// categories.
#include <system_error>
#include <compare>
#include <functional>
#include <string>
#include <type_traits>
#include "check.hpp"

// A program-defined category relying on every non-pure virtual's default behaviour.
struct PlainCat : std::error_category {
  constexpr PlainCat() noexcept = default;
  const char* name() const noexcept override { return "plain"; }
  std::string message(int ev) const override { return ev == 0 ? "zero" : "nonzero"; }
};

// Overrides only default_error_condition: the inherited equivalent(int, const error_condition&)
// is "default_error_condition(code) == condition" ([syserr.errcat.virtuals]/3), so it must call
// the override (a virtual call), as must error_code::default_error_condition()
// ([syserr.errcode.observers]/3) and error_code == error_condition ([syserr.compare]/2).
struct MapCat : std::error_category {
  const char* name() const noexcept override { return "map"; }
  std::string message(int) const override { return "map"; }
  std::error_condition default_error_condition(int ev) const noexcept override {
    return std::error_condition(ev + 1, std::generic_category());
  }
};
const MapCat map_cat;

// The constexpr constructor allows constant initialisation of a category object.
constinit PlainCat cat_a;
constinit PlainCat cat_b;

static_assert(std::is_nothrow_default_constructible_v<PlainCat>);
static_assert(!std::is_copy_constructible_v<std::error_category>);
static_assert(!std::is_copy_assignable_v<std::error_category>);
static_assert(!std::is_move_constructible_v<std::error_category>);
static_assert(!std::is_move_assignable_v<std::error_category>);
static_assert(std::is_abstract_v<std::error_category>);
static_assert(std::has_virtual_destructor_v<std::error_category>);
static_assert(std::is_polymorphic_v<std::error_category>);
static_assert(noexcept(cat_a.name()));
static_assert(noexcept(cat_a.default_error_condition(1)));
static_assert(noexcept(cat_a.equivalent(1, std::error_condition())));
static_assert(noexcept(cat_a.equivalent(std::error_code(), 1)));
static_assert(noexcept(cat_a == cat_b));
static_assert(noexcept(cat_a <=> cat_b));
static_assert(std::is_same_v<decltype(cat_a <=> cat_b), std::strong_ordering>);
static_assert(std::is_same_v<decltype(cat_a == cat_b), bool>);
static_assert(std::is_same_v<decltype(cat_a.message(0)), std::string>);
static_assert(std::is_same_v<decltype(cat_a.name()), const char*>);
static_assert(std::is_same_v<decltype(cat_a.default_error_condition(0)), std::error_condition>);

int main() {
  const std::error_category& a = cat_a;
  const std::error_category& b = cat_b;

  // identity comparisons ([syserr.errcat.nonvirtuals]/1, Note 1 of the overview)
  CHECK(a == a);
  CHECK(!(a == b));
  CHECK(a != b);
  CHECK(!(a != a));
  CHECK(a != std::generic_category());
  CHECK(a != std::system_category());

  // <=> is the pointer order of the addresses ([syserr.errcat.nonvirtuals]/2)
  CHECK((a <=> a) == std::strong_ordering::equal);
  std::strong_ordering expected = std::compare_three_way()(&a, &b);
  CHECK((a <=> b) == expected);
  CHECK((b <=> a) == std::compare_three_way()(&b, &a));
  CHECK((a <=> b) != std::strong_ordering::equal);
  CHECK((a < b) == std::less<const std::error_category*>()(&a, &b));
  CHECK((a > b) == std::less<const std::error_category*>()(&b, &a));
  CHECK((a <= a) && (a >= a));

  // default_error_condition ([syserr.errcat.virtuals]/2)
  std::error_condition c = a.default_error_condition(42);
  CHECK(c.value() == 42);
  CHECK(c.category() == a);
  CHECK(&c.category() == &cat_a);
  CHECK(a.default_error_condition(0).value() == 0);
  CHECK(a.default_error_condition(0).category() == a);
  CHECK(a.default_error_condition(-7).value() == -7);

  // equivalent(int, const error_condition&) ([syserr.errcat.virtuals]/3)
  CHECK(a.equivalent(5, std::error_condition(5, a)));
  CHECK(!a.equivalent(5, std::error_condition(6, a)));
  CHECK(!a.equivalent(5, std::error_condition(5, b)));
  CHECK(!a.equivalent(5, std::error_condition(5, std::generic_category())));

  // equivalent(const error_code&, int) ([syserr.errcat.virtuals]/4)
  CHECK(a.equivalent(std::error_code(3, a), 3));
  CHECK(!a.equivalent(std::error_code(3, a), 4));
  CHECK(!a.equivalent(std::error_code(3, b), 3));
  CHECK(!a.equivalent(std::error_code(3, std::system_category()), 3));

  // the virtual default_error_condition is used by the inherited equivalent
  {
    const std::error_category& m = map_cat;
    CHECK(m.equivalent(4, std::error_condition(5, std::generic_category())));
    CHECK(!m.equivalent(4, std::error_condition(4, m)));
    CHECK(!m.equivalent(4, std::error_condition(4, std::generic_category())));
    CHECK(std::error_code(4, m).default_error_condition() == std::error_condition(5, std::generic_category()));
    CHECK(std::error_code(4, m) == std::error_condition(5, std::generic_category()));
    CHECK(std::error_condition(5, std::generic_category()) == std::error_code(4, m));
    // the second disjunct: m.equivalent(code, 4) is "same category and value"
    CHECK(std::error_code(4, m) == std::error_condition(4, m));
    CHECK(std::error_code(4, m) != std::error_condition(4, std::generic_category()));
  }

  // the pure virtuals are dispatched
  CHECK(std::string(a.name()) == "plain");
  CHECK(a.message(0) == "zero");
  CHECK(a.message(9) == "nonzero");

  // a dynamically created category is distinct from the static ones
  PlainCat* p = new PlainCat;
  CHECK(*p != a && *p != b && *p == *p);
  CHECK(((*p <=> a) == std::strong_ordering::less) == std::less<const void*>()(p, &cat_a));
  delete static_cast<std::error_category*>(p);  // virtual destructor
  return 0;
}
