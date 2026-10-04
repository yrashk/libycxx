// [valarray.sub] Examples 1-8 verbatim (const subscripting returns a valarray, non-const
// subscripting returns slice_array/gslice_array/mask_array/indirect_array with reference
// semantics). Then assignments between two subset objects of the same valarray whose selected
// elements do not overlap: [slice.arr.assign]/1, [gslice.array.assign]/1, [mask.array.assign]/1,
// [indirect.array.assign]/1: "const X& operator=(const X&) const" assigns "the values of the
// argument array elements to selected elements of the valarray object to which [*this] refers"
// (reference semantics: neither operand is a copy), and the compound assignments
// ([slice.arr.comp.assign] etc.) with a valarray built from another subset of the same object.
// Also: a subset object copied (copy constructor) still refers to the same valarray.
#include <cstddef>
#include <valarray>
#include "check.hpp"

using V = std::valarray<char>;
using I = std::valarray<std::size_t>;
using B = std::valarray<bool>;

bool eq(const V& v, const char* s) {
  for (std::size_t i = 0; i < v.size(); ++i)
    if (v[i] != s[i] || s[i] == 0) return false;
  return s[v.size()] == 0;
}

int main() {
  const std::size_t lv[] = {2, 3};
  const std::size_t dv[] = {7, 2};
  const I len(lv, 2), str(dv, 2);
  const bool vb[] = {false, false, true, true, false, true};
  const std::size_t vi[] = {7, 5, 2, 3, 8};
  {
    const V v0("abcdefghijklmnop", 16);
    CHECK(eq(v0[std::slice(2, 5, 3)], "cfilo"));           // Example 1
    CHECK(eq(v0[std::gslice(3, len, str)], "dfhkmo"));     // Example 3
    CHECK(eq(v0[B(vb, 6)], "cdf"));                         // Example 5
    CHECK(eq(v0[I(vi, 5)], "hfcdi"));                       // Example 7
  }
  {
    V v0("abcdefghijklmnop", 16);
    v0[std::slice(2, 5, 3)] = V("ABCDE", 5);  // Example 2
    CHECK(eq(v0, "abAdeBghCjkDmnEp"));
  }
  {
    V v0("abcdefghijklmnop", 16);
    v0[std::gslice(3, len, str)] = V("ABCDEF", 6);  // Example 4
    CHECK(eq(v0, "abcAeBgCijDlEnFp"));
  }
  {
    V v0("abcdefghijklmnop", 16);
    v0[B(vb, 6)] = V("ABC", 3);  // Example 6
    CHECK(eq(v0, "abABeCghijklmnop"));
  }
  {
    V v0("abcdefghijklmnop", 16);
    v0[I(vi, 5)] = V("ABCDE", 5);  // Example 8
    CHECK(eq(v0, "abCDeBgAEjklmnop"));
  }

  // subset = subset of the same valarray, disjoint selections
  {
    V v("abcdefgh", 8);
    v[std::slice(0, 4, 2)] = v[std::slice(1, 4, 2)];  // even positions <- odd positions
    CHECK(eq(v, "bbddffhh"));
  }
  {
    V v("abcdefgh", 8);
    const std::size_t l[] = {2, 2}, s1[] = {4, 1};
    const I L(l, 2), S(s1, 2);
    v[std::gslice(0, L, S)] = v[std::gslice(2, L, S)];  // {0,1,4,5} <- {2,3,6,7}
    CHECK(eq(v, "cdcdghgh"));
  }
  {
    V v("abcdef", 6);
    const bool lo[] = {true, true, true, false, false, false};
    const bool hi[] = {false, false, false, true, true, true};
    v[B(lo, 6)] = v[B(hi, 6)];
    CHECK(eq(v, "defdef"));
  }
  {
    V v("abcdef", 6);
    const std::size_t to[] = {0, 2, 4}, from[] = {5, 3, 1};
    v[I(to, 3)] = v[I(from, 3)];
    CHECK(eq(v, "fbddbf"));
  }
  // compound assignment from another subset (converted to a valarray first)
  {
    std::valarray<int> w = {1, 2, 3, 4, 5, 6};
    w[std::slice(0, 3, 1)] += std::valarray<int>(w[std::slice(3, 3, 1)]);
    CHECK(w[0] == 5 && w[1] == 7 && w[2] == 9 && w[3] == 4 && w[5] == 6);
    const std::size_t odd[] = {1, 3, 5}, even[] = {0, 2, 4};
    w[I(odd, 3)] *= std::valarray<int>(w[I(even, 3)]);
    CHECK(w[1] == 35 && w[3] == 36 && w[5] == 30 && w[0] == 5);
    const bool m[] = {true, false, true, false, true, false};
    w[B(m, 6)] -= std::valarray<int>(1, 3);
    CHECK(w[0] == 4 && w[2] == 8 && w[4] == 4 && w[1] == 35);
  }
  // copies of a subset object refer to the same valarray
  {
    V v("abcd", 4);
    std::slice_array<char> sa = v[std::slice(1, 2, 1)];
    std::slice_array<char> sb(sa);
    sb = V("XY", 2);
    CHECK(eq(v, "aXYd"));
    const I three(vi + 3, 1);  // element 3; kept alive while ia and ib exist
    std::indirect_array<char> ia = v[three];
    std::indirect_array<char> ib(ia);
    ib = V("Z", 1);
    CHECK(eq(v, "aXYZ"));
  }
  return 0;
}
