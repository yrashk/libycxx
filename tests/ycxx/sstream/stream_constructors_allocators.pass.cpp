// The string streams' constructors and members, with a stateful allocator:
//   [istringstream.cons]/1-10, [ostringstream.cons], [stringstream.cons]/1-10: the stringbuf is
//     made with which | in (istringstream), which | out (ostringstream) or which as given
//     (stringstream, so a stringstream opened with in only cannot be written); the allocator
//     forms store a; the SAlloc forms (/5-7) copy a string with another allocator; the forms
//     taking a T convertible to basic_string_view (/8-10) default which and a; the constructors
//     from a string and a mode, and the one from a mode, are explicit.
//   [istringstream.members]/1-10 (and the same members of the other two): rdbuf() is the
//     member stringbuf, even through a const stream; str(), str(sa), str() &&, view() noexcept,
//     str(s), str(s with another allocator), str(s&&) and str(t) forward to it.
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "test_allocators.hpp"

using A = IdAlloc<char>;
using Tr = std::char_traits<char>;
using Str = std::basic_string<char, Tr, A>;
using IS = std::basic_istringstream<char, Tr, A>;
using OS = std::basic_ostringstream<char, Tr, A>;
using SS = std::basic_stringstream<char, Tr, A>;

static_assert(!std::is_convertible_v<std::ios_base::openmode, IS> && !std::is_convertible_v<const Str&, OS>);
static_assert(!std::is_convertible_v<std::string_view, SS> && std::is_constructible_v<SS, std::string_view, A>);
static_assert(!std::is_constructible_v<IS, int>); // T must convert to a string_view
static_assert(std::is_same_v<decltype(std::declval<const IS&>().rdbuf()), std::basic_stringbuf<char, Tr, A>*>);
static_assert(noexcept(std::declval<const SS&>().view()));

int main() {
  const A a7(7);
  const std::string other("other allocator");

  // istringstream: in is always added.
  IS i1(std::ios_base::out); // which | in
  i1.str("42");
  int v = 0;
  CHECK((i1 >> v) && v == 42);
  IS i2(Str("1 2", a7), std::ios_base::binary);
  int x = 0, y = 0;
  CHECK((i2 >> x >> y) && x == 1 && y == 2);
  IS i3(std::ios_base::in, a7);
  CHECK(i3.rdbuf()->get_allocator().id == 7 && i3.str().empty() && i3.str().get_allocator().id == 7);
  IS i4(other, std::ios_base::in, a7); // SAlloc form
  CHECK(i4.view() == "other allocator" && i4.str().get_allocator().id == 7);
  IS i5(other); // SAlloc form without an allocator
  CHECK(i5.str() == "other allocator");
  IS i6(std::string_view("sv text"), a7); // T form
  CHECK(i6.view() == "sv text" && i6.rdbuf()->get_allocator().id == 7);
  std::string word;
  CHECK((i6 >> word) && word == "sv");
  IS i7(Str("moved", a7));
  CHECK(i7.str() == "moved");

  // ostringstream: out is always added, so writing works with which == in.
  OS o1(std::ios_base::in);
  o1 << "abc";
  CHECK(o1.str() == "abc");
  OS o2(Str("XYZW", a7)); // overwrites from the beginning
  o2 << "ab";
  CHECK(o2.view() == "abZW");
  OS o3(Str("XY", a7), std::ios_base::ate);
  o3 << "z";
  CHECK(o3.str() == "XYz");
  OS o4(std::ios_base::out, a7);
  o4 << 12;
  CHECK(o4.rdbuf()->get_allocator().id == 7 && o4.str() == "12");
  OS o5("char pointer", std::ios_base::ate, a7); // T = const char*
  o5 << '!';
  CHECK(o5.view() == "char pointer!");

  // stringstream: which is used as given.
  SS s1(std::ios_base::in);
  s1 << "x";
  CHECK(s1.fail() && s1.view().empty()); // no put area: the insertion fails
  SS s2(Str("5 6", a7), std::ios_base::in | std::ios_base::out, a7);
  int p = 0, q = 0;
  CHECK((s2 >> p >> q) && p == 5 && q == 6);
  SS s3(std::ios_base::in | std::ios_base::out, a7);
  s3 << 3.5;
  double d = 0;
  CHECK((s3 >> d) && d == 3.5 && s3.rdbuf()->get_allocator().id == 7);
  SS s4(std::string_view("view"));
  CHECK(s4.str() == "view" && s4.rdbuf()->get_allocator().id == 0);

  // Members: rdbuf() through a const stream is the same stringbuf.
  const SS& cs = s2;
  CHECK(cs.rdbuf() == s2.rdbuf());
  // str(sa)
  IdAlloc<char> a9(9);
  auto with9 = s2.str(a9);
  static_assert(std::is_same_v<decltype(with9), Str>);
  CHECK(with9 == "5 6" && with9.get_allocator().id == 9);
  // str(s), str(s with another allocator), str(s&&), str(t)
  s2.str(Str("one", a7));
  CHECK(s2.view() == "one");
  s2.str(other);
  CHECK(s2.view() == "other allocator");
  Str moved_in("two", a7);
  s2.str(std::move(moved_in));
  CHECK(s2.view() == "two");
  s2.str(std::string_view("three"));
  CHECK(s2.view() == "three");
  // str() && moves the buffer out and leaves the stream empty.
  Str out = std::move(s2).str();
  CHECK(out == "three" && s2.view().empty() && s2.str().empty());
  Str iout = std::move(i4).str();
  CHECK(iout == "other allocator" && i4.view().empty());
  Str oout = std::move(o5).str();
  CHECK(oout == "char pointer!" && o5.view().empty());
  return 0;
}
