// [range.common.view]: common_view<V> for a non-common V uses common_iterator<iterator_t<V>,
// sentinel_t<V>> (unless V is sized and random access, where end() is begin() + size()).
// [common.iter.nav]/5: common_iterator::operator++(int): "If requires(I& i) { { *i++ } ->
// can-reference; } is true or indirectly_readable<I> && constructible_from<iter_value_t<I>,
// iter_reference_t<I>> && move_constructible<iter_value_t<I>> is false, equivalent to: return
// get<I>(v_)++; Otherwise, equivalent to: postfix-proxy p(**this); ++*this; return p;" where
// postfix-proxy's operator* returns the kept value. [common.iter.access]/5: operator->
// returns the iterator itself if it has operator-> (or is a pointer), else the address of the
// referenced object when iter_reference_t is a reference, else a proxy keeping the value.
// (istream_view cannot be used: common_view requires copyable iterators.)
// [common.iter.types]: iterator_concept is forward_iterator_tag if I models forward_iterator,
// else input_iterator_tag; iterator_category is forward_iterator_tag if
// iterator_traits<I>::iterator_category is valid and derived from forward_iterator_tag, else
// input_iterator_tag.
#include <iterator>
#include <list>
#include <numeric>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"

struct VoidPost {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  using iterator_concept = std::input_iterator_tag;
  int* p = nullptr;
  int operator*() const { return *p; }
  VoidPost& operator++() {
    ++p;
    return *this;
  }
  void operator++(int) { ++p; }
};
struct End {
  int* p = nullptr;
  friend bool operator==(const VoidPost& i, const End& e) { return i.p == e.p; }
};
static_assert(std::input_iterator<VoidPost> && !std::forward_iterator<VoidPost> && std::copyable<VoidPost>);

int main() {
  {
    // A copyable input-only iterator whose operator++(int) returns void, with a sentinel of
    // another type: not *i++, so common_iterator's postfix ++ returns a postfix-proxy.
    static int data[] = {10, 20, 30, 40};
    auto cv = std::ranges::subrange(VoidPost{data}, End{data + 4}) | std::views::common;
    using CI = std::ranges::iterator_t<decltype(cv)>;
    static_assert(std::is_same_v<CI, std::common_iterator<VoidPost, End>>);
    static_assert(std::is_same_v<CI, std::ranges::sentinel_t<decltype(cv)>>);
    static_assert(std::is_same_v<std::iterator_traits<CI>::iterator_category, std::input_iterator_tag>);
    static_assert(std::is_same_v<std::iterator_traits<CI>::iterator_concept, std::input_iterator_tag>);
    auto it = cv.begin();
    auto old = it++;  // postfix-proxy: keeps the value read before the increment
    static_assert(!std::is_same_v<decltype(old), CI> && !std::is_void_v<decltype(old)>);
    static_assert(std::is_same_v<decltype(*old), const int&>);
    CHECK(*old == 10 && *it == 20);
    ++it;
    CHECK(*it == 30);
    std::vector<int> rest(it, cv.end());  // classic iterator-pair constructor
    CHECK((rest == std::vector<int>{30, 40}));
  }
  {
    // Forward base: postfix returns a copy of the common_iterator.
    std::list<int> l = {1, 2, 3, 4, 5, 6};
    auto cv = l | std::views::take_while([](int x) { return x < 5; }) | std::views::common;
    using CI = std::ranges::iterator_t<decltype(cv)>;
    static_assert(std::is_same_v<std::iterator_traits<CI>::iterator_category, std::forward_iterator_tag>);
    auto it = cv.begin();
    auto old = it++;
    static_assert(std::is_same_v<decltype(old), CI>);
    CHECK(*old == 1 && *it == 2);
    CHECK(std::accumulate(cv.begin(), cv.end(), 0) == 10);
    CHECK(it.operator->() == std::next(l.begin()));  // the iterator has operator->: 5.1
  }
  {
    // Prvalue elements: operator-> returns a proxy keeping the value.
    auto sq = std::views::iota(0) | std::views::transform([](int i) { return std::pair<int, int>(i, i * i); }) |
              std::views::take_while([](const std::pair<int, int>& p) { return p.first < 4; }) | std::views::common;
    auto it = sq.begin();
    ++it;
    ++it;
    CHECK(it->first == 2 && it->second == 4);
    int sum = 0;
    for (auto i = sq.begin(); i != sq.end(); ++i) sum += i->second;
    CHECK(sum == 0 + 1 + 4 + 9);
    // forward base with prvalue reference: postfix returns a copy of the iterator.
    auto old = it++;
    CHECK(old->second == 4 && it->second == 9);
  }
  {
    // Sized random-access non-common view: common_view keeps the iterator type.
    std::vector<int> v = {1, 2, 3, 4};
    auto sub = std::ranges::subrange(v.begin(), std::unreachable_sentinel);
    static_assert(!std::ranges::common_range<decltype(sub)>);
    auto counted = std::views::counted(std::list<int>{7, 8, 9}.begin(), 0);
    static_assert(!std::ranges::common_range<decltype(counted)>);
    auto cv = counted | std::views::common;
    CHECK(cv.begin() == cv.end());
  }
  return 0;
}
