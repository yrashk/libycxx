// The variable templates a program specializes for its own types to opt out of a concept, as
// the library looks them up (DECISIONS §2: these are names a program spells, never renamed):
// [range.sized]/3 disable_sized_range, used by ranges::size ([range.prim.size]/2.3, /2.4);
// [iterator.concept.sizedsentinel]/1, /3 disable_sized_sentinel_for.
#include <cstddef>
#include <forward_list>
#include <iterator>
#include <ranges>

// A forward range with a size() member (so only [range.prim.size]/2.3 makes it sized).
template <bool Opt>
struct counted_list {
  std::forward_list<int> items;
  std::forward_list<int>::iterator begin();
  std::forward_list<int>::iterator end();
  std::size_t size() const;
};
template <>
inline constexpr bool std::ranges::disable_sized_range<counted_list<true>> = true;

static_assert(std::ranges::sized_range<counted_list<false>>);
static_assert(!std::ranges::sized_range<counted_list<true>>);
template <class R>
concept has_ranges_size = requires(R& r) { std::ranges::size(r); };
static_assert(has_ranges_size<counted_list<false>> && !has_ranges_size<counted_list<true>>);
static_assert(std::ranges::forward_range<counted_list<true>>);

// A sentinel that can be subtracted from an int*, and one that opts out of sized_sentinel_for.
template <bool Opt>
struct end_mark {
  friend bool operator==(const int*, end_mark) { return true; }
  friend std::ptrdiff_t operator-(const int*, end_mark) { return 0; }
  friend std::ptrdiff_t operator-(end_mark, const int*) { return 0; }
};
template <>
inline constexpr bool std::disable_sized_sentinel_for<end_mark<true>, const int*> = true;

static_assert(std::sized_sentinel_for<end_mark<false>, const int*>);
static_assert(!std::sized_sentinel_for<end_mark<true>, const int*>);
static_assert(std::sentinel_for<end_mark<true>, const int*>);

int main() {}
