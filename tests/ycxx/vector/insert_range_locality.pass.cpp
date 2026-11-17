// vector::insert(p, i, j) / insert_range(p, rg) in the middle without reallocation
// ([sequence.reqmts]: "Inserts copies of elements in [i, j) before p"; [vector.modifiers]/1:
// linear in the number of elements inserted plus the distance to the end of the vector) must
// move the elements after p sequentially, as insert(p, n, t) does: not by following the cycles
// of a rotation, which visits them with a stride of the rotation amount (one cache miss per
// element moved once the vector outgrows the cache). Measured with an element type that logs
// its assignments (support/move_locality.hpp): at most a few far jumps, and a number of
// assignments and constructions linear in the elements moved, for forward, same-type, input and
// sized input sources at various positions and lengths; the result is checked against a model.
// Also: an exception from constructing an inserted element leaves a valid vector and no
// element constructed but not destroyed ([vector.modifiers]/2 gives only the basic guarantee
// then), and the insertions in constant evaluation.
// REQUIRES: exceptions
#include <cstddef>
#include <vector>
#include "check.hpp"
#include "move_locality.hpp"
#include "test_iterators.hpp"

using locality::E;
using locality::stats;

enum Source { forward, same_type, input, sized_input };

static void check_insert(Source kind, long n0, long pos, long k) {
  std::vector<E> v;
  v.reserve(static_cast<std::size_t>(n0 + k));
  for (long i = 0; i != n0; ++i) v.emplace_back(static_cast<int>(i));
  std::vector<int> src;
  for (long i = 0; i != k; ++i) src.push_back(static_cast<int>(100000 + i));
  std::vector<E> src_e(src.begin(), src.end());
  const auto cap = v.capacity();
  const auto p = v.begin() + pos;
  locality::start();
  std::vector<E>::iterator r;
  switch (kind) {
  case forward: r = v.insert(p, src.begin(), src.end()); break;
  case same_type: r = v.insert_range(p, src_e); break;
  case input: r = v.insert_range(p, InputRange<int>{src.data(), src.data() + k}); break;
  case sized_input:
    r = v.insert_range(p, locality::SizedInput<InputIter<int>, int>{src.data(), src.data() + k});
    break;
  }
  const long ctors = stats.ctors, assigns = stats.assigns;
  const long jumps = locality::far_jumps(v);
  const long moved = n0 - pos + k;
  CHECK_SAY(jumps <= 8, "source %d n0 %ld pos %ld k %ld: %ld far jumps", kind, n0, pos, k, jumps);
  CHECK_SAY(assigns <= 3 * moved + 8 && ctors <= 3 * k + (n0 - pos) + 8,
            "source %d n0 %ld pos %ld k %ld: %ld assignments, %ld constructions", kind, n0, pos, k, assigns, ctors);
  CHECK(v.capacity() == cap);
  CHECK(r == v.begin() + pos);
  CHECK(static_cast<long>(v.size()) == n0 + k);
  for (long i = 0; i != n0 + k; ++i) {
    const int want = i < pos ? static_cast<int>(i) : i < pos + k ? static_cast<int>(100000 + i - pos) : static_cast<int>(i - k);
    CHECK(v[static_cast<std::size_t>(i)].v == want);
  }
}

// The constructions of the inserted elements from int throw at the t-th.
struct Bomb {
  int v;
};
struct F : E {
  static inline long countdown = -1;
  F(int x) : E(x) {}
  F(Bomb b) : E(b.v) {
    if (countdown >= 0 && countdown-- == 0) throw 7;
  }
};

static void check_exceptions() {
  for (long pos : {1L, 10L, 19L})
    for (long k : {3L, 18L, 40L})
      for (long t = 0; t != k; ++t) {
        {
          std::vector<F> v;
          v.reserve(64);
          for (int i = 0; i != 20; ++i) v.emplace_back(i);
          std::vector<Bomb> src;
          for (long i = 0; i != k; ++i) src.push_back(Bomb{static_cast<int>(1000 + i)});
          F::countdown = t;
          bool threw = false;
          try {
            v.insert(v.begin() + pos, src.begin(), src.end());
          } catch (int) {
            threw = true;
          }
          F::countdown = -1;
          CHECK(threw);
          CHECK(stats.live == static_cast<long>(v.size()));
          CHECK(v.size() >= 20 && v.size() <= static_cast<std::size_t>(20 + k));
          for (const F& f : v) CHECK((f.v >= 0 && f.v < 20) || (f.v >= 1000 && f.v < 1000 + k));
        }
        CHECK(stats.live == 0);
      }
}

// In constant evaluation, with a type constructible from the source's elements but not
// assignable from them, at every position of a vector with spare capacity.
struct OnlyFromInt {
  int v;
  constexpr OnlyFromInt(int x) : v(x) {}
  OnlyFromInt& operator=(int) = delete;
};
constexpr bool constexpr_inserts() {
  for (int k = 1; k != 8; ++k)
    for (int pos = 0; pos <= 5; ++pos) {
      std::vector<OnlyFromInt> v;
      v.reserve(20);
      for (int i = 0; i != 5; ++i) v.emplace_back(i);
      int src[8] = {10, 11, 12, 13, 14, 15, 16, 17};
      v.insert(v.begin() + pos, ForwardIter<int>(src), ForwardIter<int>(src + k));
      v.insert_range(v.begin() + pos, InputRange<int>{src, src + k});
      if (static_cast<int>(v.size()) != 5 + 2 * k) return false;
      for (int i = 0; i != 5 + 2 * k; ++i) {
        const int j = i - pos;
        const int want = j < 0 ? i : j < k ? 10 + j : j < 2 * k ? 10 + j - k : i - 2 * k;
        if (v[static_cast<std::size_t>(i)].v != want) return false;
      }
    }
  return true;
}
static_assert(constexpr_inserts());

int main() {
  for (Source kind : {forward, same_type, input, sized_input})
    for (long pos : {0L, 1L, 700L, 1999L, 2000L})
      for (long k : {1L, 2L, 7L, 100L, 1000L, 3000L}) check_insert(kind, 2000, pos, k);
  CHECK(stats.live == 0);
  check_exceptions();
  CHECK(constexpr_inserts());
}
