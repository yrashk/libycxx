// [optional.monadic]: and_then/transform exist for &, const&, && and const&& *this:
//   /1 and_then &/const&: "Let U be invoke_result_t<F, decltype((val))>" and returns
//      invoke(std::forward<F>(f), val); /4 the && overloads use std::move(val);
//   /7, /10 transform likewise with val / std::move(val);
//   /13-18 or_else: std::forward<F>(f)() when empty.
// So every combination of the optional's category (val is T&, const T&, T&&, const T&&) and of
// the callable's category (std::forward<F>(f)) reaches the matching overload. Also checked for
// optional<T&> ([optional.ref.monadic]): the argument is always T& (*val), whatever the
// category of the optional. A transform whose result is not movable works by guaranteed
// copy elision ([optional.monadic]/8, Note 1: "There is no requirement that U is movable").
#include <optional>
#include <utility>
#include "category_probe.hpp"
#include "check.hpp"

using catprobe::AsInt;
struct AsOpt {
  constexpr std::optional<int> operator()(int c) const { return c; }
};
struct NonMovable {
  int v;
  constexpr NonMovable(int x) : v(x) {}
  NonMovable(NonMovable&&) = delete;
};
struct MakeNM {
  constexpr NonMovable operator()(int c) const { return NonMovable(c); }
};

template <class Opt, class F>
constexpr int tr(Opt&& o, F&& f) { return *std::forward<Opt>(o).transform(std::forward<F>(f)); }
template <class Opt, class F>
constexpr int at(Opt&& o, F&& f) { return *std::forward<Opt>(o).and_then(std::forward<F>(f)); }

constexpr bool test() {
  catprobe::Probe<AsInt> p;
  catprobe::Probe<AsOpt> q;
  std::optional<int> o = 1;
  const std::optional<int>& co = o;
  // optional category -> units digit; callable category -> tens digit.
  if (tr(o, p) != 0 || tr(co, p) != 1 || tr(std::move(o), p) != 2 || tr(std::move(co), p) != 3) return false;
  if (tr(o, std::as_const(p)) != 10 || tr(o, std::move(p)) != 20 || tr(o, std::move(std::as_const(p))) != 30) return false;
  if (tr(std::move(co), std::move(std::as_const(p))) != 33 || tr(std::move(o), std::as_const(p)) != 12) return false;
  if (at(o, q) != 0 || at(co, q) != 1 || at(std::move(o), q) != 2 || at(std::move(co), q) != 3) return false;
  if (at(co, std::move(q)) != 21 || at(std::move(o), std::move(std::as_const(q))) != 32) return false;

  catprobe::Probe0<AsOpt> z;
  std::optional<int> empty;
  if (*empty.or_else(z) != 0 || *empty.or_else(std::as_const(z)) != 10 || *empty.or_else(std::move(z)) != 20 ||
      *empty.or_else(std::move(std::as_const(z))) != 30)
    return false;
  if (*std::move(empty).or_else(std::move(z)) != 20) return false;

  // Non-movable results.
  if (o.transform(MakeNM{})->v != 1 || std::move(co).transform(MakeNM{})->v != 1) return false;
  if (empty.transform(MakeNM{}).has_value()) return false;

  // optional<T&>: the argument is always T& (code 0 for a non-const referent).
  int x = 5;
  std::optional<int&> r(x);
  const std::optional<int&>& cr = r;
  if (tr(r, p) != 0 || tr(cr, p) != 0 || tr(std::move(r), p) != 0 || tr(std::move(cr), std::move(p)) != 20) return false;
  if (at(std::move(cr), q) != 0) return false;
  std::optional<const int&> rc(x);
  if (tr(std::move(rc), p) != 1) return false;
  if (r.transform(MakeNM{})->v != 5) return false;
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
