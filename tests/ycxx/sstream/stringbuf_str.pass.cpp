// [stringbuf.members]: str() const& returns a copy of view(); str(s) replaces the buffer
// ("buf = s; init-buf-ptrs();"); str() && moves the buffer out and leaves it empty (/9-10);
// view() returns sv(pbase(), high_mark - pbase()) when out is set, otherwise
// sv(eback(), egptr() - eback()) for in, otherwise sv() (/12); high_mark: writes that only
// overwrite do not shorten the sequence (/1). str(sa) with another allocator; str(t) from a
// string_view-convertible t (/18-19). [stringbuf.cons]: the template constructor from a
// string_view-convertible t (/10-11).
#include <sstream>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include "check.hpp"

template<class T>
struct OtherAlloc : std::allocator<T> {
  OtherAlloc() = default;
  template<class U> OtherAlloc(const OtherAlloc<U>&) {}
  template<class U> struct rebind { using other = OtherAlloc<U>; };
};

int main() {
  std::stringbuf sb("hello");
  CHECK(sb.str() == "hello");
  CHECK(sb.view() == "hello");
  static_assert(noexcept(sb.view()));
  // overwrite the first characters: the high mark stays at 5
  sb.sputn("HE", 2);
  CHECK(sb.str() == "HEllo");
  sb.sputn("LLO!!", 5);
  CHECK(sb.str() == "HELLO!!");

  sb.str("new");
  CHECK(sb.str() == "new");
  CHECK(sb.sgetc() == 'n');  // input sequence reset to the beginning

  std::string moved = std::move(sb).str();
  CHECK(moved == "new");
  CHECK(sb.str().empty() && sb.view().empty());

  std::string src = "rvalue";
  sb.str(std::move(src));
  CHECK(sb.view() == "rvalue");

  std::string_view sv = "from view";
  sb.str(sv);
  CHECK(sb.str() == "from view");

  auto other = sb.str(OtherAlloc<char>());
  static_assert(std::is_same_v<decltype(other), std::basic_string<char, std::char_traits<char>, OtherAlloc<char>>>);
  CHECK(std::string_view(other) == "from view");
  sb.str(std::basic_string<char, std::char_traits<char>, OtherAlloc<char>>("alloc"));
  CHECK(sb.str() == "alloc");

  // modes
  std::stringbuf in_only("abc", std::ios_base::in);
  CHECK(in_only.view() == "abc");
  CHECK(in_only.sputc('x') == std::char_traits<char>::eof());  // no output sequence
  std::stringbuf out_only("abc", std::ios_base::out);
  CHECK(out_only.sgetc() == std::char_traits<char>::eof());  // no input sequence
  out_only.sputc('X');
  CHECK(out_only.str() == "Xbc");
  std::stringbuf ate("abc", std::ios_base::out | std::ios_base::ate);
  ate.sputc('d');
  CHECK(ate.str() == "abcd");

  std::stringbuf from_sv(std::string_view("sv ctor"));
  CHECK(from_sv.str() == "sv ctor");
  std::stringbuf from_sv_mode(std::string_view("in"), std::ios_base::in);
  CHECK(from_sv_mode.sgetc() == 'i');
  static_assert(!std::is_convertible_v<std::string_view, std::stringbuf>);
  return 0;
}
