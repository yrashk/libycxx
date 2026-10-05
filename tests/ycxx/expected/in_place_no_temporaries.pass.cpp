// expected's in_place and unexpect constructors and emplace construct the value (or the error)
// in place from the forwarded arguments: no temporary, no copy or move, categories kept.
//   [expected.object.cons]: expected(in_place_t, Args&&... args): "Direct-non-list-
//     initializes val with std::forward<Args>(args)..."; the initializer_list form "with il,
//     std::forward<Args>(args)..."; expected(unexpect_t, Args&&...): "Direct-non-list-
//     initializes unex with std::forward<Args>(args)..."; and its initializer_list form.
//   [expected.object.assign]: emplace(Args&&... args) noexcept, Constraints:
//     is_nothrow_constructible_v<T, Args...>; Effects: destroys the current value or error,
//     then construct_at(addressof(val), std::forward<Args>(args)...); Returns: val.
//   [expected.void.cons]: expected<void, E>(unexpect_t, args...) direct-non-list-initializes
//     unex with std::forward<Args>(args)....
//   [expected.un.cons]: unexpected(in_place_t, Args&&... args): "Direct-non-list-initializes
//     unex with std::forward<Args>(args)...".
#include <expected>
#include <initializer_list>
#include <utility>
#include "check.hpp"
#include "inplace_probe.hpp"

using probe::Arg;
using probe::counts;
using probe::Pinned;

// Pinned with noexcept constructors (expected::emplace requires nothrow constructibility).
struct NPinned {
  int key = 0, cat = 0;
  NPinned(int k, Arg&) noexcept : key(k), cat(probe::lref) { ++counts.made; }
  NPinned(int k, const Arg&) noexcept : key(k), cat(probe::clref) { ++counts.made; }
  NPinned(int k, Arg&& a) noexcept : key(k), cat(probe::rref) {
    a.moved_from = true;
    ++counts.made;
  }
  NPinned(std::initializer_list<int> il, const Arg&&) noexcept
      : key(static_cast<int>(il.size())), cat(probe::crref) {
    ++counts.made;
  }
  NPinned(const NPinned&) = delete;
  ~NPinned() { ++counts.destroyed; }
};

struct Lit {
  int v, cat;
  constexpr Lit(int& x) noexcept : v(x), cat(probe::lref) {}
  constexpr Lit(int&& x) noexcept : v(x), cat(probe::rref) {}
  constexpr Lit(std::initializer_list<int> il, int& x) noexcept
      : v(static_cast<int>(il.size()) * x), cat(probe::lref) {}
  Lit(const Lit&) = delete;
};

constexpr bool constant() {
  int x = 3;
  std::expected<Lit, Lit> a(std::in_place, x);
  std::expected<Lit, Lit> b(std::unexpect, 4);
  std::expected<Lit, Lit> c(std::in_place, {1, 2}, x);
  std::expected<Lit, Lit> d(std::unexpect, {1, 2, 3}, x);
  std::expected<void, Lit> v(std::unexpect, x);
  bool ok = a->cat == probe::lref && b.error().cat == probe::rref && c->v == 6 && d.error().v == 9 &&
            v.error().cat == probe::lref;
  return ok;
}
static_assert(constant());

int main() {
  Arg a{7};
  const Arg ca{8};
  probe::reset();
  {
    std::expected<Pinned, int> e1(std::in_place, 1, a);
    std::expected<Pinned, int> e2(std::in_place, 2, std::move(ca));
    std::expected<int, Pinned> e3(std::unexpect, 3, ca);
    std::expected<int, Pinned> e4(std::unexpect, 4, Arg{});
    std::expected<void, Pinned> e5(std::unexpect, 5, a);
    CHECK(e1->cat == probe::lref && e2->cat == probe::crref);
    CHECK(e3.error().cat == probe::clref && e4.error().cat == probe::rref);
    CHECK(e5.error().cat == probe::lref);
    CHECK(counts.made == 5 && counts.extra() == 0 && counts.destroyed == 0);

    std::unexpected<Pinned> u(std::in_place, 6, std::move(a));
    CHECK(u.error().cat == probe::rref && a.moved_from && counts.made == 6 && counts.extra() == 0);
  }
  CHECK(counts.destroyed == 6);

  // emplace: over a value, and over an error.
  probe::reset();
  {
    Arg b{1};
    std::expected<NPinned, int> x(std::in_place, 1, b);
    NPinned* where = &*x;
    NPinned& r = x.emplace(2, std::move(b));
    CHECK(&r == where && r.key == 2 && r.cat == probe::rref && b.moved_from);
    CHECK(counts.made == 2 && counts.destroyed == 1);
    std::expected<NPinned, NPinned> y(std::unexpect, 3, std::as_const(b));
    CHECK(y.error().cat == probe::clref);
    NPinned& s = y.emplace({1, 2, 3}, std::move(std::as_const(b)));
    CHECK(y.has_value() && &s == &*y && s.key == 3 && s.cat == probe::crref);
    CHECK(counts.made == 4 && counts.destroyed == 2);
  }
  CHECK(counts.destroyed == 4);
  return 0;
}
