// [range.istream.overview]/1: basic_istream_view models input_range and reads successive
// elements with operator>>; /2: views::istream<T>(E) is basic_istream_view<T, U::char_type,
// U::traits_type>(E) for a stream type U, ill-formed otherwise; /3 Example 1 (0-1-2-3-4-).
// [range.istream.view]: begin() reads the first value ("*stream_ >> value_") before returning
// the iterator; end() is default_sentinel and noexcept. [range.istream.iterator]: the iterator
// is move-only (copy constructor and copy assignment deleted), iterator_concept is
// input_iterator_tag, difference_type ptrdiff_t, value_type Val; ++ reads the next value into
// the view (post-increment returns void); * returns a Val& to the view's stored value (the
// same object each time); it == default_sentinel is !stream.
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"

using IV = std::ranges::basic_istream_view<int, char>;
using It = std::ranges::iterator_t<IV>;
static_assert(std::ranges::input_range<IV> && std::ranges::view<IV>);
static_assert(!std::ranges::forward_range<IV>);
static_assert(std::is_same_v<IV, std::ranges::istream_view<int>>);
static_assert(std::is_same_v<std::ranges::basic_istream_view<int, wchar_t>, std::ranges::wistream_view<int>>);
static_assert(std::is_same_v<std::ranges::sentinel_t<IV>, std::default_sentinel_t>);
static_assert(std::is_same_v<It::iterator_concept, std::input_iterator_tag>);
static_assert(std::is_same_v<It::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<It::value_type, int>);
static_assert(!std::is_copy_constructible_v<It> && !std::is_copy_assignable_v<It>);
static_assert(std::is_move_constructible_v<It> && std::is_move_assignable_v<It>);
static_assert(std::is_same_v<decltype(*std::declval<const It&>()), int&>);
static_assert(std::is_same_v<decltype(++std::declval<It&>()), It&>);
static_assert(std::is_same_v<decltype(std::declval<It&>()++), void>);
static_assert(noexcept(std::declval<const IV&>().end()));
static_assert(std::is_same_v<decltype(std::views::istream<int>(std::declval<std::istringstream&>())), IV>);
static_assert(std::is_same_v<decltype(std::views::istream<int>(std::declval<std::wistringstream&>())),
                             std::ranges::wistream_view<int>>);
template <class S>
concept can_istream = requires(S& s) { std::views::istream<int>(s); };
static_assert(can_istream<std::istringstream> && can_istream<std::istream>);
static_assert(!can_istream<std::ostringstream> && !can_istream<std::string>);

int main() {
  {  // [range.istream.overview] Example 1.
    auto ints = std::istringstream{"0 1  2   3     4"};
    std::ostringstream out;
    std::ranges::copy(std::views::istream<int>(ints), std::ostream_iterator<int>{out, "-"});
    CHECK(out.str() == "0-1-2-3-4-");
  }
  {  // begin() reads; * refers to the view's value; ++ reads the next one into it.
    std::istringstream in("10 20 30");
    auto v = std::views::istream<int>(in);
    auto it = v.begin();
    CHECK(it != v.end());
    int& a = *it;
    CHECK(a == 10);
    ++it;
    CHECK(&*it == &a && a == 20);
    it++;
    CHECK(*it == 30 && !(it == std::default_sentinel));
    *it = 7;  // a Val&, the stored value
    CHECK(*it == 7);
    ++it;
    CHECK(it == std::default_sentinel && std::default_sentinel == it);
    CHECK(in.fail());
  }
  {  // An empty stream: begin() already compares equal to end().
    std::istringstream in("");
    auto v = std::views::istream<int>(in);
    CHECK(v.begin() == v.end());
  }
  {  // A value that does not parse ends the range at the first failure.
    std::istringstream in("1 2 x 4");
    std::vector<int> got;
    for (int i : std::views::istream<int>(in)) got.push_back(i);
    CHECK((got == std::vector<int>{1, 2}));
  }
  {  // Each begin() reads again from the stream; moving the iterator keeps its position.
    std::istringstream in("a b c d");
    std::ranges::basic_istream_view<std::string, char> v(in);
    auto it = v.begin();
    CHECK(*it == "a");
    auto it2 = std::move(it);
    ++it2;
    CHECK(*it2 == "b");
    auto it3 = v.begin();  // reads "c"
    CHECK(*it3 == "c" && *it2 == "c");
  }
  {  // Wide streams.
    std::wistringstream in(L"5 6");
    std::vector<int> got;
    for (int i : std::views::istream<int>(in)) got.push_back(i);
    CHECK((got == std::vector<int>{5, 6}));
  }
}
