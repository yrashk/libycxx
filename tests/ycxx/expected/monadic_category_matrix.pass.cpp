// [expected.object.monadic]: and_then/transform pass val as decltype((val)) for & and const&
// *this and std::move(val) for && and const&& (/1-/8, /17-/24); or_else/transform_error do the
// same with error() / std::move(error()) (/9-/16, /25-/32); the callable is std::forward<F>(f).
// [expected.void.monadic]: and_then/transform invoke f with no argument; or_else and
// transform_error pass error() / std::move(error()). transform and transform_error of a
// non-movable result type work by guaranteed copy elision: /19 "If is_void_v<U> is false, the
// declaration U u(invoke(...)); is well-formed" and /27 "The declaration G g(invoke(...)); is
// well-formed" -- no move is required ([expected.object.general]/2's valid value types include
// non-movable ones; the result is direct-non-list-initialized). transform with a void result
// yields expected<void, E>.
#include <expected>
#include <type_traits>
#include <utility>
#include "category_probe.hpp"
#include "check.hpp"

using catprobe::AsInt;
struct AsExp {
  constexpr std::expected<int, int> operator()(int c) const { return c; }
};
struct AsUnexp {
  constexpr std::expected<int, int> operator()(int c) const { return std::unexpected(c); }
};
struct AsVoidUnexp {
  constexpr std::expected<void, int> operator()(int c) const { return std::unexpected(c); }
};
struct NonMovable {
  int v;
  constexpr NonMovable(int x) : v(x) {}
  NonMovable(NonMovable&&) = delete;
};
struct MakeNM {
  constexpr NonMovable operator()(int c) const { return NonMovable(c); }
};

template <class E, class F> constexpr int tr(E&& e, F&& f) { return *std::forward<E>(e).transform(std::forward<F>(f)); }
template <class E, class F> constexpr int at(E&& e, F&& f) { return *std::forward<E>(e).and_then(std::forward<F>(f)); }
template <class E, class F> constexpr int te(E&& e, F&& f) { return std::forward<E>(e).transform_error(std::forward<F>(f)).error(); }
template <class E, class F> constexpr int oe(E&& e, F&& f) { return std::forward<E>(e).or_else(std::forward<F>(f)).error(); }

constexpr bool test() {
  catprobe::Probe<AsInt> p;
  catprobe::Probe<AsExp> q;
  catprobe::Probe<AsUnexp> u;
  std::expected<int, int> v = 1;
  const auto& cv = v;
  std::expected<int, int> e = std::unexpected(2);
  const auto& ce = e;
  if (tr(v, p) != 0 || tr(cv, p) != 1 || tr(std::move(v), p) != 2 || tr(std::move(cv), p) != 3) return false;
  if (tr(v, std::as_const(p)) != 10 || tr(cv, std::move(p)) != 21 || tr(std::move(cv), std::move(std::as_const(p))) != 33) return false;
  if (at(v, q) != 0 || at(cv, std::move(q)) != 21 || at(std::move(v), std::as_const(q)) != 12 || at(std::move(cv), q) != 3) return false;
  if (te(e, p) != 0 || te(ce, p) != 1 || te(std::move(e), p) != 2 || te(std::move(ce), p) != 3) return false;
  if (te(e, std::move(std::as_const(p))) != 30 || te(std::move(ce), std::as_const(p)) != 13) return false;
  if (oe(e, u) != 0 || oe(ce, std::move(u)) != 21 || oe(std::move(e), u) != 2 || oe(std::move(ce), std::as_const(u)) != 13) return false;
  // Values pass through or_else / transform_error; errors through and_then / transform.
  if (*v.or_else(u) != 1 || *v.transform_error(p) != 1) return false;
  if (e.transform(p).error() != 2 || e.and_then(q).error() != 2) return false;

  // Non-movable results.
  auto nm = std::move(cv).transform(MakeNM{});
  if (nm->v != 1) return false;
  auto nme = std::move(e).transform_error(MakeNM{});
  if (nme.error().v != 2) return false;
  auto nm_err = e.transform(MakeNM{});
  if (nm_err.has_value() || nm_err.error() != 2) return false;
  auto nme_val = v.transform_error(MakeNM{});
  if (!nme_val.has_value() || *nme_val != 1) return false;

  // transform to void.
  int side = 0;
  auto tv = v.transform([&side](int) { side = 1; });
  static_assert(std::is_same_v<decltype(tv), std::expected<void, int>>);
  if (!tv.has_value() || side != 1) return false;

  // expected<void, E>.
  catprobe::Probe0<AsInt> z;
  catprobe::Probe0<AsExp> zq;
  catprobe::Probe<AsVoidUnexp> vu;
  std::expected<void, int> ok;
  std::expected<void, int> bad = std::unexpected(4);
  const auto& cbad = bad;
  if (*ok.transform(z) != 0 || *ok.transform(std::move(z)) != 20 || *std::as_const(ok).and_then(std::as_const(zq)) != 10) return false;
  if (bad.transform(z).error() != 4 || bad.and_then(zq).error() != 4) return false;
  if (te(bad, p) != 0 || te(cbad, p) != 1 || te(std::move(bad), p) != 2 || te(std::move(cbad), std::move(p)) != 23) return false;
  if (oe(bad, vu) != 0 || oe(std::move(cbad), std::as_const(vu)) != 13) return false;
  if (!ok.or_else(vu).has_value() || !ok.transform_error(p).has_value()) return false;
  auto vnm = std::move(bad).transform_error(MakeNM{});
  if (vnm.error().v != 4) return false;
  if (!ok.transform_error(MakeNM{}).has_value()) return false;
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
