// Equality-based algorithms and container comparisons use the element type's operator==, not a
// comparison of object representations: [alg.equal] equal / ranges::equal compare with
// "*i == *(first2 + (i - first1))"; [mismatch], [alg.find], [alg.count], [alg.search] likewise;
// [alg.lex.comparison] lexicographical_compare uses <, lexicographical_compare_three_way uses
// <=>; [container.reqmts] a == b is equal(a.begin(), a.end(), b.begin(), b.end()) and array
// likewise. So 0.0 == -0.0 (different bits) and NaN != NaN (same bits) for floating point
// ([expr.eq], IEEE 754), a program-defined == that ignores a member is honoured, and padding
// bytes of a class with a defaulted == do not matter ([class.compare.default]: memberwise).
#include <algorithm>
#include <array>
#include <cmath>
#include <compare>
#include <deque>
#include <limits>
#include <span>
#include <vector>
#include "check.hpp"
struct P {
  int key;
  int ignored;
  friend bool operator==(const P& a, const P& b) { return a.key == b.key; }
};
struct Pad {
  char c;
  int i;
  friend bool operator==(const Pad&, const Pad&) = default;
};
int main() {
  auto chk = [](const char* n, bool c) {
    if (!c) dprintf(2, "%s\n", n);
    CHECK(c);
  };
  double z[4] = {0.0, 1, 2, 3}, nz[4] = {-0.0, 1, 2, 3};
  double nan = std::numeric_limits<double>::quiet_NaN();
  double n1[4] = {nan, 1, 2, 3}, n2[4] = {nan, 1, 2, 3};
  chk("equal(0.0 vs -0.0)", std::equal(z, z + 4, nz));
  chk("ranges::equal(0.0 vs -0.0)", std::ranges::equal(z, nz));
  chk("equal(NaN vs same NaN bits) false", !std::equal(n1, n1 + 4, n2));
  chk("ranges::equal NaN false", !std::ranges::equal(n1, n2));
  chk("equal NaN self false", !std::equal(n1, n1 + 4, n1));
  chk("mismatch at NaN", std::mismatch(n1, n1 + 4, n2).first == n1);
  chk("find -0.0 for 0.0", std::find(nz, nz + 4, 0.0) == nz);
  chk("find NaN not found", std::find(n1, n1 + 4, nan) == n1 + 4);
  chk("count 0.0 in -0.0", std::count(nz, nz + 4, 0.0) == 1);
  float fz[3] = {0.0f, 1, 2}, fnz[3] = {-0.0f, 1, 2};
  chk("equal float 0/-0", std::equal(fz, fz + 3, fnz));
  chk("lexicographical_compare(0, -0) equal", !std::lexicographical_compare(fz, fz + 3, fnz, fnz + 3) && !std::lexicographical_compare(fnz, fnz + 3, fz, fz + 3));
  chk("lexicographical_compare_three_way 0/-0", std::lexicographical_compare_three_way(fz, fz + 3, fnz, fnz + 3) == std::partial_ordering::equivalent);
  P p1[3] = {{1, 10}, {2, 20}, {3, 30}}, p2[3] = {{1, 11}, {2, 21}, {3, 31}};
  chk("equal custom ==", std::equal(p1, p1 + 3, p2));
  chk("ranges::equal custom ==", std::ranges::equal(p1, p2));
  chk("find custom ==", std::find(p1, p1 + 3, P{2, 99}) == p1 + 1);
  chk("search custom ==", std::search(p1, p1 + 3, p2 + 1, p2 + 3) == p1 + 1);
  chk("mismatch custom ==", std::mismatch(p1, p1 + 3, p2).first == p1 + 3);
  std::vector<P> v1(p1, p1 + 3), v2(p2, p2 + 3);
  chk("vector<P> ==", v1 == v2);
  std::array<P, 3> a1{{{1, 10}, {2, 20}, {3, 30}}}, a2{{{1, 11}, {2, 21}, {3, 31}}};
  chk("array<P> ==", a1 == a2);
  std::vector<double> d1(z, z + 4), d2(nz, nz + 4);
  chk("vector<double> == (0/-0)", d1 == d2);
  std::vector<double> e1(n1, n1 + 4), e2(n2, n2 + 4);
  chk("vector<double> NaN !=", !(e1 == e2));
  std::array<double, 4> ad1{nan, 1, 2, 3};
  chk("array<double> NaN self !=", !(ad1 == ad1));
  std::deque<double> q1(z, z + 4), q2(nz, nz + 4);
  chk("deque<double> == (0/-0)", q1 == q2);
  Pad x[2], y[2];
  for (int k = 0; k < 2; ++k) {
    unsigned char* bx = reinterpret_cast<unsigned char*>(&x[k]);
    unsigned char* by = reinterpret_cast<unsigned char*>(&y[k]);
    for (unsigned j = 0; j < sizeof(Pad); ++j) { bx[j] = 0xAA; by[j] = 0x55; }
    x[k].c = y[k].c = 'q';
    x[k].i = y[k].i = k;
  }
  chk("equal padded struct (padding differs)", std::equal(x, x + 2, y));
}
