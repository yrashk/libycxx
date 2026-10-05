// [istreambuf.iterator.general]: member types (input_iterator_tag, value_type charT,
// difference_type traits::off_type, reference charT, char_type, traits_type, int_type,
// streambuf_type, istream_type); /1: "All specializations of istreambuf_iterator shall have a
// trivial copy constructor, a constexpr default constructor, and a trivial destructor"; the
// default and nullptr constructors make an end-of-stream iterator. [istreambuf.iterator.cons]:
// the default_sentinel_t constructor too; (s) uses s.rdbuf(); all noexcept.
// [istreambuf.iterator.ops]: *it is sbuf_->sgetc() (does not advance); ++it is sbumpc();
// it++ returns a proxy whose * is the character before the increment, and from which an
// istreambuf_iterator can be constructed; equal(b) is true iff both or neither are at
// end-of-stream, "regardless of what streambuf object they use"; == is equal; == 
// default_sentinel is equal(end-of-stream). An iterator becomes end-of-stream when sgetc()
// returns eof.
// COUNTERPART: libstdcxx:24_iterators/istreambuf_iterator/requirements/typedefs.cc
// COUNTERPART: libcxx:iterators/stream.iterators/istreambuf.iterator/types.pass.cpp
#include <iterator>
#include <algorithm>
#include <sstream>
#include <string>
#include <type_traits>
#include "check.hpp"

using It = std::istreambuf_iterator<char>;
static_assert(std::is_same_v<It::iterator_category, std::input_iterator_tag>);
static_assert(std::is_same_v<It::value_type, char>);
static_assert(std::is_same_v<It::difference_type, std::char_traits<char>::off_type>);
static_assert(std::is_same_v<It::reference, char>);
static_assert(std::is_same_v<It::char_type, char>);
static_assert(std::is_same_v<It::traits_type, std::char_traits<char>>);
static_assert(std::is_same_v<It::int_type, int>);
static_assert(std::is_same_v<It::streambuf_type, std::streambuf>);
static_assert(std::is_same_v<It::istream_type, std::istream>);
static_assert(std::is_same_v<std::istreambuf_iterator<wchar_t>::int_type, std::wint_t>);
static_assert(std::is_trivially_copy_constructible_v<It>);
static_assert(std::is_trivially_destructible_v<It>);
static_assert(std::is_trivially_copy_constructible_v<std::istreambuf_iterator<wchar_t>>);
static_assert(std::is_nothrow_default_constructible_v<It>);
static_assert(std::is_nothrow_constructible_v<It, std::istream&>);
static_assert(std::is_nothrow_constructible_v<It, std::streambuf*>);
static_assert(std::is_nothrow_constructible_v<It, std::default_sentinel_t>);
static_assert(std::input_iterator<It> && !std::forward_iterator<It>);
static_assert(std::sentinel_for<std::default_sentinel_t, It>);
static_assert(std::is_same_v<decltype(*std::declval<const It&>()), char>);
static_assert(std::is_same_v<decltype(++std::declval<It&>()), It&>);
static_assert(std::is_same_v<decltype(*std::declval<It&>()++), char>);

constexpr It eos;  // constexpr default constructor
constexpr It eos2(std::default_sentinel);

int main() {
  const It eos3(nullptr);  // through the streambuf_type* constructor
  {
    std::istringstream in("abc");
    It it(in);
    CHECK(*it == 'a' && *it == 'a');  // * does not advance
    CHECK(it != eos && it != std::default_sentinel);
    auto p = it++;
    CHECK(*p == 'a' && *it == 'b');
    CHECK(&++it == &it && *it == 'c');
    ++it;
    CHECK(it == eos && it == eos2 && it == eos3 && it == std::default_sentinel);
    CHECK(it.equal(eos) && eos.equal(it));
  }
  {
    // equal() ignores which streambuf; constructed from a proxy
    std::istringstream a("x"), b("yz");
    It ia(a), ib(b.rdbuf());
    CHECK(ia == ib && ia.equal(ib));
    auto p = ib++;
    It from_proxy(p);
    CHECK(*from_proxy == 'z');
  }
  {
    // whitespace is not skipped; the whole contents
    std::istringstream in(" a b\n");
    std::string s(It(in), It{});
    CHECK(s == " a b\n");
    // the stream position advanced to the end through the streambuf
    CHECK(in.rdbuf()->sgetc() == std::char_traits<char>::eof());
  }
  {
    std::istringstream in("hello");
    auto r = std::ranges::subrange(It(in), std::default_sentinel);
    CHECK(std::ranges::count(r, 'l') == 2);
  }
  {
    std::wistringstream in(L"été");
    std::wstring s(std::istreambuf_iterator<wchar_t>(in), {});
    CHECK(s == L"été");
  }
  {
    // an iterator becomes end-of-stream when the streambuf has no more input; an empty buffer
    // gives an end-of-stream iterator immediately
    std::istringstream in("");
    CHECK(It(in) == eos);
  }
  return 0;
}
