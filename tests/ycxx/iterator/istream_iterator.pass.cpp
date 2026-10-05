// [istream.iterator.general]: member types (input_iterator_tag, value_type T, difference_type
// Distance, pointer const T*, reference const T&, char_type, traits_type, istream_type);
// [istream.iterator.cons]/1-4: the default and default_sentinel_t constructors make the
// end-of-stream iterator (constexpr when T() is a constant initializer); istream_iterator(s)
// stores addressof(s), value-initializes value and calls operator++(); /5-6: the copy
// constructor copies both, noexcept(is_nothrow_copy_constructible_v<T>); /7: the destructor is
// trivial if T's is. [istream.iterator.ops]: *it returns value, it-> addressof(value);
// ++it is "if (!(*in_stream >> value)) in_stream = nullptr;"; it++ (defaulted postfix) returns
// the previous value of the iterator; x == y iff in_stream pointers are equal; it ==
// default_sentinel iff !in_stream. The iterator models input_iterator ([iterator.concept.input])
// and works with the algorithms.
// COUNTERPART: libcxx:iterators/stream.iterators/istream.iterator/types.pass.cpp
#include <iterator>
#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <type_traits>
#include "check.hpp"

using It = std::istream_iterator<int>;
static_assert(std::is_same_v<It::iterator_category, std::input_iterator_tag>);
static_assert(std::is_same_v<It::value_type, int>);
static_assert(std::is_same_v<It::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<It::pointer, const int*>);
static_assert(std::is_same_v<It::reference, const int&>);
static_assert(std::is_same_v<It::char_type, char>);
static_assert(std::is_same_v<It::traits_type, std::char_traits<char>>);
static_assert(std::is_same_v<It::istream_type, std::istream>);
using WIt = std::istream_iterator<long, wchar_t, std::char_traits<wchar_t>, short>;
static_assert(std::is_same_v<WIt::difference_type, short>);
static_assert(std::is_same_v<WIt::istream_type, std::wistream>);
static_assert(std::input_iterator<It> && !std::forward_iterator<It>);
static_assert(std::sentinel_for<std::default_sentinel_t, It>);
static_assert(std::is_trivially_destructible_v<It>);
static_assert(!std::is_trivially_destructible_v<std::istream_iterator<std::string>>);
static_assert(std::is_nothrow_copy_constructible_v<It>);
static_assert(!std::is_nothrow_copy_constructible_v<std::istream_iterator<std::string>>);
static_assert(std::is_same_v<decltype(*std::declval<const It&>()), const int&>);
static_assert(std::is_same_v<decltype(std::declval<const It&>().operator->()), const int*>);
static_assert(std::is_same_v<decltype(++std::declval<It&>()), It&>);
static_assert(std::is_same_v<decltype(std::declval<It&>()++), It>);

constexpr It eos1;
constexpr It eos2(std::default_sentinel);
constexpr It eos_copy = eos1;  // copy constructor usable in constant expressions here

struct Pt {
  int x = 0, y = 0;
};
std::istream& operator>>(std::istream& is, Pt& p) { return is >> p.x >> p.y; }

int main() {
  {
    std::istringstream in("1 2 3");
    It it(in);  // reads the first value
    CHECK(*it == 1);
    CHECK(it != std::default_sentinel && it != It());
    It old = it++;
    CHECK(*old == 1 && *it == 2);
    It& r = ++it;
    CHECK(&r == &it && *it == 3);
    ++it;  // extraction fails: end of stream
    CHECK(it == std::default_sentinel && it == It());
    CHECK(std::default_sentinel == it);
  }
  {
    // two iterators on the same stream compare equal (same in_stream)
    std::istringstream in("4 5 6 7");
    It a(in);
    It b(in);  // reads 5
    CHECK(a == b);
    CHECK(*a == 4 && *b == 5);
    std::istringstream other("4");
    CHECK(a != It(other));
  }
  {
    // a failed first read makes the iterator end-of-stream immediately
    std::istringstream in("x 1");
    It it(in);
    CHECK(it == It());
    std::istringstream empty("");
    CHECK(It(empty) == std::default_sentinel);
  }
  {
    // operator-> and a user type
    std::istringstream in("1 2 3 4 5");
    std::istream_iterator<Pt> it(in);
    CHECK(it->x == 1 && it->y == 2);
    CHECK(&*it == it.operator->());
    ++it;
    CHECK(it->x == 3 && it->y == 4);
    ++it;  // only "5" left: extraction fails
    CHECK(it == std::istream_iterator<Pt>());
  }
  {
    // with algorithms
    std::istringstream in("3 1 4 1 5 9 2 6");
    int out[8] = {};
    int* e = std::copy(It(in), It(), out);
    CHECK(e == out + 8 && out[0] == 3 && out[7] == 6);
    std::istringstream in2("10 20 30");
    long long sum = 0;
    for (auto i = It(in2); i != std::default_sentinel; ++i) sum += *i;
    CHECK(sum == 60);
    std::istringstream in3("5 6 7");
    auto sub = std::ranges::subrange(It(in3), std::default_sentinel);
    CHECK(std::ranges::count(sub, 6) == 1);
  }
  {
    // wide streams and a non-default Distance
    std::wistringstream in(L"8 9");
    WIt it(in);
    CHECK(*it == 8);
    ++it;
    CHECK(*it == 9);
    ++it;
    CHECK(it == WIt());
  }
  {
    // strings: whitespace-separated words
    std::istringstream in("alpha beta  gamma\n");
    std::istream_iterator<std::string> it(in), end;
    std::string words[3];
    CHECK(std::copy(it, end, words) == words + 3);
    CHECK(words[0] == "alpha" && words[2] == "gamma");
  }
  CHECK(eos1 == eos2 && eos1 == eos_copy);
  CHECK(eos1 == std::default_sentinel && std::default_sentinel == eos2);
  return 0;
}
