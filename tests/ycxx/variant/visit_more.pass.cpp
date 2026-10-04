// [variant.visit]/4-6: e(m) is INVOKE(std::forward<Visitor>(vis), GET<m>(std::forward<V>(vars))...)
// (resp. INVOKE<R>); "Returns: e(m), where m is the pack for which m_i is
// as-variant(vars_i).index()". So the visitor's value category is forwarded, the selected
// alternative is chosen by index (also with duplicate alternative types), three or more
// variants of different sizes combine, and visit<R> converts through INVOKE<R>. /8: for n == 1
// the invocation must not depend on the number of alternatives -- exercised here with a
// variant of 40 alternatives.
#include <variant>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct ByCategory {
  int operator()(int) & { return 1; }
  int operator()(int) const& { return 2; }
  int operator()(int) && { return 3; }
};
struct Base {
  int id;
};
struct Derived : Base {};

template <std::size_t I>
struct Alt {
  static constexpr std::size_t value = I;
};
template <std::size_t... I>
auto make_big(std::index_sequence<I...>) -> std::variant<Alt<I>...>;
using Big = decltype(make_big(std::make_index_sequence<40>{}));

template <std::size_t... I>
bool check_big(std::index_sequence<I...>) {
  bool ok = true;
  ((ok = ok && std::visit([](auto a) { return decltype(a)::value; }, Big(std::in_place_index<I>)) == I), ...);
  return ok;
}

int main() {
  std::variant<int> v(0);
  ByCategory vis;
  const ByCategory cvis;
  CHECK(std::visit(vis, v) == 1);
  CHECK(std::visit(cvis, v) == 2);
  CHECK(std::visit(ByCategory{}, v) == 3);
  CHECK(std::visit(std::move(vis), v) == 3);
  CHECK(v.visit(ByCategory{}) == 3);

  // duplicate alternative types: the active index decides
  std::variant<int, int> dup(std::in_place_index<1>, 42);
  CHECK(std::visit([](int x) { return x; }, dup) == 42);
  dup.emplace<0>(7);
  CHECK(std::visit([](int x) { return x; }, dup) == 7);

  // three variants of different sizes
  std::variant<int, char> a('x');
  std::variant<long> b(2L);
  std::variant<double, int, short> c(short(3));
  auto code = [](auto x, auto y, auto z) {
    return (std::is_same_v<decltype(x), char> ? 100 : 0) + (std::is_same_v<decltype(y), long> ? 10 : 0) +
           (std::is_same_v<decltype(z), short> ? 1 : 0);
  };
  CHECK(std::visit(code, a, b, c) == 111);
  a = 1;
  c = 2.0;
  CHECK(std::visit(code, a, b, c) == 10);

  // visit<R>: implicit conversions of the visitor's results
  std::variant<Derived, Base> db(Derived{{5}});
  const Base& br = std::visit<const Base&>([](const auto& x) -> const auto& { return x; }, db);
  CHECK(br.id == 5 && &br == &std::get<0>(db));
  Derived d{{1}};
  Base bb{2};
  std::variant<Derived*, Base*> ptrs(&d);
  Base* bp = std::visit<Base*>([](auto* p) { return p; }, ptrs);  // the lambda alone would not unify
  CHECK(bp == &d);
  ptrs = &bb;
  CHECK(std::visit<Base*>([](auto* p) { return p; }, ptrs)->id == 2);

  CHECK(check_big(std::make_index_sequence<40>{}));
  return 0;
}
