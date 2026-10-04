// Exception-injection sweep over valarray's constructors, assignments, resize and the members
// and helper arrays that create new valarrays (shift, cshift, apply, slice/gslice/mask/
// indirect selections): the element type's constructors and assignments and operator new
// throw at their k-th call, for every k.
//   [res.on.exception.handling]/1 and [valarray.cons], [valarray.assign], [valarray.members]:
//     no particular guarantee is stated, so the checks are the basic ones: after every run each
//     element object constructed has been destroyed exactly once and every operator new block
//     freed; a valarray that was being assigned to or resized is still a valid object whose
//     size() elements are all alive.
#include <cstddef>
#include <valarray>
#include "exc_new.hpp"

using namespace exh;
using VA = std::valarray<T>;

static const T src_g[6] = {T(1), T(2), T(3), T(4), T(5), T(6)};

template <class F>
void sw(const char* name, std::initializer_list<Kind> ks, F f) {
  for (Kind k : ks) {
    if (k == gnew)
      sweep_new(name, f);
    else
      sweep(name, k, new_balanced(f));
  }
}

static void check_alive(const VA& v) {
  for (std::size_t i = 0; i < v.size(); ++i) v[i].use("valarray element after an exception:");
}

int main() {
  const auto K = {copy_ctor, default_ctor, copy_assign, gnew};
  sw("valarray(n)", {default_ctor, gnew}, [] { return attempt([] { VA v(7); }); });
  sw("valarray(const T&, n)", {copy_ctor, gnew}, [] { return attempt([] { VA v(src_g[0], 7); }); });
  sw("valarray(const T*, n)", {copy_ctor, gnew}, [] { return attempt([] { VA v(src_g, 6); }); });
  sw("valarray(il)", {copy_ctor, gnew}, [] { return attempt([] { VA v{src_g[0], src_g[1], src_g[2]}; }); });
  sw("valarray(const valarray&)", K, [] {
    VA a(src_g, 6);
    return attempt([&] { VA b(a); });
  });
  for (int n : {3, 6, 9})
    sw("valarray::operator=(const valarray&) of another size", K, [n] {
      VA a(src_g, 6);
      VA b(src_g[2], std::size_t(n));
      bool threw = attempt([&] { b = a; });
      check_alive(b);
      return threw;
    });
  sw("valarray::operator=(const T&)", K, [] {
    VA b(src_g, 6);
    bool threw = attempt([&] { b = src_g[5]; });
    check_alive(b);
    return threw;
  });
  for (int n : {2, 6, 10})
    sw("valarray::resize(n, c)", K, [n] {
      VA b(src_g, 6);
      bool threw = attempt([&] { b.resize(std::size_t(n), src_g[1]); });
      check_alive(b);
      return threw;
    });
  sw("valarray::shift(2)", K, [] {
    VA b(src_g, 6);
    return attempt([&] { VA r = b.shift(2); });
  });
  sw("valarray::cshift(-2)", K, [] {
    VA b(src_g, 6);
    return attempt([&] { VA r = b.cshift(-2); });
  });
  sw("valarray::apply", K, [] {
    VA b(src_g, 6);
    return attempt([&] { VA r = b.apply([](T x) { return x; }); });
  });
  sw("valarray(slice_array)", K, [] {
    VA b(src_g, 6);
    return attempt([&] { VA r = b[std::slice(1, 3, 2)]; });
  });
  sw("valarray(gslice_array)", K, [] {
    VA b(src_g, 6);
    std::size_t len[2] = {2, 2}, str[2] = {3, 1};
    std::gslice g(0, std::valarray<std::size_t>(len, 2), std::valarray<std::size_t>(str, 2));
    return attempt([&] { VA r = b[g]; });
  });
  sw("valarray(mask_array)", K, [] {
    VA b(src_g, 6);
    bool m[6] = {true, false, true, true, false, true};
    std::valarray<bool> mask(m, 6);
    return attempt([&] { VA r = b[mask]; });
  });
  sw("valarray(indirect_array)", K, [] {
    VA b(src_g, 6);
    std::size_t ix[4] = {5, 0, 3, 1};
    std::valarray<std::size_t> idx(ix, 4);
    return attempt([&] { VA r = b[idx]; });
  });
  sw("slice_array = valarray", K, [] {
    VA b(src_g, 6);
    VA c(src_g[3], 3);
    bool threw = attempt([&] { b[std::slice(0, 3, 2)] = c; });
    check_alive(b);
    return threw;
  });
  sw("valarray swap/move", K, [] {
    VA a(src_g, 6), b(src_g[0], 2);
    return attempt([&] {
      a.swap(b);
      VA c(std::move(a));
    });
  });
  return finish();
}
