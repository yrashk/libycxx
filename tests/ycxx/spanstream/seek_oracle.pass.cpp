// basic_spanbuf positioning checked exhaustively against an oracle written from
// [spanbuf.virtuals]/2-7: for every open mode (in, out, in|out, with and without ate), buffer
// size (including an empty span with a null data pointer), current get / put position, way,
// which and offset in [-12, 12]:
//   - which = in|out with way = cur fails (/3);
//   - baseoff is 0 (beg), the sequence's own next - begin (cur), or for end pptr() - pbase()
//     when the mode has out but not in, else buf.size() (/4);
//   - a sequence whose next pointer is null can only be "positioned" to newoff 0 (/4);
//   - newoff outside [0, buf.size()] fails (/5); on failure the result is pos_type(-1) and a
//     single positioned sequence is unchanged; otherwise next = begin + newoff and the result
//     is newoff (/6). seekpos(sp, which) is seekoff(off_type(sp), beg, which) (/7).
// Also [spanbuf.members]/1: span() is [pbase(), pptr()) when out is set (so it follows
// seekp), else buf; /3: span(s) resets the pointers (ate puts pptr at the end); /8 setbuf(s, n)
// is span(span(s, n)); [spanbuf.cons]/2-3: the move constructor keeps all six pointers, the
// span and the locale; [spanbuf.assign]: move assignment and swap exchange the state.
#include <spanstream>
#include <ios>
#include <span>
#include <string_view>
#include <utility>
#include "check.hpp"

using ios = std::ios_base;

template <class charT>
struct Probe : std::basic_spanbuf<charT> {
  using base = std::basic_spanbuf<charT>;
  using base::base;
  Probe(Probe&&) = default;
  Probe& operator=(Probe&&) = default;
  long g() const { return this->gptr() ? static_cast<long>(this->gptr() - this->eback()) : -1; }
  long p() const { return this->pptr() ? static_cast<long>(this->pptr() - this->pbase()) : -1; }
  const charT* ptrs(int i) const {
    switch (i) {
      case 0: return this->eback();
      case 1: return this->gptr();
      case 2: return this->egptr();
      case 3: return this->pbase();
      case 4: return this->pptr();
      default: return this->epptr();
    }
  }
};

// Expected newoff for one sequence, or -1 for failure. pos = current offset (-1: null pointer).
long expect_one(bool out_seq, ios::openmode mode, ios::seekdir way, long off, long size, long gpos, long ppos) {
  long next = out_seq ? ppos : gpos;
  long base;
  if (way == ios::beg) base = 0;
  else if (way == ios::cur) base = next < 0 ? 0 : next;
  else base = ((mode & ios::out) && !(mode & ios::in)) ? (ppos < 0 ? 0 : ppos) : size;
  long newoff = base + off;
  if (next < 0 && newoff != 0) return -1;
  if (newoff < 0 || newoff > size) return -1;
  return newoff;
}

