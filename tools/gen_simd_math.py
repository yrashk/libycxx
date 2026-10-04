#!/usr/bin/env python3
"""Generates include/ycxx/core/simd_math.hpp: the <cmath> and <complex> overloads of <simd>
([simd.math], [simd.complex.math]) and the using-declarations of [simd.syn] in namespace std.

Every function applies the scalar std:: function element by element (math-func-vec), after
converting each argument to deduced-vec-t<V>. The overload sets are transcribed from the draft:
binary functions get (V, V), (D, V), (V, D); ternary ones every mix of V and D but (D, D, D)."""

import itertools
import os

LIMIT = 120
OUT = os.path.join(os.path.dirname(__file__), "..", "include", "ycxx", "core", "simd_math.hpp")

D = "ycxx::detail::deduced_vec_t<V>"
MASK = f"typename {D}::mask_type"


def rebind(t):
    return f"std::simd::rebind_t<{t}, {D}>"


# name, arity, return type, constexpr, noexcept-on-the-all-V-overload
UNARY = [
    *[(n, D, True) for n in "acos asin atan cos sin tan acosh asinh atanh cosh sinh tanh exp exp2 expm1 log log10 "
      "log1p log2 logb cbrt sqrt erf erfc lgamma tgamma ceil floor round trunc".split()],
    ("nearbyint", D, False), ("rint", D, False),
    ("ilogb", rebind("int"), True), ("lrint", rebind("long int"), False), ("llrint", rebind("long long int"), False),
    ("lround", rebind("long int"), True), ("llround", rebind("long long int"), True),
    ("fpclassify", rebind("int"), True),
    *[(n, MASK, True) for n in "isfinite isinf isnan isnormal signbit".split()],
    *[(n, D, False) for n in "comp_ellint_1 comp_ellint_2 expint riemann_zeta".split()],
]
BINARY = [
    *[(n, D, True) for n in "atan2 hypot pow fmod remainder copysign nextafter fdim fmax fmin".split()],
    *[(n, MASK, True) for n in "isgreater isgreaterequal isless islessequal islessgreater isunordered".split()],
    *[(n, D, False) for n in "beta comp_ellint_3 cyl_bessel_i cyl_bessel_j cyl_bessel_k cyl_neumann ellint_1 ellint_2".split()],
]
TERNARY = [("hypot", D, True), ("fma", D, True), ("lerp", D, True), ("ellint_3", D, False)]
# (name, unsigned parameters before x, constexpr)
UNSIGNED = [("assoc_laguerre", 2), ("assoc_legendre", 2), ("sph_legendre", 2), ("hermite", 1), ("laguerre", 1),
            ("legendre", 1), ("sph_bessel", 1), ("sph_neumann", 1)]
# (name, type of the second parameter)
LDEXP = [("ldexp", "int"), ("scalbn", "int"), ("scalbln", "long int")]

COMPLEX_REAL = ["real", "imag", "abs", "arg", "norm"]
COMPLEX_SAME = ["conj", "proj", "exp", "log", "log10", "sqrt", "sin", "asin", "cos", "acos", "tan", "atan", "sinh",
                "asinh", "cosh", "acosh", "tanh", "atanh"]

USING_ALG = ["min", "max", "minmax", "clamp"]
USING_MATH = ("acos asin atan atan2 cos sin tan acosh asinh atanh cosh sinh tanh exp exp2 expm1 frexp ilogb ldexp log "
              "log10 log1p log2 logb modf scalbn scalbln cbrt abs fabs hypot pow sqrt erf erfc lgamma tgamma ceil "
              "floor nearbyint rint lrint llrint round lround llround trunc fmod remainder remquo copysign nextafter "
              "fdim fmax fmin fma lerp fpclassify isfinite isinf isnan isnormal signbit isgreater isgreaterequal "
              "isless islessequal islessgreater isunordered assoc_laguerre assoc_legendre beta comp_ellint_1 "
              "comp_ellint_2 comp_ellint_3 cyl_bessel_i cyl_bessel_j cyl_bessel_k cyl_neumann ellint_1 ellint_2 "
              "ellint_3 expint hermite laguerre legendre riemann_zeta sph_bessel sph_legendre sph_neumann").split()
