// [optional.monadic]/1-3: and_then: U = invoke_result_t<F, decltype((val))>; returns
// invoke(std::forward<F>(f), val) or remove_cvref_t<U>() -- so a function returning a
// (const) reference to an optional yields a copy of that optional's type. /7-9: transform:
// U = remove_cv_t<invoke_result_t<F, decltype((val))>>, so a function returning a const
// class prvalue yields optional<non-const>. /4-6, /10-12: the && overloads pass std::move(val).
// /15, /18: or_else returns *this (copy) or std::move(*this) when engaged, else
// std::forward<F>(f)(). The callable is std::forward<F>(f)'d: an rvalue callable is invoked as
// an rvalue. f is not invoked on an empty optional (and_then / transform) or on an engaged one
// (or_else).
#include <optional>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct MoveOnly {
  int v;
  constexpr explicit MoveOnly(int x) : v(x) {}
  constexpr MoveOnly(MoveOnly&& o) noexcept : v(o.v) { o.v = -1; }
  MoveOnly(const MoveOnly&) = delete;
};
struct RefQual {
  constexpr int operator()(int) & { return 1; }
  constexpr int operator()(int) && { return 2; }
  constexpr std::optional<int> operator()() & { return 1; }
  constexpr std::optional<int> operator()() && { return 2; }
};
struct RefQualOpt {
  constexpr std::optional<int> operator()(int) & { return 1; }
  constexpr std::optional<int> operator()(int) && { return 2; }
};

const std::optional<long> global_opt(99);
const std::optional<long>& ref_to_global(int) { return global_opt; }
struct Small {
  int v;
};
const Small make_const(int x) { return Small{x + 1}; }

int main() {
  std::optional<int> o(3), e;
  // and_then returning a reference to an optional: result is remove_cvref
  auto r = o.and_then(ref_to_global);
  static_assert(std::is_same_v<decltype(r), std::optional<long>>);
  CHECK(r == 99L);
  CHECK(!e.and_then(ref_to_global).has_value());
  // transform strips cv from the result type
  auto t = o.transform(make_const);
  static_assert(std::is_same_v<decltype(t), std::optional<Small>>);
  CHECK(t->v == 4);
  // value category of the callable is forwarded
  RefQual rq;
  CHECK(o.transform(rq) == 1);
  CHECK(o.transform(RefQual{}) == 2);
  RefQualOpt rqo;
  CHECK(o.and_then(rqo) == 1);
  CHECK(o.and_then(RefQualOpt{}) == 2);
  CHECK(e.or_else(rq) == 1);
  CHECK(e.or_else(RefQual{}) == 2);
  // f is not invoked when it should not be
  int calls = 0;
  auto count = [&](int x) { ++calls; return x; };
  auto count_opt = [&](int x) { ++calls; return std::optional<int>(x); };
  auto fallback = [&] { ++calls; return std::optional<int>(0); };
  CHECK(!e.transform(count).has_value() && !e.and_then(count_opt).has_value());
  CHECK(o.or_else(fallback) == 3);
  CHECK(calls == 0);
  // rvalue overloads move the contained value
  std::optional<MoveOnly> m(std::in_place, 5);
  auto moved = std::move(m).transform([](MoveOnly&& x) { return MoveOnly(std::move(x)).v; });
  CHECK(moved == 5 && m->v == -1);
  std::optional<MoveOnly> m2(std::in_place, 6);
  auto kept = std::move(m2).or_else([] { return std::optional<MoveOnly>(); });
  CHECK(kept.has_value() && kept->v == 6 && m2.has_value() && m2->v == -1);
  std::optional<MoveOnly> m3(std::in_place, 7);
  auto chained = std::move(m3).and_then([](MoveOnly&& x) { return std::optional<MoveOnly>(std::move(x)); });
  CHECK(chained->v == 7 && m3->v == -1);
  // const rvalue overload sees const T&&
  const std::optional<int> co(8);
  auto cat = std::move(co).transform([](auto&& x) {
    return std::is_same_v<decltype(x), const int&&>;
  });
  CHECK(cat == true);
  return 0;
}