template <class charT>
void sweep() {
  charT storage[9] = {};
  const ios::openmode modes[] = {ios::in, ios::out, ios::in | ios::out, ios::out | ios::ate, ios::in | ios::out | ios::ate};
  const ios::seekdir ways[] = {ios::beg, ios::cur, ios::end};
  const ios::openmode whiches[] = {ios::in, ios::out, ios::in | ios::out};
  for (ios::openmode mode : modes) {
    for (long size : {0L, 1L, 5L, 9L}) {
      std::span<charT> s = size == 0 ? std::span<charT>() : std::span<charT>(storage, static_cast<std::size_t>(size));
      for (long g0 = 0; g0 <= size; g0 += 2) {
        for (long p0 = 0; p0 <= size; p0 += 3) {
          for (ios::seekdir way : ways) {
            for (ios::openmode which : whiches) {
              for (long off = -12; off <= 12; ++off) {
                for (int use_seekpos = 0; use_seekpos < 2; ++use_seekpos) {
                  if (use_seekpos && (way != ios::beg || off < 0)) continue;
                  Probe<charT> b(s, mode);
                  // move the get / put positions to g0 / p0 (when those sequences exist)
                  if ((mode & ios::in) && b.g() >= 0)
                    for (long i = 0; i < g0; ++i) b.sbumpc();
                  if ((mode & ios::out) && !(mode & ios::ate) && b.p() >= 0)
                    for (long i = 0; i < p0; ++i) b.sputc(charT('x'));
                  const long gpos = b.g(), ppos = b.p();
                  long want;
                  bool in = (which & ios::in) != 0, out = (which & ios::out) != 0;
                  if (in && out && way == ios::cur) {
                    want = -1;
                  } else {
                    long wi = in ? expect_one(false, mode, way, off, size, gpos, ppos) : 0;
                    long wo = out ? expect_one(true, mode, way, off, size, gpos, ppos) : 0;
                    want = (wi < 0 || wo < 0) ? -1 : (in ? wi : wo);
                  }
                  auto r = use_seekpos ? b.pubseekpos(std::streampos(off), which) : b.pubseekoff(off, way, which);
                  CHECK(std::streamoff(r) == want);
                  if (want >= 0) {
                    if (in) CHECK(b.g() == (gpos < 0 ? -1 : want));
                    else CHECK(b.g() == gpos);
                    if (out) CHECK(b.p() == (ppos < 0 ? -1 : want));
                    else CHECK(b.p() == ppos);
                  } else if (!(in && out)) {
                    CHECK(b.g() == gpos && b.p() == ppos);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

void members_and_moves() {
  char storage[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
  std::span<char> s(storage);
  Probe<char> b(s);  // in | out
  CHECK(b.span().size() == 0);  // out is set: [pbase(), pptr())
  b.sputn("XYZ", 3);
  b.sbumpc();
  CHECK(b.span().size() == 3 && b.span().data() == storage);
  CHECK(b.pubseekoff(1, ios::beg, ios::out) == std::streampos(1));
  CHECK(b.span().size() == 1);  // span() follows the put position
  b.sputc('Q');
  CHECK(std::string_view(storage, 4) == "XQZd");

  // move construction keeps every pointer, the span and the locale
  const char* before[6];
  for (int i = 0; i < 6; ++i) before[i] = b.ptrs(i);
  auto sp = b.span();
  auto loc = b.getloc();
  Probe<char> m(std::move(b));
  for (int i = 0; i < 6; ++i) CHECK(m.ptrs(i) == before[i]);
  CHECK(m.span().data() == sp.data() && m.span().size() == sp.size());
  CHECK(m.getloc() == loc);

  // swap / move assignment exchange the whole state, mode included
  char other[3] = {'1', '2', '3'};
  Probe<char> in_only(std::span<char>(other), ios::in);
  in_only.sbumpc();
  m.swap(in_only);
  CHECK(m.span().data() == other && m.span().size() == 3);  // in-only: span() is buf
  CHECK(m.g() == 1 && m.p() == -1);
  CHECK(in_only.p() == 2 && in_only.g() == 1);
  using std::swap;
  swap(m, in_only);
  CHECK(m.p() == 2 && in_only.g() == 1 && in_only.span().size() == 3);
  Probe<char> target(std::span<char>(storage, 2), ios::out);
  target = std::move(in_only);
  CHECK(target.span().data() == other && target.g() == 1 && target.p() == -1);
  CHECK(target.sgetc() == '2');

  // span(s) re-initializes; ate puts pptr() at the end
  Probe<char> a(s, ios::out | ios::ate);
  CHECK(a.p() == 8 && a.span().size() == 8);
  CHECK(a.sputc('z') == std::char_traits<char>::eof());  // never grows
  a.span(std::span<char>(storage, 3));
  CHECK(a.p() == 3 && a.span().size() == 3);
  // setbuf(s, n) is span(span(s, n))
  CHECK(a.pubsetbuf(storage + 2, 4) == &a);
  CHECK(a.p() == 4 && a.ptrs(3) == storage + 2 && a.ptrs(5) == storage + 6);

  // streams: tellg / tellp / seekg / seekp go through the same functions
  char text[] = "0123456789";
  std::ispanstream is(std::span<char>(text, 10));
  char c;
  is >> c;
  CHECK(c == '0' && is.tellg() == std::streampos(1));
  is.seekg(-2, ios::end);
  is >> c;
  CHECK(c == '8');
  is.seekg(11);
  CHECK(is.fail());
  std::ospanstream os(std::span<char>(text, 10));
  os << "abc";
  CHECK(os.tellp() == std::streampos(3));
  os.seekp(-1, ios::cur);
  os << 'Z';
  CHECK(std::string_view(os.span().data(), os.span().size()) == "abZ");
  os.seekp(0, ios::end);  // out without in: end is pptr() - pbase()
  CHECK(os.tellp() == std::streampos(3));
}

int main() {
  sweep<char>();
  sweep<wchar_t>();
  members_and_moves();
}
