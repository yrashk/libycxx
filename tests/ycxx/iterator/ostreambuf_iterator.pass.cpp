// [ostreambuf.iterator.general]: member types (output_iterator_tag, value_type void,
// difference_type ptrdiff_t, pointer void, reference void, char_type, traits_type,
// streambuf_type, ostream_type); constructors from ostream_type& (uses s.rdbuf()) and
// streambuf_type*, both noexcept. [ostreambuf.iter.ops]/1: it = c
// calls sbuf_->sputc(c) if failed() is false, otherwise has no effect; /3-4: *it, ++it, it++
// return *this; /5: failed() is true iff a prior sputc returned eof. Models
// output_iterator<It, const charT&>.
// COUNTERPART: libcxx:iterators/stream.iterators/ostreambuf.iterator/types.pass.cpp
#include <iterator>
#include <algorithm>
#include <sstream>
#include <streambuf>
#include <string>
#include <type_traits>
#include "check.hpp"

using It = std::ostreambuf_iterator<char>;
static_assert(std::is_same_v<It::iterator_category, std::output_iterator_tag>);
static_assert(std::is_same_v<It::value_type, void>);
static_assert(std::is_same_v<It::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<It::pointer, void>);
static_assert(std::is_same_v<It::reference, void>);
static_assert(std::is_same_v<It::char_type, char>);
static_assert(std::is_same_v<It::traits_type, std::char_traits<char>>);
static_assert(std::is_same_v<It::streambuf_type, std::streambuf>);
static_assert(std::is_same_v<It::ostream_type, std::ostream>);
static_assert(std::is_nothrow_constructible_v<It, std::ostream&>);
static_assert(std::is_nothrow_constructible_v<It, std::streambuf*>);
static_assert(std::output_iterator<It, const char&>);
static_assert(noexcept(std::declval<const It&>().failed()));
static_assert(std::is_same_v<decltype(std::declval<It&>()++), It&>);

// A streambuf that accepts at most `room` characters and then reports eof from overflow.
struct Limited : std::streambuf {
  std::string got;
  int room;
  int calls = 0;
  explicit Limited(int r) : room(r) {}
  int_type overflow(int_type c) override {
    ++calls;
    if (room == 0) return traits_type::eof();
    --room;
    got.push_back(traits_type::to_char_type(c));
    return c;
  }
};

int main() {
  {
    std::ostringstream os;
    It it(os);
    CHECK(&*it == &it && &++it == &it && &it++ == &it);
    *it++ = 'h';
    *it = 'i';
    it = '!';
    CHECK(os.str() == "hi!" && !it.failed());
  }
  {
    // failed() after sputc returns eof; later assignments have no effect (no further calls)
    Limited buf(2);
    It it(&buf);
    it = 'a';
    it = 'b';
    CHECK(!it.failed());
    it = 'c';
    CHECK(it.failed());
    int calls = buf.calls;
    it = 'd';
    it = 'e';
    CHECK(buf.calls == calls);
    CHECK(buf.got == "ab");
    // a copy carries the failed state; a fresh iterator on the same buffer does not
    It copy = it;
    CHECK(copy.failed());
    CHECK(!It(&buf).failed());
  }
  {
    std::ostringstream os;
    std::string s = "copy me";
    std::copy(s.begin(), s.end(), It(os));
    CHECK(os.str() == "copy me");
    auto r = std::ranges::copy(std::string_view("+++"), It(os.rdbuf()));
    CHECK(!r.out.failed());
    CHECK(os.str() == "copy me+++");
  }
  {
    std::wostringstream os;
    std::ostreambuf_iterator<wchar_t> it(os);
    it = L'w';
    CHECK(os.str() == L"w");
  }
  return 0;
}
