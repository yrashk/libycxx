// [variant.status]: valueless_by_exception() and index() (variant_npos when valueless),
// both noexcept. A variant becomes valueless when initialization of the new alternative
// throws during emplace ([variant.mod]/11, cf. the note in [variant.status]/2).
// Then: [variant.get]/7 get throws bad_variant_access; get_if returns nullptr;
// [variant.visit]/7 visit throws bad_variant_access; [variant.relops] valueless ordering;
// [variant.assign]/2.2 assigning a valueless variant makes *this valueless;
// [variant.swap]/3.1, [variant.ctor]/7,11 copying/moving a valueless variant.
// REQUIRES: exceptions
#include <variant>
#include <type_traits>
#include "check.hpp"

// The emplaced alternative's construction throws and its copy and move constructors throw
// too, so an implementation cannot build a temporary and move it in: the variant ends up
// valueless (the "permitted" outcome of [variant.mod]/11; no realistic alternative exists).
struct MakeEmpty {
  MakeEmpty() = default;
  MakeEmpty(int) { throw 42; }
  MakeEmpty(const MakeEmpty&) { throw 42; }
  MakeEmpty(MakeEmpty&&) { throw 42; }
  MakeEmpty& operator=(const MakeEmpty&) = default;
  MakeEmpty& operator=(MakeEmpty&&) = default;
  bool operator==(const MakeEmpty&) const = default;
  auto operator<=>(const MakeEmpty&) const = default;
};

using V = std::variant<float, int, MakeEmpty>;

static V make_valueless() {
  V v{12.f};
  try {
    v.emplace<2>(1);
  } catch (int) {
  }
  return v;
}

static_assert(noexcept(std::declval<V&>().index()));
static_assert(noexcept(std::declval<V&>().valueless_by_exception()));
static_assert(std::is_same_v<decltype(std::variant_npos), const std::size_t>);
static_assert(std::variant_npos == static_cast<std::size_t>(-1));

int main() {
  V v = make_valueless();
  CHECK(v.valueless_by_exception());
  CHECK(v.index() == std::variant_npos);
  CHECK(std::get_if<0>(&v) == nullptr);
  CHECK(std::get_if<int>(&v) == nullptr);
  CHECK(!std::holds_alternative<float>(v) && !std::holds_alternative<int>(v));

  bool threw = false;
  try { (void)std::get<0>(v); } catch (const std::bad_variant_access&) { threw = true; }
  CHECK(threw);
  threw = false;
  try { (void)std::get<int>(v); } catch (const std::bad_variant_access&) { threw = true; }
  CHECK(threw);
  threw = false;
  try { std::visit([](auto) {}, v); } catch (const std::bad_variant_access&) { threw = true; }
  CHECK(threw);
  threw = false;
  try { std::visit<void>([](auto) {}, v); } catch (const std::bad_variant_access&) { threw = true; }
  CHECK(threw);
  threw = false;
  try { v.visit([](auto) {}); } catch (const std::bad_variant_access&) { threw = true; }
  CHECK(threw);

  // copy and move of a valueless variant are valueless
  V c(v);
  CHECK(c.valueless_by_exception());
  V m(std::move(c));
  CHECK(m.valueless_by_exception());

  // relational operators
  V good(1);
  CHECK(v == m && !(v != m));
  CHECK(!(v == good) && v != good);
  CHECK(v < good && !(good < v));
  CHECK(good > v && !(v > good));
  CHECK(v <= good && v <= m && !(good <= v));
  CHECK(good >= v && v >= m && !(v >= good));
  CHECK(!(v < m) && !(v > m));
  CHECK((v <=> m) == 0);
  CHECK((v <=> good) < 0);
  CHECK((good <=> v) > 0);

  // assignment from a valueless variant makes *this valueless
  V a(2);
  a = v;
  CHECK(a.valueless_by_exception());
  V b(3.f);
  b = std::move(m);
  CHECK(b.valueless_by_exception());
  // assignment into a valueless variant restores a value
  a = good;
  CHECK(a.index() == 1 && std::get<1>(a) == 1);
  b = 2.5f;
  CHECK(b.index() == 0);

  // swap: both valueless -> no effect; one valueless -> exchanged
  V x = make_valueless(), y = make_valueless();
  x.swap(y);
  CHECK(x.valueless_by_exception() && y.valueless_by_exception());
  V z(5);
  x.swap(z);
  CHECK(!x.valueless_by_exception() && std::get<1>(x) == 5 && z.valueless_by_exception());

  // emplace into a valueless variant
  z.emplace<0>(1.f);
  CHECK(z.index() == 0);
  return 0;
}
