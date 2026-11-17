// [range.stride.iterator], [range.chunk.fwd.iter]: operator+= over a random-access base is O(1)
// (beyond the n steps advance(current, n, end) may take when the sentinel is not sized): it does
// not measure the distance to the end. With an unbounded base, iota(0) | stride(3), that distance
// is infinite; with a sentinel that is not sized, measuring it walks the whole rest of the range.
#include <cstddef>
#include <iterator>
#include <ranges>
#include "check.hpp"
#include "watchdog.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

// The end of an int array that counts its comparisons with an iterator and is not sized.
struct CountingEnd {
  const int* end;
  std::size_t* count;
  friend bool operator==(const int* p, const CountingEnd& s) {
    ++*s.count;
    return p == s.end;
  }
};
static_assert(std::sentinel_for<CountingEnd, const int*> && !std::sized_sentinel_for<CountingEnd, const int*>);

int main() {
  watchdog(10); // the unbounded cases used to loop forever
  {
    auto s = vw::iota(0) | vw::stride(3);
    auto it = s.begin();
    it += 5;
    CHECK(*it == 15);
    CHECK(*(it + 2) == 21 && it[1] == 18);
    it -= 4;
    CHECK(*it == 3);
  }
  {
    auto s = vw::iota(0) | vw::chunk(4);
    auto it = s.begin();
    it += 3;
    CHECK(*rg::begin(*it) == 12);
    CHECK(*rg::begin(*(it + 1)) == 16 && *rg::begin(it[2]) == 20);
  }
  static int a[1000];
  for (int i = 0; i != 1000; ++i)
    a[i] = i;
  {
    std::size_t count = 0;
    auto s = rg::subrange<const int*, CountingEnd>(a, CountingEnd{a + 1000, &count}) | vw::stride(3);
    auto it = s.begin();
    count = 0;
    it += 5;
    CHECK(*it == 15);
    CHECK_SAY(count <= 4, "stride_view += compared with the end %zu times", count);
  }
  {
    std::size_t count = 0;
    auto s = rg::subrange<const int*, CountingEnd>(a, CountingEnd{a + 1000, &count}) | vw::chunk(4);
    auto it = s.begin();
    count = 0;
    it += 3;
    CHECK(*rg::begin(*it) == 12);
    CHECK_SAY(count <= 5, "chunk_view += compared with the end %zu times", count);
  }
}
