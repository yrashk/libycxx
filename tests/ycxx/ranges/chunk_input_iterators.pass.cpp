// chunk_view over an input-only range, whose outer and inner iterators share the view's
// position (current_) and the count left in the current chunk (remainder_):
// [range.chunk.view.input]/3: begin() sets remainder_ to n; /5: size() is
// div-ceil(distance, n).
// [range.chunk.outer.iter]/5: ++ advances current_ by what is left of the chunk (at most to
// the end) and resets remainder_ to n; /7: == default_sentinel when current_ is at the end and
// remainder_ != 0; /8: default_sentinel - it is 0 or 1 if fewer than remainder_ elements are
// left, else div-ceil(dist - remainder_, n) + 1 (the current chunk, even when consumed, counts).
// [range.chunk.outer.value]/4: a chunk's size() is min(remainder_, elements left).
// [range.chunk.inner.iter]/6: ++ moves current_ and decrements remainder_ (to 0 at the end);
// /8: == default_sentinel when remainder_ is 0; /9: default_sentinel - it is
// min(remainder_, elements left); /11-/12: iter_move and iter_swap act on current_.
#include <ranges>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>
#include "check.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

int main() {
  std::vector<int> v{1, 2, 3, 4, 5, 6, 7};
  auto in = v | vw::as_input;
  static_assert(rg::input_range<decltype(in)> && !rg::forward_range<decltype(in)>);
  auto c = in | vw::chunk(3);
  static_assert(std::is_same_v<rg::sentinel_t<decltype(c)>, std::default_sentinel_t>);
  CHECK(c.size() == 3);

  auto it = c.begin();
  CHECK(std::default_sentinel - it == 3 && it - std::default_sentinel == -3);
  {
    auto chunk = *it;
    CHECK(chunk.size() == 3);
    auto i = chunk.begin();
    CHECK(*i == 1 && std::default_sentinel - i == 3);
    ++i;
    CHECK(*i == 2 && chunk.size() == 2 && std::default_sentinel - i == 2 && i - std::default_sentinel == -2);
  }
  ++it;  // skips the rest of the first chunk (3)
  CHECK(!(it == std::default_sentinel) && std::default_sentinel - it == 2);
  {
    auto chunk = *it;
    int seen[3], k = 0;
    for (auto i = chunk.begin(); i != std::default_sentinel; ++i) seen[k++] = *i;
    CHECK(k == 3 && seen[0] == 4 && seen[2] == 6);
    CHECK(chunk.size() == 0);
  }
  // the consumed chunk still counts until the outer iterator moves
  CHECK(!(it == std::default_sentinel) && std::default_sentinel - it == 2);
  ++it;
  CHECK(std::default_sentinel - it == 1);
  {
    auto chunk = *it;
    CHECK(chunk.size() == 1);
    auto i = chunk.begin();
    CHECK(*i == 7);
    ++i;  // reaches the end: remainder_ becomes 0
    CHECK(i == std::default_sentinel && std::default_sentinel - i == 0);
  }
  CHECK(!(it == std::default_sentinel));  // remainder_ is 0: the consumed chunk is still current
  CHECK(std::default_sentinel - it == 1);  // dist 0, not < remainder_ 0: div-ceil(0, 3) + 1
  ++it;
  CHECK(it == std::default_sentinel && std::default_sentinel - it == 0);

  // begin() restarts the counting; an exact multiple of n
  std::vector<int> six{1, 2, 3, 4, 5, 6};
  auto c6 = six | vw::as_input | vw::chunk(2);
  auto o = c6.begin();
  CHECK(std::default_sentinel - o == 3 && c6.size() == 3);
  ++o, ++o, ++o;
  CHECK(o == std::default_sentinel);

  // iter_move and iter_swap through inner iterators
  std::vector<std::unique_ptr<int>> ps;
  for (int i = 0; i < 4; ++i) ps.push_back(std::make_unique<int>(i));
  auto cp = ps | vw::as_input | vw::chunk(4);
  auto po = cp.begin();
  auto pc = *po;
  auto pi = pc.begin();
  std::unique_ptr<int> taken = rg::iter_move(pi);
  CHECK(taken && *taken == 0 && !ps[0]);
  static_assert(std::is_same_v<decltype(rg::iter_move(pi)), std::unique_ptr<int>&&>);
  ++pi;
  rg::iter_swap(pi, pi);
  CHECK(*ps[1] == 1);

  std::vector<int> sw{1, 2};
  auto cs = sw | vw::as_input | vw::chunk(2);
  auto so = cs.begin();
  auto sc = *so;
  auto si = sc.begin();
  int& first = *si;
  rg::iter_swap(si, si);
  CHECK(first == 1 && *si == 1);
  return 0;
}
