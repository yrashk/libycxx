// vector<bool> from single-pass input iterators and input ranges, at sizes around the word
// boundaries of the packed representation ([vector.bool.pspc]: "elements are packed").
// [sequence.reqmts]: X(i, j) "Constructs a sequence container equal to the range [i, j). Each
// iterator in the range [i, j) shall be dereferenced exactly once."; X(from_range, rg)
// likewise ("Each iterator in the range rg is dereferenced exactly once"); a.insert(p, i, j)
// "Inserts copies of elements in [i, j) before p. Each iterator in the range [i, j) shall be
// dereferenced exactly once."; a.insert_range(p, rg); a.assign(i, j) "Replaces elements in a
// with a copy of [i, j)" (each iterator dereferenced exactly once); a.assign_range(rg);
// a.append_range(rg) "Inserts copies of elements in rg before end()". vector<bool> has all of
// these members ([vector.bool.pspc] synopsis) with the semantics of [sequence.reqmts].
// An input iterator cannot be measured in advance, so the container must grow while reading.
#include <vector>
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool pattern(int i) { return (i * 7 + i / 3) % 5 < 2; }

constexpr bool equal_to(const std::vector<bool>& v, const bool* src, int n) {
  if (v.size() != static_cast<std::size_t>(n)) return false;
  for (int i = 0; i < n; ++i)
    if (v[static_cast<std::size_t>(i)] != src[i]) return false;
  return true;
}

constexpr bool test(int n, bool full = true) {
  bool src[300]{};
  for (int i = 0; i < n; ++i) src[i] = pattern(i);
  int derefs = 0;

  std::vector<bool> a(InputIter<bool>(src, &derefs), InputIter<bool>(src + n));
  if (!equal_to(a, src, n) || derefs != n) return false;

  derefs = 0;
  std::vector<bool> b(std::from_range, InputRange<bool>{src, src + n, &derefs});
  if (!equal_to(b, src, n) || derefs != n) return false;

  // assign over contents of a different size, larger and smaller
  for (int pre : {0, 1, 64, 130, 299}) {
    std::vector<bool> c(static_cast<std::size_t>(pre), true);
    derefs = 0;
    c.assign(InputIter<bool>(src, &derefs), InputIter<bool>(src + n));
    if (!equal_to(c, src, n) || derefs != n) return false;
    std::vector<bool> d(static_cast<std::size_t>(pre), true);
    derefs = 0;
    d.assign_range(InputRange<bool>{src, src + n, &derefs});
    if (!equal_to(d, src, n) || derefs != n) return false;
  }

  // insert into the middle of an existing vector, across word boundaries
  for (int pre : {0, 1, 63, 64, 65, 129}) {
    if (!full && pre != 0 && pre != 65) continue;
    for (int at : {0, pre / 2, pre}) {
      bool expect[600]{};
      int k = 0;
      for (int i = 0; i < at; ++i) expect[k++] = (i % 3 == 0);
      for (int i = 0; i < n; ++i) expect[k++] = src[i];
      for (int i = at; i < pre; ++i) expect[k++] = (i % 3 == 0);
      std::vector<bool> base;
      for (int i = 0; i < pre; ++i) base.push_back(i % 3 == 0);

      std::vector<bool> e = base;
      derefs = 0;
      auto it = e.insert(e.cbegin() + at, InputIter<bool>(src, &derefs), InputIter<bool>(src + n));
      if (!equal_to(e, expect, k) || derefs != n || it != e.begin() + at) return false;

      std::vector<bool> f = base;
      derefs = 0;
      auto jt = f.insert_range(f.cbegin() + at, InputRange<bool>{src, src + n, &derefs});
      if (!equal_to(f, expect, k) || derefs != n || jt != f.begin() + at) return false;
    }
    std::vector<bool> g;
    for (int i = 0; i < pre; ++i) g.push_back(i % 3 == 0);
    derefs = 0;
    g.append_range(InputRange<bool>{src, src + n, &derefs});
    if (g.size() != static_cast<std::size_t>(pre + n) || derefs != n) return false;
    for (int i = 0; i < n; ++i)
      if (g[static_cast<std::size_t>(pre + i)] != src[i]) return false;
  }
  return true;
}

constexpr bool all() {
  for (int n : {0, 1, 2, 31, 32, 33, 63, 64, 65, 127, 128, 129, 200, 300})
    if (!test(n)) return false;
  return true;
}

// a few sizes during constant evaluation (all of them would exceed the default step limits
// with an implementation that inserts input-iterator elements one at a time)
static_assert(test(0, false) && test(1, false) && test(65, false));

int main() {
  CHECK(all());
  return 0;
}
