// [variant.visit]/3-6: visit(vis, vars...) returns e(m) "where m is the pack for which m_i is
// as-variant(vars_i).index() for all 0 <= i < n", e(m) being INVOKE(std::forward<Visitor>(vis),
// GET<m>(std::forward<V>(vars))...) (INVOKE<R> for visit<R>). Checked exhaustively for every
// index combination of five variants (3 x 4 x 3 x 4 x 3 = 432 combinations) plus a
// single-alternative one, mixing const, rvalue and a class derived from variant
// (as-variant), with visit<long> converting a short result. Duplicate alternative types
// are selected by index, also through the member visit<R> ([variant.visit]).
#include <utility>
#include <variant>
#include "check.hpp"

template <int N>
struct I {
  static constexpr int v = N;
};
using V3 = std::variant<I<0>, I<1>, I<2>>;
using V4 = std::variant<I<0>, I<1>, I<2>, I<3>>;
using V1 = std::variant<I<0>>;
struct D : V4 {
  using V4::V4;
};

template <class V, std::size_t... Is>
V make(std::size_t i, std::index_sequence<Is...>) {
  V v;
  ((i == Is ? (void)v.template emplace<Is>() : void()), ...);
  return v;
}
template <class V>
V mk(std::size_t i) {
  return make<V>(i, std::make_index_sequence<std::variant_size_v<V>>{});
}

int main() {
  int total = 0;
  for (int a = 0; a < 3; ++a)
    for (int b = 0; b < 4; ++b)
      for (int c = 0; c < 3; ++c)
        for (int d = 0; d < 4; ++d)
          for (int e = 0; e < 3; ++e) {
            V3 va = mk<V3>(a);
            V4 vb = mk<V4>(b);
            V3 vc = mk<V3>(c);
            D vd;
            static_cast<V4&>(vd) = mk<V4>(d);
            V3 ve = mk<V3>(e);
            V1 vf;
            const int want = a * 10000 + b * 1000 + c * 100 + d * 10 + e;
            int r = std::visit(
                [](auto x, auto y, const auto& z, auto&& w, auto u, auto f) {
                  return x.v * 10000 + y.v * 1000 + z.v * 100 + w.v * 10 + u.v + f.v;
                },
                va, vb, std::as_const(vc), std::move(vd), ve, vf);
            long rl = std::visit<long>(
                [](auto x, auto y, auto z, auto w, auto u) {
                  return static_cast<short>(x.v * 10000 + y.v * 1000 + z.v * 100 + w.v * 10 + u.v);
                },
                va, vb, vc, vd, ve);
            CHECK(r == want && rl == want);
            ++total;
          }
  CHECK(total == 432);

  std::variant<int, int, long, int> dv(std::in_place_index<3>, 7);
  long got = dv.visit<long>([](auto x) { return static_cast<int>(x) * 2; });
  CHECK(got == 14);
  dv.emplace<1>(5);
  CHECK(std::visit([](auto x) { return static_cast<long>(x); }, dv) == 5);
  return 0;
}
