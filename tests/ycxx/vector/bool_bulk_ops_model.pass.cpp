// [vector.bool.pspc]: vector<bool> has the members of [vector] with the semantics of
// [sequence.reqmts], its elements packed. A randomized differential check of the bulk members
// against a vector<char> model, at positions, lengths and source offsets around the word
// boundaries of the packed representation (where a word-at-a-time implementation shifts, masks
// and splits): vector(n, value), vector(i, j), assign(n, t), assign(i, j), resize(sz, c),
// insert(p, t), insert(p, n, t), insert(p, i, j) and insert_range(p, rg) (from vector<bool>
// iterators at any bit offset, from a forward range of bool and from a sized single-pass
// range), erase(p) and erase(q1, q2), and append_range(rg), also with rg a subrange of the
// vector itself, which append_range allows ([sequence.reqmts]: unlike insert_range it has no
// precondition that rg does not overlap a), with and without reallocation. Also in constant
// evaluation (every member is constexpr) and with a fancy-pointer allocator.
#include <cstddef>
#include <ranges>
#include <vector>
#include "check.hpp"
#include "fancy_ptr.hpp"
#include "test_iterators.hpp"

namespace {

struct Rng {
  unsigned long long s;
  constexpr unsigned next() {
    s = s * 6364136223846793005ull + 1442695040888963407ull;
    return static_cast<unsigned>(s >> 33);
  }
  constexpr std::size_t below(std::size_t n) { return n == 0 ? 0 : next() % n; }
  // A length or position up to max, biased to the word boundaries.
  constexpr std::size_t pick(std::size_t max) {
    constexpr std::size_t special[] = {0, 1, 2, 31, 32, 33, 63, 64, 65, 127, 128, 129, 191, 192, 193, 255, 256, 257};
    if (next() % 2 == 0) {
      const std::size_t v = special[below(sizeof(special) / sizeof(special[0]))];
      if (v <= max) return v;
    }
    return below(max + 1);
  }
};

// A sized range whose iterator is single-pass.
struct SizedInput {
  bool* b;
  bool* e;
  constexpr InputIter<bool> begin() const { return InputIter<bool>(b); }
  constexpr PtrSentinel<bool> end() const { return PtrSentinel<bool>{e}; }
  constexpr std::size_t size() const { return static_cast<std::size_t>(e - b); }
};

template <class VB>
constexpr bool same(const VB& v, const std::vector<char>& m) {
  if (v.size() != m.size()) return false;
  for (std::size_t i = 0; i != m.size(); ++i)
    if (v[i] != (m[i] != 0)) return false;
  return true;
}

template <class VB>
constexpr VB random_bits(Rng& r, std::size_t n, std::vector<char>& m) {
  VB v;
  m.clear();
  for (std::size_t i = 0; i != n; ++i) {
    const bool b = r.next() % 3 == 0;
    v.push_back(b);
    m.push_back(b);
  }
  return v;
}

template <class VB>
constexpr bool run(unsigned long long seed, int steps, std::size_t max_len) {
  Rng r{seed};
  std::vector<char> m;
  VB v = random_bits<VB>(r, r.pick(max_len), m);
  for (int step = 0; step != steps; ++step) {
    if (r.next() % 4 == 0) v.shrink_to_fit(); // vary the spare capacity
    else if (r.next() % 4 == 0) v.reserve(v.size() + r.pick(max_len));
    // A source: another vector<bool>, and the bits [s, s + n) of it.
    std::vector<char> sm;
    const VB src = random_bits<VB>(r, r.pick(max_len), sm);
    const std::size_t s = r.pick(src.size());
    const std::size_t n = r.pick(src.size() - s);
    const auto sb = src.begin() + static_cast<std::ptrdiff_t>(s);
    const auto se = sb + static_cast<std::ptrdiff_t>(n);
    const auto mb = sm.begin() + static_cast<std::ptrdiff_t>(s);
    const auto me = mb + static_cast<std::ptrdiff_t>(n);
    const bool val = r.next() % 2 == 0;
    const std::size_t pos = r.pick(v.size());
    const auto vp = v.begin() + static_cast<std::ptrdiff_t>(pos);
    const auto mp = m.begin() + static_cast<std::ptrdiff_t>(pos);
    bool arr[300] = {};
    for (std::size_t i = 0; i != n; ++i) arr[i] = sm[s + i] != 0;
    switch (r.next() % 16) {
    case 0: v = VB(n, val); m.assign(n, val); break;
    case 1: v = VB(sb, se); m.assign(mb, me); break;
    case 2: v.assign(n, val); m.assign(n, val); break;
    case 3: v.assign(sb, se); m.assign(mb, me); break;
    case 4: {
      const std::size_t sz = r.pick(max_len);
      v.resize(sz, val);
      m.resize(sz, val);
      break;
    }
    case 5: v.insert(vp, val); m.insert(mp, val); break;
    case 6: v.insert(vp, n, val); m.insert(mp, n, val); break;
    case 7: v.insert(vp, sb, se); m.insert(mp, mb, me); break;
    case 8: v.insert_range(vp, std::ranges::subrange(sb, se)); m.insert(mp, mb, me); break;
    case 9: v.insert_range(vp, ForwardRange<bool>{arr, arr + n}); m.insert(mp, mb, me); break;
    case 10: v.insert_range(vp, SizedInput{arr, arr + n}); m.insert(mp, mb, me); break;
    case 11: {
      const std::size_t len = r.pick(v.size() - pos);
      const auto it = v.erase(vp, vp + static_cast<std::ptrdiff_t>(len));
      m.erase(mp, mp + static_cast<std::ptrdiff_t>(len));
      if (it != v.begin() + static_cast<std::ptrdiff_t>(pos)) return false;
      break;
    }
    case 12:
      if (pos != v.size()) {
        v.erase(vp);
        m.erase(mp);
      }
      break;
    case 13: v.append_range(std::ranges::subrange(sb, se)); m.insert(m.end(), mb, me); break;
    case 14: {
      // append_range of a part of the vector itself.
      const std::size_t a = r.pick(v.size()), len = r.pick(v.size() - a);
      const std::vector<char> part(m.begin() + static_cast<std::ptrdiff_t>(a),
                                   m.begin() + static_cast<std::ptrdiff_t>(a + len));
      v.append_range(std::ranges::subrange(v.begin() + static_cast<std::ptrdiff_t>(a),
                                           v.begin() + static_cast<std::ptrdiff_t>(a + len)));
      m.insert(m.end(), part.begin(), part.end());
      break;
    }
    default: v.assign_range(ForwardRange<bool>{arr, arr + n}); m.assign(mb, me); break;
    }
    if (!same(v, m)) return false;
    if (v.size() > 4 * max_len) { // keep the sizes bounded
      v.erase(v.begin() + static_cast<std::ptrdiff_t>(max_len), v.end());
      m.resize(max_len);
    }
  }
  return true;
}

using VB = std::vector<bool>;
using FancyVB = std::vector<bool, FancyAlloc<bool>>;

} // namespace

static_assert(run<VB>(1, 30, 140));
static_assert(run<FancyVB>(2, 15, 140));

int main() {
  for (unsigned long long seed = 1; seed != 41; ++seed) {
    CHECK_SAY(run<VB>(seed, 400, 300), "seed %llu", seed);
    CHECK_SAY(run<FancyVB>(seed + 1000, 100, 300), "seed %llu", seed + 1000);
  }
}