USING_BIT = ("byteswap bit_ceil bit_floor bit_reverse has_single_bit shl shr rotl rotr bit_repeat bit_width "
             "countl_zero countl_one countr_zero countr_one popcount bit_compress bit_expand").split()
USING_COMPLEX = "real imag arg norm conj proj polar".split()

out = []
w = out.append


def fn(name, ret, params, args, constexpr, noexcept=False, lam=None):
    """One function template over V: params are (type, name) pairs, args the D-converted arguments."""
    w("template <ycxx::detail::math_floating_point V>")
    head = f"{'constexpr ' if constexpr else ''}{ret}"
    ps = [f"const {t}& {n}" for t, n in params]
    tail = f"){' noexcept' if noexcept else ''} {{"
    line = f"{head} {name}({', '.join(ps)}{tail}"
    if len(line) <= LIMIT:
        w(line)
    else:  # one parameter per line
        w(f"{head}")
        w(f"{name}({ps[0]},")
        for p in ps[1:-1]:
            w(f"    {p},")
        w(f"    {ps[-1]}{tail}" if len(ps) > 1 else "")
    w(f"  using D = {D};")
    lam = lam or f"[](const auto&... a) {{ return std::{name}(a...); }}"
    w(f"  return ycxx::detail::simd_map<{ret.replace(D, 'D')}>(")
    w(f"      {lam}, {', '.join(args)});")
    w("}")


def mixes(arity):
    """The parameter-type mixes of the draft's overload sets, in its order."""
    if arity == 1:
        return [("V",)]
    combos = [c for c in itertools.product("VD", repeat=arity) if "V" in c]
    # draft order: all V first, then by number of D, D positions from the left
    return sorted(combos, key=lambda c: (c.count("D"), [x == "V" for x in c]))


NAMES = "xyz"
for name, ret, ce in UNARY:
    fn(name, ret, [("V", "x")], ["D(x)"], ce)
for name, ret, ce in BINARY:
    for mix in mixes(2):
        fn(name, ret, [(("V" if m == "V" else D), NAMES[i]) for i, m in enumerate(mix)],
           [f"D({NAMES[i]})" for i in range(2)], ce)
for name, ret, ce in TERNARY:
    for mix in mixes(3):
        fn(name, ret, [(("V" if m == "V" else D), NAMES[i]) for i, m in enumerate(mix)],
           [f"D({NAMES[i]})" for i in range(3)], ce, noexcept=(name == "lerp" and mix == ("V", "V", "V")))
for name, nu in UNSIGNED:
    ps = [(rebind("unsigned"), n) for n in "nm"[:nu]] + [("V", "x")]
    fn(name, D, ps, [n for n in "nm"[:nu]] + ["D(x)"], False)
for name, t in LDEXP:
    fn(name, D, [("V", "x"), (rebind(t), "n")], ["D(x)", "n"], True)

body = "\n".join(out)
out.clear()

