// [stringbuf.virtuals]/5-8: overflow(c) appends c, making a write position available "only if
// ios_base::out is set in mode"; "If ios_base::in is set in mode, the function alters the read
// end pointer egptr() to point just past the new write position", so characters written to an
// in|out stringbuf become readable; /1: underflow returns *gptr() when a read position is
// available ("Any character in the underlying buffer which has been initialized is considered
// to be part of the input sequence"), otherwise eof(). With in only, output fails; with out
// only, input fails.
// [stringbuf.cons]/13-14: the move constructors (with and without an allocator) preserve str()
// and the offsets gptr() - eback(), egptr() - eback(), pptr() - pbase(), epptr() - pbase();
// [stringbuf.assign]: move assignment likewise; swap exchanges the states.
// [stringbuf.cons]/4-5: basic_stringbuf(which, a) is empty and uses a (get_allocator(),
// [stringbuf.members]).
#include <sstream>
#include <string>
#include <utility>
#include <memory>
#include "test_allocators.hpp"
#include "check.hpp"

using traits = std::char_traits<char>;

struct SB : std::stringbuf {
  using std::stringbuf::stringbuf;
  SB(SB&& o) = default;
  SB& operator=(SB&&) = default;
  std::ptrdiff_t g() const { return gptr() - eback(); }
  std::ptrdiff_t eg() const { return egptr() - eback(); }
  std::ptrdiff_t p() const { return pptr() - pbase(); }
  bool has_get() const { return eback() != nullptr; }
  bool has_put() const { return pbase() != nullptr; }
};

int main() {
  {
    // characters written become part of the input sequence
    SB b(std::ios_base::in | std::ios_base::out);
    CHECK(b.sgetc() == traits::eof());
    CHECK(b.sputc('a') == 'a');
    CHECK(b.in_avail() == 1);
    CHECK(b.sgetc() == 'a');
    for (char c = 'b'; c <= 'z'; ++c) CHECK(b.sputc(c) == c);  // several reallocations
    std::string got;
    int ch;
    while ((ch = b.sbumpc()) != traits::eof()) got.push_back(traits::to_char_type(ch));
    CHECK(got == "abcdefghijklmnopqrstuvwxyz");
    CHECK(b.sputn("0123456789", 10) == 10);
    char buf[11] = {};
    CHECK(b.sgetn(buf, 10) == 10 && std::string(buf) == "0123456789");
    CHECK(b.str() == got + "0123456789");
    CHECK(b.sputbackc('9') == '9');  // the backup sequence survives reallocation
    CHECK(b.sgetc() == '9');
  }
  {
    // reading from an initial string, then writing at the beginning (no ate): overwrite
    SB b(std::string("hello"));
    CHECK(b.sbumpc() == 'h');
    CHECK(b.sputn("J", 1) == 1);
    CHECK(b.sgetc() == 'e');
    CHECK(b.str() == "Jello");
    CHECK(b.sputn("ELLO-world", 10) == 10);
    std::string rest;
    int ch;
    while ((ch = b.sbumpc()) != traits::eof()) rest.push_back(traits::to_char_type(ch));
    CHECK(rest == "ELLO-world");
  }
  {
    SB in_only(std::string("abc"), std::ios_base::in);
    CHECK(in_only.sputc('x') == traits::eof());
    CHECK(in_only.str() == "abc");
    SB out_only(std::string("abc"), std::ios_base::out);
    CHECK(out_only.sgetc() == traits::eof());
    CHECK(out_only.sputc('x') == 'x');
    CHECK(out_only.str() == "xbc");
  }
  {
    // move construction preserves the contents and the offsets
    SB a(std::string("0123456789"));
    a.sbumpc();
    a.sbumpc();
    a.sputn("ab", 2);
    CHECK(a.g() == 2 && a.p() == 2);
    std::ptrdiff_t eg = a.eg();
    SB b(std::move(a));
    CHECK(b.str() == "ab23456789");
    CHECK(b.g() == 2 && b.eg() == eg && b.p() == 2);
    CHECK(b.sgetc() == '2');
    b.sputc('c');
    CHECK(b.str() == "abc3456789");
    // move assignment
    SB c(std::string("zz"));
    c = std::move(b);
    CHECK(c.str() == "abc3456789" && c.g() == 2 && c.p() == 3);
    // swap
    SB d(std::string("XY"), std::ios_base::in);
    d.sbumpc();
    c.swap(d);
    CHECK(c.str() == "XY" && c.g() == 1 && c.sgetc() == 'Y');
    CHECK(c.sputc('q') == traits::eof());  // the mode was swapped too
    CHECK(d.str() == "abc3456789" && d.g() == 2 && d.p() == 3);
    CHECK(d.sputc('d') == 'd');
  }
  {
    // allocator-extended constructors and get_allocator
    using A = IdAlloc<char>;
    using S = std::basic_stringbuf<char, traits, A>;
    S s(std::ios_base::in | std::ios_base::out, A(7));
    CHECK(s.get_allocator() == A(7));
    CHECK(s.str().empty());
    s.sputn("text", 4);
    S m(std::move(s), A(9));
    CHECK(m.get_allocator() == A(9));
    CHECK(m.str() == "text");
    CHECK(m.sgetc() == 't');
  }
  return 0;
}