w("""// libycxx core: <simd> part 3, the mathematical functions ([simd.math]) and complex math
// ([simd.complex.math]) of <simd>, and the using-declarations of [simd.syn] in namespace std.
// Generated by tools/gen_simd_math.py; do not edit.
#pragma once

namespace ycxx::detail {
// math-func-vec: R whose element i is f applied to element i of each argument.
template <class R, class F, class... A>
constexpr R simd_map(F f, const A&... a) {
  if constexpr (simd_mask_type<R>)
    return ycxx::detail::simd_build<R>([&](int i) { return static_cast<bool>(f(a[i]...)); });
  else
    return ycxx::detail::simd_build<R>([&](int i) { return static_cast<typename R::value_type>(f(a[i]...)); });
}
} // namespace ycxx::detail

namespace std::simd {
""")
w(body)
w("""
template <ycxx::detail::math_floating_point V>
constexpr ycxx::detail::deduced_vec_t<V> fabs(const V& x) {
  return ycxx::detail::simd_fabs(ycxx::detail::deduced_vec_t<V>(x));
}
template <ycxx::detail::math_floating_point V>
constexpr ycxx::detail::deduced_vec_t<V> abs(const V& j) {
  return ycxx::detail::simd_fabs(ycxx::detail::deduced_vec_t<V>(j));
}
template <signed_integral T, class Abi>
constexpr basic_vec<T, Abi> abs(const basic_vec<T, Abi>& j) {
  ycxx::detail::precondition(std::simd::all_of(j >= basic_vec<T, Abi>(-numeric_limits<T>::max())),
                             "std::simd::abs: the most negative value has no absolute value");
  return simd_select_impl(j < basic_vec<T, Abi>(T(0)), -j, j);
}

template <ycxx::detail::math_floating_point V>
constexpr ycxx::detail::deduced_vec_t<V> frexp(const V& value, rebind_t<int, ycxx::detail::deduced_vec_t<V>>* exp) {
  using D = ycxx::detail::deduced_vec_t<V>;
  D x(value);
  ycxx::detail::simd_array<int, D::size()> e;
  D r = ycxx::detail::simd_build<D>([&](int i) { return std::frexp(x[i], e.v + i); });
  *exp = ycxx::detail::simd_build<rebind_t<int, D>>([&](int i) { return e.v[i]; });
  return r;
}
""")
# remquo
for mix in mixes(2):
    ps = ", ".join(f"const {'V' if m == 'V' else D}& {NAMES[i]}" for i, m in enumerate(mix))
    w("template <ycxx::detail::math_floating_point V>")
    w(f"constexpr {D}")
    w(f"remquo({ps},")
    w(f"       rebind_t<int, {D}>* quo) {{")
    w(f"  using D = {D};")
    w("  D a(x), b(y);")
    w("  ycxx::detail::simd_array<int, D::size()> q;")
    w("  D r = ycxx::detail::simd_build<D>([&](int i) { return std::remquo(a[i], b[i], q.v + i); });")
    w("  *quo = ycxx::detail::simd_build<rebind_t<int, D>>([&](int i) { return q.v[i]; });")
    w("  return r;")
    w("}")
w("""template <class T, class Abi>
constexpr basic_vec<T, Abi> modf(const type_identity_t<basic_vec<T, Abi>>& value, basic_vec<T, Abi>* iptr) {
  using V = basic_vec<T, Abi>;
  ycxx::detail::simd_array<T, V::size()> ip;
  V r = ycxx::detail::simd_build<V>([&](int i) { return std::modf(value[i], ip.v + i); });
  *iptr = ycxx::detail::simd_build<V>([&](int i) { return ip.v[i]; });
  return r;
}
""")
# complex math
for name in COMPLEX_REAL:
    ne = " noexcept" if name in ("real", "imag") else ""
    w("template <ycxx::detail::simd_complex V>")
    w(f"constexpr rebind_t<typename V::value_type::value_type, V> {name}(const V& v){ne} {{")
    w(f"  return ycxx::detail::simd_map<rebind_t<typename V::value_type::value_type, V>>(")
    w(f"      [](const auto& z) {{ return std::{name}(z); }}, v);")
    w("}")
for name in COMPLEX_SAME:
    w("template <ycxx::detail::simd_complex V>")
    w(f"constexpr V {name}(const V& v) {{")
    w(f"  return ycxx::detail::simd_map<V>([](const auto& z) {{ return std::{name}(z); }}, v);")
    w("}")
w("""template <ycxx::detail::simd_floating_point V>
rebind_t<complex<typename V::value_type>, V> polar(const V& x, const V& y = {}) {
  return ycxx::detail::simd_map<rebind_t<complex<typename V::value_type>, V>>(
      [](const auto& r, const auto& t) { return std::polar(r, t); }, x, y);
}
template <ycxx::detail::simd_complex V>
constexpr V pow(const V& x, const V& y) {
  return ycxx::detail::simd_map<V>([](const auto& a, const auto& b) { return std::pow(a, b); }, x, y);
}

} // namespace std::simd

namespace std {""")
w("// [simd.syn]: the algorithms, mathematical, bit and complex functions also in namespace std.")
for n in USING_ALG + USING_MATH + USING_BIT + USING_COMPLEX:
    w(f"using simd::{n};")
w("} // namespace std")

with open(OUT, "w") as f:
    f.write("\n".join(out) + "\n")
